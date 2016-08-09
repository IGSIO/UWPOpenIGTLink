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
#include "igtlStatusMessage.h"

// STD includes
#include <chrono>
#include <regex>

// Windows includes
#include <collection.h>
#include <pplawait.h>
#include <ppltasks.h>
#include <robuffer.h>

using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Platform::Collections;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::Data::Xml::Dom;

namespace UWPOpenIGTLink
{

  const int IGTLinkClient::CLIENT_SOCKET_TIMEOUT_MSEC = 500;
  const uint32 IGTLinkClient::MESSAGE_LIST_MAX_SIZE = 200;

  //----------------------------------------------------------------------------
  IGTLinkClient::IGTLinkClient()
    : m_igtlMessageFactory( igtl::MessageFactory::New() )
    , m_clientSocket( igtl::ClientSocket::New() )
  {
    m_igtlMessageFactory->AddMessageType( "TRACKEDFRAME", ( igtl::MessageFactory::PointerToMessageBaseNew )&igtl::TrackedFrameMessage::New );
    this->ServerHost = L"127.0.0.1";
    this->ServerPort = 18944;
    this->ServerIGTLVersion = IGTL_HEADER_VERSION_2;

    this->m_frameSize.assign( 3, 0 );
  }

  //----------------------------------------------------------------------------
  IGTLinkClient::~IGTLinkClient()
  {
    this->Disconnect();
    auto disconnectTask = concurrency::create_task( [this]()
    {
      while ( this->Connected )
      {
        Sleep( 33 );
      }
    } );
    disconnectTask.wait();
  }

