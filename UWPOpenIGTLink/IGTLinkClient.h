#pragma once

#include "igtlClientSocket.h"
#include "igtlMessageHeader.h"
#include "igtlMessageFactory.h"

#include <deque>
#include <string>

#include <ppltasks.h>
#include <concrt.h>
#include <vccorlib.h>

namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
  [Windows::Foundation::Metadata::WebHostHiddenAttribute]
  public ref class ImageReply sealed
  {
  public:
    property bool Result {bool get(); void set( bool arg );}
    property WFC::IMap<Platform::String^, Platform::String^>^ Parameters {WFC::IMap<Platform::String^, Platform::String^>^ get(); void set( WFC::IMap<Platform::String^, Platform::String^>^ arg );}
    property WUXM::Imaging::WriteableBitmap^ ImageSource {WUXM::Imaging::WriteableBitmap ^ get(); void set( WUXM::Imaging::WriteableBitmap ^ arg );}

    WFC::IMapView<Platform::String^, Platform::String^>^ GetValidTransforms();

  protected private:
    bool m_Result;
    WFC::IMap<Platform::String^, Platform::String^>^ m_Parameters;
    WUXM::Imaging::WriteableBitmap^ m_ImageSource;
  };

  [Windows::Foundation::Metadata::WebHostHiddenAttribute]
  public ref class CommandReply sealed
  {
  public:
    property bool Result {bool get(); void set( bool arg ); }
    property WFC::IMap<Platform::String^, Platform::String^>^ Parameters {WFC::IMap<Platform::String^, Platform::String^>^ get(); void set( WFC::IMap<Platform::String^, Platform::String^>^ arg ); }
    property int32_t OriginalCommandId {int32_t get(); void set( int32_t arg ); }
    property Platform::String^ ErrorString {Platform::String ^ get(); void set( Platform::String ^ arg ); }
    property Platform::String^ CommandContent {Platform::String ^ get(); void set( Platform::String ^ arg ); }
    property Platform::String^ CommandName {Platform::String ^ get(); void set( Platform::String ^ arg ); }

  protected private:
    bool m_Result;
    WFC::IMap<Platform::String^, Platform::String^>^ m_Parameters;
    int32_t m_OriginalCommandId;
    Platform::String^ m_ErrorString;
    Platform::String^ m_CommandContent;
    Platform::String^ m_CommandName;
  };


  ///
  /// \class IGTLinkClient
  /// \brief This class provides an OpenIGTLink client. It has basic functionality for sending and receiving messages
  ///
  /// \description It connects to an IGTLink v3+ server, sends requests and receives responses.
  ///
  [Windows::Foundation::Metadata::WebHostHiddenAttribute]
  public ref class IGTLinkClient sealed
  {
  public:
    IGTLinkClient();

    property int ServerPort {int get(); void set( int ); }
    property Platform::String^ ServerHost { Platform::String ^ get(); void set( Platform::String^ ); }
    property int ServerIGTLVersion { int get(); void set( int ); }
    property bool Connected { bool get(); }

    /// If timeoutSec<0 then connection will be attempted multiple times until successfully connected or the timeout elapse
    WF::IAsyncOperation<bool>^ ConnectAsync( double timeoutSec );

    /// Disconnect from the connected server
    WF::IAsyncAction^ DisconnectAsync();

    virtual ~IGTLinkClient();

    /// Retrieve a command reply from the queue of replies and clear it
    CommandReply^ ParseCommandReply( double timeoutSec );

    /// Retrieve an image reply from the queue of replies and clear it
    ImageReply^ ParseImageReply( double timeoutSec );

  internal:
    /// Send a packed message to the connected server
    bool SendMessage( igtl::MessageBase::Pointer packedMessage );

    /// Threaded function to receive data from the connected server
    static void DataReceiverPump( IGTLinkClient^ self, concurrency::cancellation_token token );

  protected private:
    /// Thread-safe method that allows child classes to read data from the socket
    int SocketReceive( void* data, int length );

    /// Convert a c-style byte array to a managed image object
    bool FromNativePointer( unsigned char* pData, int width, int height, int numberOfcomponents, WUXM::Imaging::WriteableBitmap^ wbm );

    /// igtl Factory for message sending
    igtl::MessageFactory::Pointer IgtlMessageFactory;

    concurrency::task<void> DataReceiverTask;
    concurrency::cancellation_token_source CancellationTokenSource;

    /// Mutex instance for safe data access
    Concurrency::critical_section Mutex;
    Concurrency::critical_section SocketMutex;

    /// Socket that is connected to the server
    igtl::ClientSocket::Pointer ClientSocket;

    /// List of replies received through the socket, transformed to igtl messages
    std::deque<igtl::MessageBase::Pointer> Replies;

    /// Stored WriteableBitmap to reduce overhead of memory reallocation unless necessary
    WUXM::Imaging::WriteableBitmap^ WriteableBitmap;
    std::vector<int> FrameSize;

    /// Server information
    Platform::String^ m_ServerHost;
    int m_ServerPort;
    int m_ServerIGTLVersion;

  private:
    IGTLinkClient( IGTLinkClient^ ) {}
    void operator=( IGTLinkClient^ ) {}
  };

}