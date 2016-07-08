#include "IGTLinkClient.h"
#include "TrackedFrameMessage.h"
#include "igtlCommandMessage.h"
#include "igtlCommon.h"
#include "igtlMessageHeader.h"
#include "igtlOSUtil.h"
#include "igtlServerSocket.h"
#include "igtlTrackingDataMessage.h"

#include <chrono>
#include <collection.h>
#include <pplawait.h>
#include <ppltasks.h>

static const int CLIENT_SOCKET_TIMEOUT_MSEC = 500;

namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WSS = Windows::Storage::Streams;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;
namespace WDXD = Windows::Data::Xml::Dom;

namespace UWPOpenIGTLink
{

  //----------------------------------------------------------------------------
  IGTLinkClient::IGTLinkClient()
    : IgtlMessageFactory( igtl::MessageFactory::New() )
    , ClientSocket( igtl::ClientSocket::New() )
  {
    IgtlMessageFactory->AddMessageType( "TRACKEDFRAME", ( igtl::MessageFactory::PointerToMessageBaseNew )&igtl::TrackedFrameMessage::New );
    this->ServerHost = L"127.0.0.1";
    this->ServerPort = 18944;
    this->ServerIGTLVersion = IGTL_HEADER_VERSION_2;
  }

  //----------------------------------------------------------------------------
  IGTLinkClient::~IGTLinkClient()
  {
    auto action = this->DisconnectAsync();
    auto disconnectTask = concurrency::create_task( action );
    disconnectTask.wait();
  }

  //----------------------------------------------------------------------------
  Windows::Foundation::IAsyncOperation<bool>^ IGTLinkClient::ConnectAsync( double timeoutSec )
  {
    auto token = this->CancellationTokenSource.get_token();

    return concurrency::create_async( [this, timeoutSec, token]() -> bool
    {
      auto connectTask = concurrency::create_task( [this, timeoutSec, token]() -> bool
      {
        const int retryDelaySec = 1.0;
        int errorCode = 1;
        auto start = std::chrono::high_resolution_clock::now();
        while ( errorCode != 0 )
        {
          std::wstring wideStr( this->ServerHost->Begin() );
          std::string str( wideStr.begin(), wideStr.end() );
          errorCode = this->ClientSocket->ConnectToServer( str.c_str(), this->ServerPort );
          std::chrono::duration<double, std::milli> timeDiff = std::chrono::high_resolution_clock::now() - start;
          if ( timeDiff.count() > timeoutSec * 1000 )
          {
            // time is up
            break;
          }
          std::this_thread::sleep_for( std::chrono::seconds( retryDelaySec ) );
        }

        if ( errorCode != 0 )
        {
          return false;
        }

        this->ClientSocket->SetTimeout( CLIENT_SOCKET_TIMEOUT_MSEC );
        return true;
      } );

      // Wait (inside the async operation) and retrieve the result of connection
      bool result = connectTask.get();

      if( result )
      {
        // We're connected, start the data receiver thread
        concurrency::create_task( [this, token]( void )
        {
          this->DataReceiverPump( this, token );
        } );
      }

      return result;
    } );
  }

