#include "igtlCommandMessage.h"
#include "igtlCommon.h"
#include "igtlMessageHeader.h"
#include "igtlOSUtil.h"
#include "igtlServerSocket.h"
#include "igtlTrackingDataMessage.h"
#include "IGTLinkClient.h"
#include <chrono>

static const int CLIENT_SOCKET_TIMEOUT_MSEC = 500;

namespace uwpigtl
{

//----------------------------------------------------------------------------
IGTLinkClient::IGTLinkClient()
  : IgtlMessageFactory( igtl::MessageFactory::New() )
  , ClientSocket( igtl::ClientSocket::New() )
  , ServerHost( NULL )
  , ServerPort( -1 )
  , ServerIGTLVersion( IGTL_HEADER_VERSION_1 )
{

}

//----------------------------------------------------------------------------
IGTLinkClient::~IGTLinkClient()
{
}

//----------------------------------------------------------------------------
void IGTLinkClient::SetServerPort( int arg )
{
  this->ServerPort = arg;
}

//----------------------------------------------------------------------------
void IGTLinkClient::SetServerHost( const std::string& host )
{
  this->ServerHost = host;
}

//----------------------------------------------------------------------------
void IGTLinkClient::SetServerIGTLVersion( int arg )
{
  this->ServerIGTLVersion = arg;
}

//----------------------------------------------------------------------------
int IGTLinkClient::GetServerIGTLVersion() const
{
  return this->ServerIGTLVersion;
}

//----------------------------------------------------------------------------
bool IGTLinkClient::Connect( double timeoutSec/*=-1*/ )
{
  const int retryDelaySec = 1.0;
  int errorCode = 1;
  auto start = std::chrono::system_clock::now();
  while ( errorCode != 0 )
  {
    errorCode = this->ClientSocket->ConnectToServer( this->ServerHost.c_str(), this->ServerPort );
    if ( ( std::chrono::system_clock::now() - start ).count() > timeoutSec )
    {
      // time is up
      break;
    }
    std::this_thread::sleep_for( std::chrono::seconds( retryDelaySec ) );
  }

  if ( errorCode != 0 )
  {
    std::cerr << "Cannot connect to the server." << std::endl;
    return false;
  }
  std::cout << "Client successfully connected to server." << std::endl;

  this->ClientSocket->SetTimeout( CLIENT_SOCKET_TIMEOUT_MSEC );

  auto token = this->CancellationTokenSource.get_token();

  this->DataReceiverTask = concurrency::create_task( [self = this, cancelToken = token]
  {
    while ( !cancelToken.is_canceled() )
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

      if ( self->OnMessageReceived( headerMsg.GetPointer() ) )
      {
        // The message body is read and processed
        continue;
      }

      igtl::MessageBase::Pointer bodyMsg = self->IgtlMessageFactory->CreateReceiveMessage( headerMsg );
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
          Concurrency::critical_section::scoped_lock lock( self->SocketMutex );
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
        std::cout << "Received message: " << headerMsg->GetMessageType() << " (not processed)" << std::endl;
        {
          Concurrency::critical_section::scoped_lock lock( self->SocketMutex );
          self->ClientSocket->Skip( headerMsg->GetBodySizeToRead(), 0 );
        }
      }
    }

    // Close thread
    return;
  }, token );

  return true;
}

//----------------------------------------------------------------------------
bool IGTLinkClient::Disconnect()
{
  {
    Concurrency::critical_section::scoped_lock lock( SocketMutex );
    this->ClientSocket->CloseSocket();
  }

  this->CancellationTokenSource.cancel();
  this->DataReceiverTask.wait();

  return true;
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
bool IGTLinkClient::ReceiveReply( bool& result, int32_t& outOriginalCommandId, std::string& outErrorString,
                                  std::string& outContent, std::map<std::string, std::string>& outParameters,
                                  std::string& outCommandName, double timeoutSec/*=0*/ )
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

          //vtkSmartPointer<vtkXMLDataElement> cmdElement = vtkSmartPointer<vtkXMLDataElement>::Take( vtkXMLUtilities::ReadElementFromString( rtsCommandMsg->GetCommandContent().c_str() ) );

          outCommandName = rtsCommandMsg->GetCommandName();
          outOriginalCommandId = rtsCommandMsg->GetCommandId();

          //XML_FIND_NESTED_ELEMENT_OPTIONAL( resultElement, cmdElement, "Result" );
          //if( resultElement != NULL )
          //{
          //result = STRCASECMP( resultElement->GetCharacterData(), "true" ) == 0 ? PLUS_SUCCESS : PLUS_FAIL;
          //}
          //XML_FIND_NESTED_ELEMENT_OPTIONAL( errorElement, cmdElement, "Error" );
          //if( !result && errorElement == NULL )
          //{
          //LOG_ERROR( "Server sent error without reason. Notify server developers." );
          //}
          //else if( !result && errorElement != NULL )
          //{
          //outErrorString = errorElement->GetCharacterData();
          //}
          //XML_FIND_NESTED_ELEMENT_REQUIRED( messageElement, cmdElement, "Message" );
          //outContent = messageElement->GetCharacterData();

          outParameters = rtsCommandMsg->GetMetaData();
        }
        else if ( typeid( *message ) == typeid( igtl::RTSTrackingDataMessage ) )
        {
          igtl::RTSTrackingDataMessage* rtsTrackingMsg = dynamic_cast<igtl::RTSTrackingDataMessage*>( message.GetPointer() );

          result = rtsTrackingMsg->GetStatus() == 0 ? true : false;
          outContent = ( rtsTrackingMsg->GetStatus() == 0 ? "SUCCESS" : "FAILURE" );
          outCommandName = "RTSTrackingDataMessage";
          outOriginalCommandId = -1;
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
bool IGTLinkClient::OnMessageReceived( igtl::MessageHeader::Pointer messageHeader )
{
  return true;
}

//----------------------------------------------------------------------------
int IGTLinkClient::SocketReceive( void* data, int length )
{
  Concurrency::critical_section::scoped_lock lock( SocketMutex );
  return ClientSocket->Receive( data, length );
}

}