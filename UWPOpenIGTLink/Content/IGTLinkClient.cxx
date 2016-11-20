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
#include "pch.h"
#include "IGTLinkClient.h"
#include "IGTCommon.h"
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
    : m_igtlMessageFactory(igtl::MessageFactory::New())
    , m_clientSocket(igtl::ClientSocket::New())
  {
    m_igtlMessageFactory->AddMessageType("TRACKEDFRAME", (igtl::MessageFactory::PointerToMessageBaseNew)&igtl::TrackedFrameMessage::New);
    ServerHost = L"127.0.0.1";
    ServerPort = 18944;
    ServerIGTLVersion = IGTL_HEADER_VERSION_2;

    m_frameSize.assign(3, 0);
  }

  //----------------------------------------------------------------------------
  IGTLinkClient::~IGTLinkClient()
  {
    Disconnect();
    auto disconnectTask = concurrency::create_task([this]()
    {
      while (Connected)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
      }
    });
    disconnectTask.wait();
  }

  //----------------------------------------------------------------------------
  IAsyncOperation<bool>^ IGTLinkClient::ConnectAsync(double timeoutSec)
  {
    Disconnect();

    m_receiverPumpTokenSource = cancellation_token_source();

    return create_async([this, timeoutSec]() -> bool
    {
      const int retryDelaySec = 1.0;
      int errorCode = 1;
      auto start = std::chrono::high_resolution_clock::now();
      while (errorCode != 0)
      {
        std::wstring wideStr(ServerHost->Begin());
        std::string str(wideStr.begin(), wideStr.end());
        errorCode = m_clientSocket->ConnectToServer(str.c_str(), ServerPort);
        if (errorCode == 0)
        {
          break;
        }
        std::chrono::duration<double, std::milli> timeDiff = std::chrono::high_resolution_clock::now() - start;
        if (timeDiff.count() > timeoutSec * 1000)
        {
          // time is up
          break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(retryDelaySec));
      }

      if (errorCode != 0)
      {
        return false;
      }

      m_clientSocket->SetTimeout(CLIENT_SOCKET_TIMEOUT_MSEC);
      m_clientSocket->SetReceiveBlocking(true);

      // We're connected, start the data receiver thread
      create_task([this]()
      {
        DataReceiverPump(this, m_receiverPumpTokenSource.get_token());
      });

      return true;
    });
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::Disconnect()
  {
    m_receiverPumpTokenSource.cancel();

    std::lock_guard<std::mutex> guard(m_socketMutex);
    m_clientSocket->CloseSocket();
  }

  //----------------------------------------------------------------------------
  TrackedFrame^ IGTLinkClient::GetTrackedFrame(double lastKnownTimestamp)
  {
    igtl::TrackedFrameMessage::Pointer trackedFrameMsg = nullptr;
    {
      // Retrieve the next available tracked frame reply
      std::lock_guard<std::mutex> guard(m_messageListMutex);
      for (auto replyIter = m_receivedMessages.rbegin(); replyIter != m_receivedMessages.rend(); ++replyIter)
      {
        if (typeid(*(*replyIter)) == typeid(igtl::TrackedFrameMessage))
        {
          trackedFrameMsg = dynamic_cast<igtl::TrackedFrameMessage*>((*replyIter).GetPointer());
          break;
        }
      }

      if (trackedFrameMsg == nullptr)
      {
        // No message found
        return nullptr;
      }
    }

    auto ts = igtl::TimeStamp::New();
    trackedFrameMsg->GetTimeStamp(ts);
    if (ts->GetTimeStamp() <= lastKnownTimestamp)
    {
      // No new messages since requested timestamp
      if (m_receivedUWPMessages.find(lastKnownTimestamp) != m_receivedUWPMessages.end())
      {
        return m_receivedUWPMessages.at(lastKnownTimestamp);
      }
      return nullptr;
    }

    auto frame = ref new TrackedFrame();
    m_receivedUWPMessages[ts->GetTimeStamp()] = frame;

    // Tracking/other related fields
    for (auto& pair : trackedFrameMsg->GetMetaData())
    {
      std::wstring keyWideStr(pair.first.begin(), pair.first.end());
      std::wstring valueWideStr(pair.second.begin(), pair.second.end());
      frame->SetCustomFrameField(keyWideStr, valueWideStr);
    }

    for (auto& pair : trackedFrameMsg->GetCustomFrameFields())
    {
      frame->SetCustomFrameField(pair.first, pair.second);
    }

    // Image related fields
    frame->SetFrameSize(trackedFrameMsg->GetFrameSize());
    frame->Timestamp = ts->GetTimeStamp();
    frame->ImageSizeBytes = trackedFrameMsg->GetImageSizeInBytes();
    frame->SetImageData(trackedFrameMsg->GetImage());
    frame->NumberOfComponents = trackedFrameMsg->GetNumberOfComponents();
    frame->ScalarType = trackedFrameMsg->GetScalarType();
    frame->SetEmbeddedImageTransform(trackedFrameMsg->GetEmbeddedImageTransform());
    frame->ImageType = (uint16)trackedFrameMsg->GetImageType();
    frame->ImageOrientation = (uint16)trackedFrameMsg->GetImageOrientation();
    frame->SetFrameTransformsInternal(trackedFrameMsg->GetFrameTransforms());

    return frame;
  }

  //----------------------------------------------------------------------------
  Command^ IGTLinkClient::GetCommand(double lastKnownTimestamp)
  {
    igtl::MessageBase::Pointer igtMessage = nullptr;
    {
      // Retrieve the next available tracked frame reply
      std::lock_guard<std::mutex> guard(m_messageListMutex);
      for (auto replyIter = m_receivedMessages.rbegin(); replyIter != m_receivedMessages.rend(); ++replyIter)
      {
        if (typeid(*(*replyIter)) == typeid(igtl::RTSCommandMessage))
        {
          igtMessage = *replyIter;
          break;
        }
      }

      if (igtMessage == nullptr)
      {
        return nullptr;
      }
    }

    auto ts = igtl::TimeStamp::New();
    igtMessage->GetTimeStamp(ts);
    if (ts->GetTimeStamp() <= lastKnownTimestamp)
    {
      return nullptr;
    }

    auto cmd = ref new Command();
    auto cmdMsg = dynamic_cast<igtl::RTSCommandMessage*>(igtMessage.GetPointer());

    cmd->CommandContent = ref new Platform::String(std::wstring(cmdMsg->GetCommandContent().begin(), cmdMsg->GetCommandContent().end()).c_str());
    cmd->CommandName = ref new Platform::String(std::wstring(cmdMsg->GetCommandName().begin(), cmdMsg->GetCommandName().end()).c_str());
    cmd->OriginalCommandId = cmdMsg->GetCommandId();
    cmd->Timestamp = lastKnownTimestamp;

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
        OutputDebugStringA("Error string not found even though error was reported. Check IGT server implementation.");
      }
    }

    for (auto& pair : cmdMsg->GetMetaData())
    {
      std::wstring keyWideStr(pair.first.begin(), pair.first.end());
      std::wstring valueWideStr(pair.second.begin(), pair.second.end());
      cmd->Parameters->Insert(ref new Platform::String(keyWideStr.c_str()), ref new Platform::String(valueWideStr.c_str()));
    }

    return cmd;
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::SendMessage(igtl::MessageBase::Pointer packedMessage)
  {
    int success = 0;
    {
      std::lock_guard<std::mutex> guard(m_socketMutex);
      success = m_clientSocket->Send(packedMessage->GetBufferPointer(), packedMessage->GetBufferSize());
    }
    if (!success)
    {
      std::cerr << "OpenIGTLink client couldn't send message to server." << std::endl;
      return false;
    }
    return true;
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::SendMessage(MessageBasePointerPtr messageBasePointerPtr)
  {
    igtl::MessageBase::Pointer* messageBasePointer = (igtl::MessageBase::Pointer*)(messageBasePointerPtr);
    return SendMessage(*messageBasePointer);
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::DataReceiverPump(IGTLinkClient^ self, concurrency::cancellation_token token)
  {
    LOG_TRACE(L"IGTLinkClient::DataReceiverPump");

    while (!token.is_canceled())
    {
      auto headerMsg = self->m_igtlMessageFactory->CreateHeaderMessage(IGTL_HEADER_VERSION_1);

      // Receive generic header from the socket
      int numOfBytesReceived = 0;
      {
        std::lock_guard<std::mutex> guard(self->m_socketMutex);
        if (!self->m_clientSocket->GetConnected())
        {
          // We've become disconnected while waiting for the socket, we're done here!
          return;
        }
        numOfBytesReceived = self->m_clientSocket->Receive(headerMsg->GetBufferPointer(), headerMsg->GetBufferSize());
      }
      if (numOfBytesReceived == 0  // No message received
          || numOfBytesReceived != headerMsg->GetBufferSize() // Received data is not as we expected
         )
      {
        // Failed to receive data, maybe the socket is disconnected
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      int c = headerMsg->Unpack(1);
      if (!(c & igtl::MessageHeader::UNPACK_HEADER))
      {
        std::cerr << "Failed to receive message (invalid header)" << std::endl;
        continue;
      }

      igtl::MessageBase::Pointer bodyMsg = nullptr;
      try
      {
        bodyMsg = self->m_igtlMessageFactory->CreateReceiveMessage(headerMsg);
      }
      catch (const std::exception&)
      {
        // Message header was not correct, skip this message
        // Will be impossible to tell if the body of this message is in the socket... this is a pretty bad corruption.
        // Force re-connect?
        LOG_TRACE("Corruption in the message header. Serious error.");
        continue;
      }

      if (bodyMsg.IsNull())
      {
        LOG_TRACE("Unable to create message of type: " << headerMsg->GetMessageType());
        continue;
      }

      // Accept all messages but status messages, they are used as a keep alive mechanism
      if (typeid(*bodyMsg) != typeid(igtl::StatusMessage))
      {
        {
          std::lock_guard<std::mutex> guard(self->m_socketMutex);
          if (!self->m_clientSocket->GetConnected())
          {
            // We've become disconnected while waiting for the socket, we're done here!
            return;
          }
          self->m_clientSocket->Receive(bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize());
        }

        int c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          LOG_TRACE("Failed to receive reply (invalid body)");
          continue;
        }

        {
          // save reply
          std::lock_guard<std::mutex> guard(self->m_messageListMutex);
          self->m_receivedMessages.push_back(bodyMsg);
        }
      }
      else
      {
        std::lock_guard<std::mutex> guard(self->m_socketMutex);
        self->m_clientSocket->Skip(headerMsg->GetBodySizeToRead(), 0);
      }

      if (self->m_receivedMessages.size() > MESSAGE_LIST_MAX_SIZE)
      {
        std::lock_guard<std::mutex> guard(self->m_messageListMutex);

        // erase the front N results
        uint32 toErase = self->m_receivedMessages.size() - MESSAGE_LIST_MAX_SIZE;
        self->m_receivedMessages.erase(self->m_receivedMessages.begin(), self->m_receivedMessages.begin() + toErase);
      }

      auto oldestTimestamp = self->GetOldestTrackedFrameTimestamp();
      if (oldestTimestamp > 0)
      {
        for (auto it = self->m_receivedUWPMessages.begin(); it != self->m_receivedUWPMessages.end();)
        {
          if (it->first < oldestTimestamp)
          {
            it = self->m_receivedUWPMessages.erase(it);
          }
          else
          {
            ++it;
          }
        }
      }
    }

    return;
  }

  //----------------------------------------------------------------------------
  int IGTLinkClient::SocketReceive(void* data, int length)
  {
    std::lock_guard<std::mutex> guard(m_socketMutex);
    return m_clientSocket->Receive(data, length);
  }

  //----------------------------------------------------------------------------
  double IGTLinkClient::GetLatestTrackedFrameTimestamp()
  {
    // Retrieve the next available tracked frame reply
    std::lock_guard<std::mutex> guard(m_messageListMutex);
    for (auto replyIter = m_receivedMessages.rbegin(); replyIter != m_receivedMessages.rend(); ++replyIter)
    {
      if (typeid(*(*replyIter)) == typeid(igtl::TrackedFrameMessage))
      {
        igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
        (*replyIter)->GetTimeStamp(ts);
        return ts->GetTimeStamp();
      }
    }
    return -1.0;
  }

  //----------------------------------------------------------------------------
  double IGTLinkClient::GetOldestTrackedFrameTimestamp()
  {
    // Retrieve the next available tracked frame reply
    std::lock_guard<std::mutex> guard(m_messageListMutex);
    for (auto replyIter = m_receivedMessages.begin(); replyIter != m_receivedMessages.end(); ++replyIter)
    {
      if (typeid(*(*replyIter)) == typeid(igtl::TrackedFrameMessage))
      {
        igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
        (*replyIter)->GetTimeStamp(ts);
        return ts->GetTimeStamp();
      }
    }
    return -1.0;
  }

  //----------------------------------------------------------------------------
  int IGTLinkClient::ServerPort::get()
  {
    return m_serverPort;
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::ServerPort::set(int arg)
  {
    m_serverPort = arg;
  }

  //----------------------------------------------------------------------------
  Platform::String^ IGTLinkClient::ServerHost::get()
  {
    return m_serverHost;
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::ServerHost::set(Platform::String^ arg)
  {
    m_serverHost = arg;
  }

  //----------------------------------------------------------------------------
  int IGTLinkClient::ServerIGTLVersion::get()
  {
    return m_serverIGTLVersion;
  }

  //----------------------------------------------------------------------------
  void IGTLinkClient::ServerIGTLVersion::set(int arg)
  {
    m_serverIGTLVersion = arg;
  }

  //----------------------------------------------------------------------------
  bool IGTLinkClient::Connected::get()
  {
    return m_clientSocket->GetConnected();
  }
}