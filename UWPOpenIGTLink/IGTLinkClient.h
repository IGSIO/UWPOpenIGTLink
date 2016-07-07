#ifndef __IGTLinkClient_h
#define __IGTLinkClient_h

#include "igtlClientSocket.h"
#include "igtlMessageHeader.h"
#include "igtlMessageFactory.h"

#include <deque>
#include <string>

#include <ppltasks.h>
#include <concrt.h>
#include <vccorlib.h>

namespace WFC = Windows::Foundation::Collections;
namespace WFM = Windows::Foundation::Metadata;

namespace UWPOpenIGTLink
{

  ///
  /// \class IGTLinkClient
  /// \brief This class provides an OpenIGTLink client. It has basic functionality for sending and receiving messages
  ///
  /// \description It connects to an IGTLink v3+ server, sends requests and receives responses.
  ///
  public ref class IGTLinkClient sealed
  {
  public:
    IGTLinkClient();

    property int ServerPort
    {
      int get();
      void set( int );
    }
    property Platform::String^ ServerHost
    {
      Platform::String ^ get();
      void set( Platform::String^ );
    }
    property int ServerIGTLVersion
    {
      int get();
      void set( int );
    }
    property bool Connected
    {
      bool get();
      void set(bool);
    }

    /*! If timeoutSec<0 then connection will be attempted multiple times until successfully connected or the timeout elapse */
    Windows::Foundation::IAsyncOperation<Platform::String^>^ ConnectAsync( double timeoutSec );

    /*! Disconnect from the connected server */
    Windows::Foundation::IAsyncOperation<bool>^ DisconnectAsync();

    /*! Wait for a command reply */
    bool ReceiveReply( bool* result,
                       int32_t* outOriginalCommandId,
                       Platform::String^ outErrorString,
                       Platform::String^ outContent,
                       Platform::String^ outCommandContent,
                       WFC::IMap<Platform::String^, Platform::String^>^ outParameters,
                       Platform::String^ outCommandName,
                       double timeoutSec );

    virtual ~IGTLinkClient();

  internal:
    /*! Send a packed message to the connected server */
    bool SendMessage( igtl::MessageBase::Pointer packedMessage );

  protected private:
    /*! Thread-safe method that allows child classes to read data from the socket */
    int SocketReceive( void* data, int length );

    /*! igtl Factory for message sending */
    igtl::MessageFactory::Pointer IgtlMessageFactory;

    concurrency::task<void> DataReceiverTask;
    concurrency::cancellation_token_source CancellationTokenSource;

    /*! Mutex instance for safe data access */
    Concurrency::critical_section Mutex;
    Concurrency::critical_section SocketMutex;

    igtl::ClientSocket::Pointer ClientSocket;

    std::deque<igtl::MessageBase::Pointer> Replies;

    Platform::String^ m_ServerHost;
    int m_ServerPort;
    int m_ServerIGTLVersion;

    bool m_Connected;

  private:
    IGTLinkClient( IGTLinkClient^ ) {}
    void operator=( IGTLinkClient^ ) {}
  };

}

#endif