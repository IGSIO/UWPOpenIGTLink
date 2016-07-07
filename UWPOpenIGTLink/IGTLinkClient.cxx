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
#include <ppltasks.h>

static const int CLIENT_SOCKET_TIMEOUT_MSEC = 500;

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
  }

  //----------------------------------------------------------------------------
  Windows::Foundation::IAsyncOperation<Platform::String^>^ IGTLinkClient::ConnectAsync( double timeoutSec )
  {
    auto token = this->CancellationTokenSource.get_token();

    return concurrency::create_async( [this, timeoutSec, token]() -> Platform::String^
    {
      auto connectTask = concurrency::create_task( [this, timeoutSec, token]() -> Platform::String^
      {
        const int retryDelaySec = 1.0;
        int errorCode = 1;
        auto start = std::chrono::system_clock::now();
        while ( errorCode != 0 )
        {
          std::wstring wideStr( this->ServerHost->Begin() );
          std::string str( wideStr.begin(), wideStr.end() );
          errorCode = this->ClientSocket->ConnectToServer( str.c_str(), this->ServerPort );
          if ( ( std::chrono::system_clock::now() - start ).count() > timeoutSec )
          {
            // time is up
            break;
          }
          std::this_thread::sleep_for( std::chrono::seconds( retryDelaySec ) );
        }

        if ( errorCode != 0 )
        {
          this->Connected = false;
          return ref new Platform::String( L"Unable to connect." );
        }

        this->ClientSocket->SetTimeout( CLIENT_SOCKET_TIMEOUT_MSEC );
        this->Connected = true;
        return ref new Platform::String( L"Success! Connected to " ) + this->ServerHost + ref new Platform::String( L":" ) + ref new Platform::String( std::to_wstring( this->ServerPort ).c_str() );
      } );
      Platform::String^ result = connectTask.get();

      if ( !this->Connected )
      {
        return result;
      }
      else
      {
        concurrency::create_task( [this, token]( void )
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
              Concurrency::critical_section::scoped_lock lock( this->SocketMutex );
              numOfBytesReceived = this->ClientSocket->Receive( headerMsg->GetBufferPointer(), headerMsg->GetBufferSize() );
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

            igtl::MessageBase::Pointer bodyMsg = this->IgtlMessageFactory->CreateReceiveMessage( headerMsg );
            if ( bodyMsg.IsNull() )
            {
              std::cerr << "Unable to create message of type: " << headerMsg->GetMessageType() << std::endl;
              continue;
            }

            // Only accept string messages if they have a deviceName of the format ACK_xyz
            if ( typeid( *bodyMsg ) == typeid( igtl::RTSTrackingDataMessage ) )
            {
              bodyMsg->SetMessageHeader( headerMsg );
              bodyMsg->AllocateBuffer();
              {
                Concurrency::critical_section::scoped_lock lock( this->SocketMutex );
                this->ClientSocket->Receive( bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize() );
              }

              int c = bodyMsg->Unpack( 1 );
              if ( !( c & igtl::MessageHeader::UNPACK_BODY ) )
              {
                std::cerr << "Failed to receive reply (invalid body)" << std::endl;
                continue;
              }
              {
                // save command reply
                Concurrency::critical_section::scoped_lock lock( this->Mutex );
                this->Replies.push_back( bodyMsg );
              }
            }
            else
            {
              // if the incoming message is not a reply to a command, we discard it and continue
              std::cout << "Received message: " << headerMsg->GetMessageType() << " (not processed)" << std::endl;
              {
                Concurrency::critical_section::scoped_lock lock( this->SocketMutex );
                this->ClientSocket->Skip( headerMsg->GetBodySizeToRead(), 0 );
              }
            }
          }

          return;
        } );
      }

      return result;
    } );
  }

  //----------------------------------------------------------------------------
  Windows::Foundation::IAsyncOperation<bool>^ IGTLinkClient::DisconnectAsync()
  {
    return concurrency::create_async( [this]() -> bool
    {
      {
        Concurrency::critical_section::scoped_lock lock( this->SocketMutex );
        this->ClientSocket->CloseSocket();
      }

      this->CancellationTokenSource.cancel();
      this->DataReceiverTask.wait();

      return true;
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
  bool IGTLinkClient::ReceiveReply( bool* result,
                                    int32_t* outOriginalCommandId,
                                    Platform::String^ outErrorString,
                                    Platform::String^ outContent,
                                    Platform::String^ outCommandContent,
                                    Windows::Foundation::Collections::IMap<Platform::String^, Platform::String^>^ outParameters,
                                    Platform::String^ outCommandName,
                                    double timeoutSec )
  {
    auto start = std::chrono::system_clock::now();

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
            // Process the command as v3 RTS_Command
            igtl::RTSCommandMessage::Pointer rtsCommandMsg = dynamic_cast<igtl::RTSCommandMessage*>( message.GetPointer() );

            std::wstring wideStr( rtsCommandMsg->GetCommandName().begin(), rtsCommandMsg->GetCommandName().end() );
            outCommandName = ref new Platform::String( wideStr.c_str() );
            *outOriginalCommandId = rtsCommandMsg->GetCommandId();

            std::wstring wideContentStr( rtsCommandMsg->GetCommandContent().begin(), rtsCommandMsg->GetCommandContent().end() );
            outCommandContent = ref new Platform::String( wideContentStr.c_str() );

            for ( std::map< std::string, std::string>::const_iterator it = rtsCommandMsg->GetMetaData().begin(); it != rtsCommandMsg->GetMetaData().end(); ++it )
            {
              std::wstring keyWideStr( it->first.begin(), it->first.end() );
              std::wstring valueWideStr( it->second.begin(), it->second.end() );
              outParameters->Insert( ref new Platform::String( keyWideStr.c_str() ), ref new Platform::String( valueWideStr.c_str() ) );
            }
          }
          else if ( typeid( *message ) == typeid( igtl::RTSTrackingDataMessage ) )
          {
            igtl::RTSTrackingDataMessage* rtsTrackingMsg = dynamic_cast<igtl::RTSTrackingDataMessage*>( message.GetPointer() );

            *result = rtsTrackingMsg->GetStatus() == 0 ? true : false;
            outContent = ( rtsTrackingMsg->GetStatus() == 0 ? L"SUCCESS" : L"FAILURE" );
            outCommandName = L"RTSTrackingDataMessage";
            *outOriginalCommandId = -1;
          }
          else if ( typeid( *message ) == typeid( igtl::TrackedFrameMessage ) )
          {

          }

          this->Replies.pop_front();
          return true;
        }
      }

      if ( ( std::chrono::system_clock::now() - start ).count() > timeoutSec )
      {
        std::cerr << "vtkPlusOpenIGTLinkClient::ReceiveReply timeout passed (" << timeoutSec << "sec)" << std::endl;
        return false;
      }
      std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
    return false;
  }

  //----------------------------------------------------------------------------
  int IGTLinkClient::SocketReceive( void* data, int length )
  {
    Concurrency::critical_section::scoped_lock lock( SocketMutex );
    return ClientSocket->Receive( data, length );
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
    return m_Connected;
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::Connected::set( bool arg )
  {
    m_Connected = arg;
  }

}