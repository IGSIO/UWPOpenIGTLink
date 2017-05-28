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
#include <igtlMessageFactory.h>
#include <igtlTrackingDataMessage.h>

// STL includes
#include <deque>
#include <string>

// Windows includes
#include <ppltasks.h>
#include <concrt.h>
#include <vccorlib.h>
#include <collection.h>

namespace UWPOpenIGTLink
{
  ///
  /// \class IGTLinkClient
  /// \brief This class provides an OpenIGTLink client. It has basic functionality for sending and receiving messages
  ///
  /// \description It connects to an IGTLink v3+ server, sends requests and receives responses.
  ///
  public ref class IGTClient sealed
  {
  public:
    IGTClient();
    virtual ~IGTClient();

    property Platform::String^ ServerPort {Platform::String ^ get(); void set(Platform::String^); }
    property Windows::Networking::HostName^ ServerHost { Windows::Networking::HostName ^ get(); void set(Windows::Networking::HostName^); }
    property int ServerIGTLVersion { int get(); void set(int); }
    property bool Connected { bool get(); }
    property float TrackerUnitScale { float get(); void set(float); }
    property TransformName^ EmbeddedImageTransformName { TransformName ^ get(); void set(TransformName^); }

    /// If timeoutSec > 0 then connection will be attempted multiple times until successfully connected or the timeout elapse
    Windows::Foundation::IAsyncOperation<bool>^ ConnectAsync(double timeoutSec);

    /// Disconnect from the connected server
    void Disconnect();

    /// Retrieve the latest tracked frame
    TrackedFrame^ GetTrackedFrame(double lastKnownTimestamp);

    /// Retrieve the latest transform frame
    TransformListABI^ GetTransformFrame(double lastKnownTimestamp);

    /// Send a message to the connected server
    Windows::Foundation::IAsyncOperation<bool>^ SendMessageAsync(MessageBasePointerPtr messageBasePointerAsIntPtr);

  internal:
    /// Send a packed message to the connected server
    Concurrency::task<bool> SendMessageAsync(igtl::MessageBase::Pointer packedMessage);

    /// Threaded function to receive data from the connected server
    void DataReceiverPump();

  protected private:
    /// Thread-safe method that allows child classes to read data from the socket
    int SocketReceive(void* data, int length);
    void PruneUWPTypes();
    void PruneIGTMessages();

    double GetLatestTrackedFrameTimestamp() const;
    double GetOldestTrackedFrameTimestamp() const;

    double GetLatestTDataTimestamp() const;
    double GetOldestTDataTimestamp() const;

    template<typename MessageTypePointer> double GetLatestTimestamp(const std::deque<MessageTypePointer>& messages) const;
    template<typename MessageTypePointer> double GetOldestTimestamp(const std::deque<MessageTypePointer>& messages) const;

  protected private:
    /// igtl Factory for message sending
    igtl::MessageFactory::Pointer                     m_igtlMessageFactory = igtl::MessageFactory::New();

    Concurrency::task<void>                           m_dataReceiverTask;
    Concurrency::cancellation_token_source            m_receiverPumpTokenSource;

    /// Socket that is connected to the server
    std::mutex                                        m_socketMutex;
    Windows::Networking::Sockets::StreamSocket^       m_clientSocket = ref new Windows::Networking::Sockets::StreamSocket();
    Windows::Storage::Streams::DataWriter^            m_sendStream = nullptr;
    Windows::Storage::Streams::DataReader^            m_readStream = nullptr;
    Windows::Networking::HostName^                    m_hostName = nullptr;
    std::atomic_bool                                  m_connected = false;

    /// Lists of messages received through the socket, transformed to igtl messages
    mutable std::mutex                                m_trackedFrameMessagesMutex;
    std::deque<igtl::TrackedFrameMessage::Pointer>    m_trackedFrameMessages;

    mutable std::mutex                                m_tdataMessagesMutex;
    std::deque<igtl::TrackingDataMessage::Pointer>    m_tdataMessages;

    /// List of messages to be sent to the IGT server
    std::vector<igtl::MessageBase::Pointer>           m_sendMessages;

    /// List of messages converted to UWP run time
    std::mutex                                        m_framesMutex;
    std::deque<TrackedFrame^>                         m_trackedFrames;
    std::mutex                                        m_transformsMutex;
    std::deque<TransformListABI^>                     m_transforms;

    /// Server information
    float                                             m_trackerUnitScale = 0.001f; // Scales translation component of incoming transformations by the given factor
    TransformName^                                    m_embeddedImageTransformName = nullptr;
    Platform::String^                                 m_serverPort = L"18944";
    int                                               m_serverIGTLVersion = IGTL_HEADER_VERSION_2;

    static const int                                  CLIENT_SOCKET_TIMEOUT_MSEC;
    static const uint32                               MESSAGE_LIST_MAX_SIZE;

  private:
    IGTClient(IGTClient^) {}
    void operator=(IGTClient^) {}
  };
}

#include "IGTClient.txx"