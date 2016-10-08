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

#pragma once

// Local includes
#include "Command.h"
#include "IGTCommon.h"
#include "TrackedFrame.h"
#include "TrackedFrameMessage.h"

// IGT includes
#include <igtlClientSocket.h>
#include <igtlMessageHeader.h>
#include <igtlMessageFactory.h>

// std includes
#include <deque>
#include <string>

// Windows includes
#include <ppltasks.h>
#include <concrt.h>
#include <vccorlib.h>
#include <collection.h>

using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
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
    virtual ~IGTLinkClient();

    property int ServerPort {int get(); void set(int); }
    property Platform::String^ ServerHost { Platform::String ^ get(); void set(Platform::String^); }
    property int ServerIGTLVersion { int get(); void set(int); }
    property bool Connected { bool get(); }

    /// If timeoutSec > 0 then connection will be attempted multiple times until successfully connected or the timeout elapse
    IAsyncOperation<bool>^ ConnectAsync(double timeoutSec);

    /// Disconnect from the connected server
    void Disconnect();

    /// Retrieve the latest tracked frame reply
    bool GetLatestTrackedFrame(TrackedFrame^ frame, double* latestTimestamp);

    /// Retrieve the latest command
    bool GetLatestCommand(UWPOpenIGTLink::Command^ cmd, double* latestTimestamp);

    /// Send a message to the connected server
    bool SendMessage(MessageBasePointerPtr messageBasePointerAsIntPtr);

  internal:
    /// Send a packed message to the connected server
    bool SendMessage(igtl::MessageBase::Pointer packedMessage);

    /// Threaded function to receive data from the connected server
    static void DataReceiverPump(IGTLinkClient^ self, concurrency::cancellation_token token);

  protected private:
    /// Thread-safe method that allows child classes to read data from the socket
    int SocketReceive(void* data, int length);

  protected private:
    /// igtl Factory for message sending
    igtl::MessageFactory::Pointer m_igtlMessageFactory;

    task<void> m_dataReceiverTask;
    cancellation_token_source m_cancellationTokenSource;

    /// Mutex instance for safe data access
    critical_section m_messageListMutex;
    critical_section m_socketMutex;

    /// Socket that is connected to the server
    igtl::ClientSocket::Pointer m_clientSocket;

    /// List of messages received through the socket, transformed to igtl messages
    std::deque<igtl::MessageBase::Pointer> m_receiveMessages;
    std::vector<igtl::MessageBase::Pointer> m_sendMessages;

    /// Stored WriteableBitmap to reduce overhead of memory reallocation unless necessary
    Imaging::WriteableBitmap^ m_writeableBitmap;
    std::vector<uint32> m_frameSize;

    /// Server information
    Platform::String^ m_ServerHost;
    int m_ServerPort;
    int m_ServerIGTLVersion;

    static const int CLIENT_SOCKET_TIMEOUT_MSEC;
    static const uint32 MESSAGE_LIST_MAX_SIZE;

  private:
    IGTLinkClient(IGTLinkClient^) {}
    void operator=(IGTLinkClient^) {}
  };
}