  //----------------------------------------------------------------------------
  Windows::Foundation::IAsyncAction^ IGTLinkClient::DisconnectAsync()
  {
    return concurrency::create_async( [this]()
    {
      this->CancellationTokenSource.cancel();

      {
        Concurrency::critical_section::scoped_lock lock( this->SocketMutex );
        this->ClientSocket->CloseSocket();
      }
    } );
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::SendMessage( igtl::MessageBase::Pointer packedMessage )
  {
    int success = 0;
    {
      Concurrency::critical_section::scoped_lock lock( SocketMutex );
      success = this->ClientSocket->Send( packedMessage->GetBufferPointer(), packedMessage->GetBufferSize() );
    }
    if ( !success )
    {
      std::cerr << "OpenIGTLink client couldn't send message to server." << std::endl;
      return false;
    }
    return true;
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::DataReceiverPump( IGTLinkClient^ self, concurrency::cancellation_token token )
  {
    while ( !token.is_canceled() )
    {
      igtl::MessageHeader::Pointer headerMsg;
      {
        igtl::MessageFactory::Pointer factory = igtl::MessageFactory::New();
        headerMsg = factory->CreateHeaderMessage( IGTL_HEADER_VERSION_1 );
      }

      // Receive generic header from the socket
      int numOfBytesReceived = 0;
      {
        Concurrency::critical_section::scoped_lock lock( self->SocketMutex );
        if ( !self->ClientSocket->GetConnected() )
        {
          // We've become disconnected while waiting for the socket, we're done here!
          return;
        }
        numOfBytesReceived = self->ClientSocket->Receive( headerMsg->GetBufferPointer(), headerMsg->GetBufferSize() );
      }
      if ( numOfBytesReceived == 0 // No message received
           || numOfBytesReceived != headerMsg->GetPackSize() // Received data is not as we expected
         )
      {
        // Failed to receive data, maybe the socket is disconnected
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        continue;
      }

      int c = headerMsg->Unpack( 1 );
      if ( !( c & igtl::MessageHeader::UNPACK_HEADER ) )
      {
        std::cerr << "Failed to receive reply (invalid header)" << std::endl;
        continue;
      }

      igtl::MessageBase::Pointer bodyMsg = self->IgtlMessageFactory->CreateReceiveMessage( headerMsg );
      if ( bodyMsg.IsNull() )
      {
        std::cerr << "Unable to create message of type: " << headerMsg->GetMessageType() << std::endl;
        continue;
      }

      // Only accept tracked frame messages
      if ( typeid( *bodyMsg ) == typeid( igtl::TrackedFrameMessage ) )
      {
        bodyMsg->SetMessageHeader( headerMsg );
        bodyMsg->AllocateBuffer();
        {
          Concurrency::critical_section::scoped_lock lock( self->SocketMutex );
          if ( !self->ClientSocket->GetConnected() )
          {
            // We've become disconnected while waiting for the socket, we're done here!
            return;
          }
          self->ClientSocket->Receive( bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize() );
        }

        int c = bodyMsg->Unpack( 1 );
        if ( !( c & igtl::MessageHeader::UNPACK_BODY ) )
        {
          std::cerr << "Failed to receive reply (invalid body)" << std::endl;
          continue;
        }
        {
          // save command reply
          Concurrency::critical_section::scoped_lock lock( self->Mutex );
          self->Replies.push_back( bodyMsg );
        }
      }
      else
      {
        // if the incoming message is not a reply to a command, we discard it and continue
        {
          Concurrency::critical_section::scoped_lock lock( self->SocketMutex );
          if ( !self->ClientSocket->GetConnected() )
          {
            // We've become disconnected while waiting for the socket, we're done here!
            return;
          }
          self->ClientSocket->Skip( headerMsg->GetBodySizeToRead(), 0 );
        }
      }
    }

    return;
  }

  //----------------------------------------------------------------------------
  CommandReply^ IGTLinkClient::ParseCommandReply( double timeoutSec )
  {

    auto start = std::chrono::high_resolution_clock::now();

    auto reply = ref new CommandReply;
    reply->Result = false;
    reply->Parameters = ref new Platform::Collections::Map<Platform::String^, Platform::String^>;

    while ( 1 )
    {
      {
        // save command reply
        Concurrency::critical_section::scoped_lock lock( Mutex );
        if ( !this->Replies.empty() )
        {
          igtl::MessageBase::Pointer message = this->Replies.front();
          if ( typeid( *message ) == typeid( igtl::RTSCommandMessage ) )
          {
            igtl::RTSCommandMessage::Pointer rtsCommandMsg = dynamic_cast<igtl::RTSCommandMessage*>( message.GetPointer() );

            // TODO : this whole function will need to know about all of the different TUO command message replies in the future

            std::wstring wideStr( rtsCommandMsg->GetCommandName().begin(), rtsCommandMsg->GetCommandName().end() );
            reply->CommandName = ref new Platform::String( wideStr.c_str() );
            reply->OriginalCommandId = rtsCommandMsg->GetCommandId();

            std::wstring wideContentStr( rtsCommandMsg->GetCommandContent().begin(), rtsCommandMsg->GetCommandContent().end() );
            std::transform( wideContentStr.begin(), wideContentStr.end(), wideContentStr.begin(), ::towlower );
            reply->CommandContent = ref new Platform::String( wideContentStr.c_str() );

            WDXD::XmlDocument^ commandDoc = ref new WDXD::XmlDocument;
            commandDoc->LoadXml( reply->CommandContent );
            reply->Result = commandDoc->GetElementsByTagName( L"Result" )->Item( 0 )->NodeValue == L"true";

            for ( unsigned int i = 0; i < commandDoc->GetElementsByTagName( L"Parameter" )->Size; ++i )
            {
              auto entry = commandDoc->GetElementsByTagName( L"Parameter" )->Item( i );
              Platform::String^ name = dynamic_cast<Platform::String^>( entry->Attributes->GetNamedItem( L"Name" )->NodeValue );
              Platform::String^ value = dynamic_cast<Platform::String^>( entry->Attributes->GetNamedItem( L"Value" )->NodeValue );
              reply->Parameters->Insert( name, value );
            }

            for ( std::map< std::string, std::string>::const_iterator it = rtsCommandMsg->GetMetaData().begin(); it != rtsCommandMsg->GetMetaData().end(); ++it )
            {
              std::wstring keyWideStr( it->first.begin(), it->first.end() );
              std::wstring valueWideStr( it->second.begin(), it->second.end() );
              reply->Parameters->Insert( ref new Platform::String( keyWideStr.c_str() ), ref new Platform::String( valueWideStr.c_str() ) );
            }

            this->Replies.pop_front();
            reply->Result = true;
            return reply;
          }
        }
      }

      std::chrono::duration<double, std::milli> timeDiff = std::chrono::high_resolution_clock::now() - start;
      if ( timeDiff.count() > timeoutSec * 1000 )
      {
        return reply;
      }
      std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
    return reply;
  }

  //----------------------------------------------------------------------------
  ImageReply^ IGTLinkClient::ParseImageReply( double timeoutSec )
  {
    auto start = std::chrono::high_resolution_clock::now();

    auto reply = ref new ImageReply;
    reply->Result = false;
    reply->Parameters = ref new Platform::Collections::Map<Platform::String^, Platform::String^>;

    while ( 1 )
    {
      {
        // save command reply
        Concurrency::critical_section::scoped_lock lock( Mutex );
        if ( !this->Replies.empty() )
        {
          igtl::MessageBase::Pointer message = this->Replies.front();
          if ( typeid( *message ) == typeid( igtl::TrackedFrameMessage ) )
          {
            igtl::TrackedFrameMessage::Pointer trackedFrameMsg = dynamic_cast<igtl::TrackedFrameMessage*>( message.GetPointer() );

            for ( std::map< std::string, std::string>::const_iterator it = trackedFrameMsg->GetMetaData().begin(); it != trackedFrameMsg->GetMetaData().end(); ++it )
            {
              std::wstring keyWideStr( it->first.begin(), it->first.end() );
              std::wstring valueWideStr( it->second.begin(), it->second.end() );
              reply->Parameters->Insert( ref new Platform::String( keyWideStr.c_str() ), ref new Platform::String( valueWideStr.c_str() ) );
            }

            reply->ImageSource = FromNativePointer( trackedFrameMsg->GetImage(),
                                                    trackedFrameMsg->GetFrameSize()[0],
                                                    trackedFrameMsg->GetFrameSize()[1],
                                                    trackedFrameMsg->GetNumberOfComponents(),
                                                    trackedFrameMsg->GetImageSizeInBytes()
                                                  );

            this->Replies.pop_front();
            reply->Result = true;
            return reply;
          }
        }

        std::chrono::duration<double, std::milli> timeDiff = std::chrono::high_resolution_clock::now() - start;
        if ( timeDiff.count() > timeoutSec * 1000 )
        {
          return reply;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
      }
    }

    return reply;
  }

  //----------------------------------------------------------------------------
  int IGTLinkClient::SocketReceive( void* data, int length )
  {
    Concurrency::critical_section::scoped_lock lock( SocketMutex );
    return ClientSocket->Receive( data, length );
  }

  //----------------------------------------------------------------------------
  WUXM::Imaging::BitmapSource^ IGTLinkClient::FromNativePointer( unsigned char* pData, int width, int height, int numberOfcomponents, int pDataSize )
  {
    auto imageData = ref new WSS::InMemoryRandomAccessStream();
    auto imageDataWriter = ref new WSS::DataWriter( imageData->GetOutputStreamAt( 0 ) );
    for ( int i = 0; i < pDataSize; ++i )
    {
      imageDataWriter->WriteByte( pData[i] ); // b
      imageDataWriter->WriteByte( pData[i] ); // g
      imageDataWriter->WriteByte( pData[i] ); // r
      imageDataWriter->WriteByte( 255 ); // a
    }

    WUXM::Imaging::WriteableBitmap^ wbm = ref new WUXM::Imaging::WriteableBitmap( width, height );
    concurrency::create_task(imageDataWriter->StoreAsync()).wait();

    wbm->SetSource(imageData);
    wbm->Invalidate();

    return wbm;
  }

  //----------------------------------------------------------------------------
  int IGTLinkClient::ServerPort::get()
  {
    return this->m_ServerPort;
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::ServerPort::set( int arg )
  {
    this->m_ServerPort = arg;
  }

  //----------------------------------------------------------------------------
  Platform::String^ IGTLinkClient::ServerHost::get()
  {
    return this->m_ServerHost;
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::ServerHost::set( Platform::String^ arg )
  {
    this->m_ServerHost = arg;
  }

  //----------------------------------------------------------------------------
  int IGTLinkClient::ServerIGTLVersion::get()
  {
    return this->m_ServerIGTLVersion;
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::ServerIGTLVersion::set( int arg )
  {
    this->m_ServerIGTLVersion = arg;
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::Connected::get()
  {
    return this->ClientSocket->GetConnected();
  }

  //----------------------------------------------------------------------------
  bool ImageReply::Result::get()
  {
    return m_Result;
  }

  //----------------------------------------------------------------------------
  void ImageReply::Result::set( bool arg )
  {
    m_Result = arg;
  }

  //----------------------------------------------------------------------------
  WFC::IMap<Platform::String^, Platform::String^>^ ImageReply::Parameters::get()
  {
    return m_Parameters;
  }

  //----------------------------------------------------------------------------
  void ImageReply::Parameters::set( WFC::IMap<Platform::String^, Platform::String^>^ arg )
  {
    m_Parameters = arg;
  }

  //----------------------------------------------------------------------------
  WUXM::Imaging::BitmapSource^ ImageReply::ImageSource::get()
  {
    return m_ImageSource;
  }

  //----------------------------------------------------------------------------
  void ImageReply::ImageSource::set( WUXM::Imaging::BitmapSource^ arg )
  {
    m_ImageSource = arg;
  }

  //----------------------------------------------------------------------------
  bool CommandReply::Result::get()
  {
    return m_Result;
  }

  //----------------------------------------------------------------------------
  void CommandReply::Result::set( bool arg )
  {
    m_Result = arg;
  }

  //----------------------------------------------------------------------------
  WFC::IMap<Platform::String^, Platform::String^>^ CommandReply::Parameters::get()
  {
    return m_Parameters;
  }

  //----------------------------------------------------------------------------
  void CommandReply::Parameters::set( WFC::IMap<Platform::String^, Platform::String^>^ arg )
  {
    m_Parameters = arg;
  }

  //----------------------------------------------------------------------------
  int32_t CommandReply::OriginalCommandId::get()
  {
    return m_OriginalCommandId;
  }

  //----------------------------------------------------------------------------
  void CommandReply::OriginalCommandId::set( int32_t arg )
  {
    m_OriginalCommandId = arg;
  }

  //----------------------------------------------------------------------------
  Platform::String^ CommandReply::ErrorString::get()
  {
    return m_ErrorString;
  }

  //----------------------------------------------------------------------------
  void CommandReply::ErrorString::set( Platform::String^ arg )
  {
    m_ErrorString = arg;
  }

  //----------------------------------------------------------------------------
  Platform::String^ CommandReply::CommandContent::get()
  {
    return m_CommandContent;
  }

  //----------------------------------------------------------------------------
  void CommandReply::CommandContent::set( Platform::String^ arg )
  {
    m_CommandContent = arg;
  }

  //----------------------------------------------------------------------------
  Platform::String^ CommandReply::CommandName::get()
  {
    return m_CommandName;
  }

  //----------------------------------------------------------------------------
  void CommandReply::CommandName::set( Platform::String^ arg )
  {
    m_CommandName = arg;
  }

}