  //----------------------------------------------------------------------------
  IAsyncOperation<bool>^ IGTLinkClient::ConnectAsync( double timeoutSec )
  {
    this->Disconnect();

    this->m_cancellationTokenSource = cancellation_token_source();
    auto token = this->m_cancellationTokenSource.get_token();

    return create_async( [this, timeoutSec, token]() -> bool
    {
      auto connectTask = create_task( [this, timeoutSec, token]() -> bool
      {
        const int retryDelaySec = 1.0;
        int errorCode = 1;
        auto start = std::chrono::high_resolution_clock::now();
        while ( errorCode != 0 )
        {
          std::wstring wideStr( this->ServerHost->Begin() );
          std::string str( wideStr.begin(), wideStr.end() );
          errorCode = this->m_clientSocket->ConnectToServer( str.c_str(), this->ServerPort );
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

        this->m_clientSocket->SetTimeout( CLIENT_SOCKET_TIMEOUT_MSEC );
        return true;
      } );

      // Wait (inside the async operation) and retrieve the result of connection
      bool result = connectTask.get();

      if( result )
      {
        // We're connected, start the data receiver thread
        create_task( [this, token]( void )
        {
          this->DataReceiverPump( this, token );
        } );
      }

      return result;
    } );
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::Disconnect()
  {
    this->m_cancellationTokenSource.cancel();

    {
      critical_section::scoped_lock lock( this->m_socketMutex );
      this->m_clientSocket->CloseSocket();
    }
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::GetOldestTrackedFrame( TrackedFrame^ frame )
  {
    igtl::MessageBase::Pointer igtMessage = nullptr;
    {
      // Retrieve the next available tracked frame reply
      Concurrency::critical_section::scoped_lock lock( m_messageListMutex );
      for ( auto replyIter = m_messages.begin(); replyIter != m_messages.end(); ++replyIter )
      {
        if ( typeid( *( *replyIter ) ) == typeid( igtl::TrackedFrameMessage ) )
        {
          igtMessage = *replyIter;
          break;
        }
      }
    }

    if ( igtMessage != nullptr )
    {
      igtl::TrackedFrameMessage::Pointer trackedFrameMsg = dynamic_cast<igtl::TrackedFrameMessage*>( igtMessage.GetPointer() );

      // Tracking/other related fields
      for ( auto pair : trackedFrameMsg->GetMetaData() )
      {
        std::wstring keyWideStr( pair.first.begin(), pair.first.end() );
        std::wstring valueWideStr( pair.second.begin(), pair.second.end() );
        frame->FrameFields->Insert( ref new Platform::String( keyWideStr.c_str() ), ref new Platform::String( valueWideStr.c_str() ) );
      }

      for ( auto pair : trackedFrameMsg->GetCustomFrameFields() )
      {
        std::wstring keyWideStr( pair.first.begin(), pair.first.end() );
        std::wstring valueWideStr( pair.second.begin(), pair.second.end() );
        frame->FrameFields->Insert( ref new Platform::String( keyWideStr.c_str() ), ref new Platform::String( valueWideStr.c_str() ) );
      }

      // Image related fields
      auto vec = ref new Vector<uint16>;
      vec->Append( trackedFrameMsg->GetFrameSize()[0] );
      vec->Append( trackedFrameMsg->GetFrameSize()[1] );
      vec->Append( trackedFrameMsg->GetFrameSize()[2] );
      frame->FrameSize = vec->GetView();
      frame->ImageSizeBytes = trackedFrameMsg->GetImageSizeInBytes();
      Platform::ArrayReference<byte> arraywrapper( ( byte* )trackedFrameMsg->GetImage(), trackedFrameMsg->GetImageSizeInBytes() );
      frame->ImageData = Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray( arraywrapper );
      frame->NumberOfComponents = trackedFrameMsg->GetNumberOfComponents();
      frame->ScalarType = trackedFrameMsg->GetScalarType();

      return true;
    }

    return false;
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::GetLatestTrackedFrame( TrackedFrame^ frame )
  {
    igtl::MessageBase::Pointer igtMessage = nullptr;
    {
      // Retrieve the next available tracked frame reply
      Concurrency::critical_section::scoped_lock lock(m_messageListMutex);
      for (auto replyIter = m_messages.rbegin(); replyIter != m_messages.rend(); ++replyIter)
      {
        if (typeid(*(*replyIter)) == typeid(igtl::TrackedFrameMessage))
        {
          igtMessage = *replyIter;
          break;
        }
      }
    }

    if (igtMessage != nullptr)
    {
      igtl::TrackedFrameMessage::Pointer trackedFrameMsg = dynamic_cast<igtl::TrackedFrameMessage*>(igtMessage.GetPointer());

      // Tracking/other related fields
      for (auto pair : trackedFrameMsg->GetMetaData())
      {
        std::wstring keyWideStr(pair.first.begin(), pair.first.end());
        std::wstring valueWideStr(pair.second.begin(), pair.second.end());
        frame->FrameFields->Insert(ref new Platform::String(keyWideStr.c_str()), ref new Platform::String(valueWideStr.c_str()));
      }

      for (auto pair : trackedFrameMsg->GetCustomFrameFields())
      {
        std::wstring keyWideStr(pair.first.begin(), pair.first.end());
        std::wstring valueWideStr(pair.second.begin(), pair.second.end());
        frame->FrameFields->Insert(ref new Platform::String(keyWideStr.c_str()), ref new Platform::String(valueWideStr.c_str()));
      }

      // Image related fields
      auto vec = ref new Vector<uint16>;
      vec->Append(trackedFrameMsg->GetFrameSize()[0]);
      vec->Append(trackedFrameMsg->GetFrameSize()[1]);
      vec->Append(trackedFrameMsg->GetFrameSize()[2]);
      frame->FrameSize = vec->GetView();
      frame->ImageSizeBytes = trackedFrameMsg->GetImageSizeInBytes();
      Platform::ArrayReference<byte> arraywrapper((byte*)trackedFrameMsg->GetImage(), trackedFrameMsg->GetImageSizeInBytes());
      frame->ImageData = Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(arraywrapper);
      frame->NumberOfComponents = trackedFrameMsg->GetNumberOfComponents();
      frame->ScalarType = trackedFrameMsg->GetScalarType();

      return true;
    }

    return false;
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::GetOldestCommand(UWPOpenIGTLink::Command^ cmd)
  {
    igtl::MessageBase::Pointer igtMessage = nullptr;
    {
      // Retrieve the next available tracked frame reply
      Concurrency::critical_section::scoped_lock lock(m_messageListMutex);
      for (auto replyIter = m_messages.begin(); replyIter != m_messages.end(); ++replyIter)
      {
        if (typeid(*(*replyIter)) == typeid(igtl::RTSCommandMessage))
        {
          igtMessage = *replyIter;
          break;
        }
      }
    }

    if (igtMessage != nullptr)
    {
      auto cmdMsg = dynamic_cast<igtl::RTSCommandMessage*>(igtMessage.GetPointer());

      cmd->CommandContent = ref new Platform::String(std::wstring(cmdMsg->GetCommandContent().begin(), cmdMsg->GetCommandContent().end()).c_str());
      cmd->CommandName = ref new Platform::String(std::wstring(cmdMsg->GetCommandName().begin(), cmdMsg->GetCommandName().end()).c_str());
      cmd->OriginalCommandId = cmdMsg->GetCommandId();

      XmlDocument^ doc = ref new XmlDocument();
      doc->LoadXml(cmd->CommandContent);

      for (IXmlNode^ node : doc->ChildNodes)
      {
        if (dynamic_cast<Platform::String^>(node->NodeName) == L"Result")
        {
          cmd->Result = (dynamic_cast<Platform::String^>(node->NodeValue) == L"true");
          break;
        }
      }

      if (!cmd->Result)
      {
        bool found(false);
        // Parse for the error string
        for (IXmlNode^ node : doc->ChildNodes)
        {
          if (dynamic_cast<Platform::String^>(node->NodeName) == L"Error")
          {
            cmd->ErrorString = dynamic_cast<Platform::String^>(node->NodeValue);
            found = true;
            break;
          }
        }

        if (!found)
        {
          // TODO : quiet error reporting
        }
      }

      for (auto pair : cmdMsg->GetMetaData())
      {
        std::wstring keyWideStr(pair.first.begin(), pair.first.end());
        std::wstring valueWideStr(pair.second.begin(), pair.second.end());
        cmd->Parameters->Insert(ref new Platform::String(keyWideStr.c_str()), ref new Platform::String(valueWideStr.c_str()));
      }

      return true;
    }

    return false;
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::GetLatestCommand(UWPOpenIGTLink::Command^ cmd)
  {
    igtl::MessageBase::Pointer igtMessage = nullptr;
    {
      // Retrieve the next available tracked frame reply
      Concurrency::critical_section::scoped_lock lock(m_messageListMutex);
      for (auto replyIter = m_messages.rbegin(); replyIter != m_messages.rend(); ++replyIter)
      {
        if (typeid(*(*replyIter)) == typeid(igtl::RTSCommandMessage))
        {
          igtMessage = *replyIter;
          break;
        }
      }
    }

    if (igtMessage != nullptr)
    {
      auto cmdMsg = dynamic_cast<igtl::RTSCommandMessage*>(igtMessage.GetPointer());

      cmd->CommandContent = ref new Platform::String(std::wstring(cmdMsg->GetCommandContent().begin(), cmdMsg->GetCommandContent().end()).c_str());
      cmd->CommandName = ref new Platform::String(std::wstring(cmdMsg->GetCommandName().begin(), cmdMsg->GetCommandName().end()).c_str());
      cmd->OriginalCommandId = cmdMsg->GetCommandId();

      XmlDocument^ doc = ref new XmlDocument();
      doc->LoadXml(cmd->CommandContent);

      for (IXmlNode^ node : doc->ChildNodes)
      {
        if (dynamic_cast<Platform::String^>(node->NodeName) == L"Result")
        {
          cmd->Result = (dynamic_cast<Platform::String^>(node->NodeValue) == L"true");
          break;
        }
      }

      if (!cmd->Result)
      {
        bool found(false);
        // Parse for the error string
        for (IXmlNode^ node : doc->ChildNodes)
        {
          if (dynamic_cast<Platform::String^>(node->NodeName) == L"Error")
          {
            cmd->ErrorString = dynamic_cast<Platform::String^>(node->NodeValue);
            found = true;
            break;
          }
        }

        if (!found)
        {
          // TODO : quiet error reporting
        }
      }

      for (auto pair : cmdMsg->GetMetaData())
      {
        std::wstring keyWideStr(pair.first.begin(), pair.first.end());
        std::wstring valueWideStr(pair.second.begin(), pair.second.end());
        cmd->Parameters->Insert(ref new Platform::String(keyWideStr.c_str()), ref new Platform::String(valueWideStr.c_str()));
      }

      return true;
    }

    return false;
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::SendMessage( igtl::MessageBase::Pointer packedMessage )
  {
    int success = 0;
    {
      critical_section::scoped_lock lock( m_socketMutex );
      success = this->m_clientSocket->Send( packedMessage->GetBufferPointer(), packedMessage->GetBufferSize() );
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
      auto headerMsg = self->m_igtlMessageFactory->CreateHeaderMessage( IGTL_HEADER_VERSION_1 );

      // Receive generic header from the socket
      int numOfBytesReceived = 0;
      {
        critical_section::scoped_lock lock( self->m_socketMutex );
        if ( !self->m_clientSocket->GetConnected() )
        {
          // We've become disconnected while waiting for the socket, we're done here!
          return;
        }
        numOfBytesReceived = self->m_clientSocket->Receive( headerMsg->GetBufferPointer(), headerMsg->GetBufferSize() );
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

      auto bodyMsg = self->m_igtlMessageFactory->CreateReceiveMessage( headerMsg );
      if ( bodyMsg.IsNull() )
      {
        std::cerr << "Unable to create message of type: " << headerMsg->GetMessageType() << std::endl;
        continue;
      }

      // Accept all messages but status messages, they are used as a keep alive mechanism
      if ( typeid( *bodyMsg ) != typeid( igtl::StatusMessage ) )
      {
        bodyMsg->SetMessageHeader( headerMsg );
        bodyMsg->AllocateBuffer();
        {
          critical_section::scoped_lock lock( self->m_socketMutex );
          if ( !self->m_clientSocket->GetConnected() )
          {
            // We've become disconnected while waiting for the socket, we're done here!
            return;
          }
          self->m_clientSocket->Receive( bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize() );
        }

        int c = bodyMsg->Unpack( 1 );
        if ( !( c & igtl::MessageHeader::UNPACK_BODY ) )
        {
          std::cerr << "Failed to receive reply (invalid body)" << std::endl;
          continue;
        }

        {
          // save reply
          critical_section::scoped_lock lock( self->m_messageListMutex );

          self->m_messages.push_back( bodyMsg );
        }
      }
      else
      {
        critical_section::scoped_lock lock( self->m_socketMutex );

        if ( !self->m_clientSocket->GetConnected() )
        {
          // We've become disconnected while waiting for the socket, we're done here!
          return;
        }
        self->m_clientSocket->Skip( headerMsg->GetBodySizeToRead(), 0 );
      }

      if ( self->m_messages.size() > MESSAGE_LIST_MAX_SIZE )
      {
        critical_section::scoped_lock lock( self->m_messageListMutex );

        // erase the front N results
        uint32 toErase = self->m_messages.size() - MESSAGE_LIST_MAX_SIZE;
        self->m_messages.erase( self->m_messages.begin(), self->m_messages.begin() + toErase );
      }
    }

    return;
  }

  //----------------------------------------------------------------------------
  int IGTLinkClient::SocketReceive( void* data, int length )
  {
    critical_section::scoped_lock lock( m_socketMutex );
    return m_clientSocket->Receive( data, length );
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
    return this->m_clientSocket->GetConnected();
  }
}