#ifndef __IGTLinkClient_h
#define __IGTLinkClient_h

#include "ServerExport.h"

#include "igtlClientSocket.h"
#include "igtlCommandMessage.h"
#include "igtlMessageHeader.h"
#include "igtlOSUtil.h"
#include "igtlMessageFactory.h"

#include <deque>
#include <string>

#include <ppltasks.h>
#include <concrt.h>

namespace uwpigtl
{

///
/// \class IGTLinkClient
/// \brief This class provides an OpenIGTLink client. It has basic functionality for sending and receiving messages
///
/// \description It connects to an IGTLink v3+ server, sends requests and receives responses.
///
class ServerExport IGTLinkClient
{
public:
  IGTLinkClient();
  virtual ~IGTLinkClient();

  void SetServerPort( int arg );
  void SetServerHost( const std::string& host );
  void SetServerIGTLVersion( int arg );
  int GetServerIGTLVersion() const;

  /*! If timeoutSec<0 then connection will be attempted multiple times until successfully connected or the timeout elapse */
  bool Connect( double timeoutSec = -1 );

  /*! Disconnect from the connected server */
  bool Disconnect();

  /*! Send a packed message to the connected server */
  bool SendMessage( igtl::MessageBase::Pointer packedMessage );

  /*! Wait for a command reply */
  bool ReceiveReply( bool& result,
                     int32_t& outOriginalCommandId,
                     std::string& outErrorString,
                     std::string& outContent,
                     std::map<std::string,
                     std::string>& outParameters,
                     std::string& outCommandName,
                     double timeoutSec = 0 );

  /*!
    This method can be overridden in child classes to process received messages.
    Note that this method is executed from the data receiver thread and not the
    main thread.
    If the message body is read then this method should return true.
    If the message is not read then this method should return false (and the
    message body will be skipped).
  */
  virtual bool OnMessageReceived( igtl::MessageHeader::Pointer messageHeader );

protected:
  /*! Thread-safe method that allows child classes to read data from the socket */
  int SocketReceive( void* data, int length );

  /*! igtl Factory for message sending */
  igtl::MessageFactory::Pointer IgtlMessageFactory;

private:
  IGTLinkClient( const IGTLinkClient& ) {}
  void operator=( const IGTLinkClient& ) {}

  concurrency::task<void> DataReceiverTask;
  concurrency::cancellation_token_source CancellationTokenSource;

  /*! Mutex instance for safe data access */
  Concurrency::critical_section Mutex;
  Concurrency::critical_section SocketMutex;

  igtl::ClientSocket::Pointer ClientSocket;

  std::deque<igtl::MessageBase::Pointer> Replies;

  int ServerPort;
  std::string ServerHost;

  // IGTL protocol version of the server
  int ServerIGTLVersion;
};

}

#endif