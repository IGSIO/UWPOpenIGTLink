/*====================================================================
Copyright(c) 2016 Adam Rankin


Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files(the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and / or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
====================================================================*/

// Local includes
#include "IGTLinkClient.h"
#include "TrackedFrameMessage.h"

// IGT includes
#include "igtlCommandMessage.h"
#include "igtlCommon.h"
#include "igtlMessageHeader.h"
#include "igtlOSUtil.h"
#include "igtlServerSocket.h"
#include "igtlTrackingDataMessage.h"

// STD includes
#include <chrono>
#include <regex>

// Windows includes
#include <collection.h>
#include <pplawait.h>
#include <ppltasks.h>
#include <robuffer.h>

namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WSS = Windows::Storage::Streams;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;
namespace WDXD = Windows::Data::Xml::Dom;

namespace
{
  inline void ThrowIfFailed( HRESULT hr )
  {
    if ( FAILED( hr ) )
    {
      throw Platform::Exception::CreateException( hr );
    }
  }

  byte* GetPointerToPixelData( WSS::IBuffer^ buffer )
  {
    // Cast to Object^, then to its underlying IInspectable interface.
    Platform::Object^ obj = buffer;
    Microsoft::WRL::ComPtr<IInspectable> insp( reinterpret_cast<IInspectable*>( obj ) );

    // Query the IBufferByteAccess interface.
    Microsoft::WRL::ComPtr<WSS::IBufferByteAccess> bufferByteAccess;
    ThrowIfFailed( insp.As( &bufferByteAccess ) );

    // Retrieve the buffer data.
    byte* pixels = nullptr;
    ThrowIfFailed( bufferByteAccess->Buffer( &pixels ) );
    return pixels;
  }
}

namespace UWPOpenIGTLink
{

  const int IGTLinkClient::CLIENT_SOCKET_TIMEOUT_MSEC = 500;

  //----------------------------------------------------------------------------
  IGTLinkClient::IGTLinkClient()
    : IgtlMessageFactory( igtl::MessageFactory::New() )
    , ClientSocket( igtl::ClientSocket::New() )
  {
    IgtlMessageFactory->AddMessageType( "TRACKEDFRAME", ( igtl::MessageFactory::PointerToMessageBaseNew )&igtl::TrackedFrameMessage::New );
    this->ServerHost = L"127.0.0.1";
    this->ServerPort = 18944;
    this->ServerIGTLVersion = IGTL_HEADER_VERSION_2;

    this->FrameSize.push_back( 0 );
    this->FrameSize.push_back( 0 );
    this->FrameSize.push_back( 0 );
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
    this->CancellationTokenSource = concurrency::cancellation_token_source();
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
  bool IGTLinkClient::ParseCommandReply( CommandReply^ reply )
  {
    reply->Result = false;
    reply->Parameters = ref new Platform::Collections::Map<Platform::String^, Platform::String^>;

    igtl::MessageBase::Pointer message = nullptr;
    {
      // Retrieve the next available command reply
      Concurrency::critical_section::scoped_lock lock( Mutex );
      for ( auto replyIter = Replies.begin(); replyIter != Replies.end(); ++replyIter )
      {
        if ( typeid( *( *replyIter ) ) == typeid( igtl::RTSCommandMessage ) )
        {
          message = *replyIter;
          Replies.erase( replyIter );
          break;
        }
      }
    }

    if( message != nullptr )
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

      reply->Result = true;
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::ParseTrackedFrameReply( TrackedFrameReply^ reply )
  {
    reply->Result = false;
    reply->Parameters = ref new Platform::Collections::Map<Platform::String^, Platform::String^>;
    reply->ImageSource = this->WriteableBitmap;

    igtl::MessageBase::Pointer message = nullptr;
    {
      // Retrieve the next available tracked frame reply
      Concurrency::critical_section::scoped_lock lock( Mutex );
      for ( auto replyIter = Replies.begin(); replyIter != Replies.end(); ++replyIter )
      {
        if ( typeid( *( *replyIter ) ) == typeid( igtl::TrackedFrameMessage ) )
        {
          message = *replyIter;
          Replies.erase( replyIter );
          break;
        }
      }
    }

    if ( message != nullptr )
    {
      igtl::TrackedFrameMessage::Pointer trackedFrameMsg = dynamic_cast<igtl::TrackedFrameMessage*>( message.GetPointer() );

      for ( std::map< std::string, std::string>::const_iterator it = trackedFrameMsg->GetMetaData().begin(); it != trackedFrameMsg->GetMetaData().end(); ++it )
      {
        std::wstring keyWideStr( it->first.begin(), it->first.end() );
        std::wstring valueWideStr( it->second.begin(), it->second.end() );
        reply->Parameters->Insert( ref new Platform::String( keyWideStr.c_str() ), ref new Platform::String( valueWideStr.c_str() ) );
      }

      for ( std::map<std::string, std::string>::const_iterator it = trackedFrameMsg->GetCustomFrameFields().begin(); it != trackedFrameMsg->GetCustomFrameFields().end(); ++it )
      {
        std::wstring keyWideStr( it->first.begin(), it->first.end() );
        std::wstring valueWideStr( it->second.begin(), it->second.end() );
        reply->Parameters->Insert( ref new Platform::String( keyWideStr.c_str() ), ref new Platform::String( valueWideStr.c_str() ) );
      }

      if ( trackedFrameMsg->GetFrameSize()[0] != this->FrameSize[0] ||
           trackedFrameMsg->GetFrameSize()[1] != this->FrameSize[1] ||
           trackedFrameMsg->GetFrameSize()[2] != this->FrameSize[2] )
      {
        this->FrameSize.clear();
        this->FrameSize.push_back( trackedFrameMsg->GetFrameSize()[0] );
        this->FrameSize.push_back( trackedFrameMsg->GetFrameSize()[1] );
        this->FrameSize.push_back( trackedFrameMsg->GetFrameSize()[2] );

        // Reallocate a new image
        this->WriteableBitmap = ref new WUXM::Imaging::WriteableBitmap( FrameSize[0], FrameSize[1] );
      }

      FromNativePointer( trackedFrameMsg->GetImage(),
                         trackedFrameMsg->GetFrameSize()[0],
                         trackedFrameMsg->GetFrameSize()[1],
                         trackedFrameMsg->GetNumberOfComponents(),
                         this->WriteableBitmap );

      this->WriteableBitmap->Invalidate();
      reply->Result = true;
      return true;
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
  bool IGTLinkClient::FromNativePointer( unsigned char* pData, int width, int height, int numberOfcomponents, WUXM::Imaging::WriteableBitmap^ wbm )
  {
    byte* pPixels = ::GetPointerToPixelData( wbm->PixelBuffer );

    int i( 0 );
    for ( int y = 0; y < height; y++ )
    {
      for ( int x = 0; x < width; x++ )
      {
        pPixels[( x + y * width ) * 4] = pData[i]; // B
        pPixels[( x + y * width ) * 4 + 1] = pData[i]; // G
        pPixels[( x + y * width ) * 4 + 2] = pData[i]; // R
        pPixels[( x + y * width ) * 4 + 3] = 255; // A

        i++;
      }
    }

    return true;
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
}