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
#include "IGTClient.h"
#include "IGTCommon.h"
#include "TrackedFrameMessage.h"

// IGT includes
#include <igtlCommandMessage.h>
#include <igtlCommon.h>
#include <igtlMessageHeader.h>
#include <igtlOSUtil.h>
#include <igtlStatusMessage.h>

// STL includes
#include <chrono>
#include <regex>

// Windows includes
#include <collection.h>
#include <pplawait.h>
#include <ppltasks.h>
#include <robuffer.h>

using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Platform::Collections;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::Data::Xml::Dom;

namespace UWPOpenIGTLink
{
  namespace
  {
    static const double NEGLIGIBLE_DIFFERENCE = 0.0001;
  }
  const int IGTClient::CLIENT_SOCKET_TIMEOUT_MSEC = 500;
  const uint32 IGTClient::MESSAGE_LIST_MAX_SIZE = 200;

  //----------------------------------------------------------------------------
  IGTClient::IGTClient()
  {
    m_igtlMessageFactory->AddMessageType("TRACKEDFRAME", (igtl::MessageFactory::PointerToMessageBaseNew)&igtl::TrackedFrameMessage::New);
  }

  //----------------------------------------------------------------------------
  IGTClient::~IGTClient()
  {
    Disconnect();
  }

  //----------------------------------------------------------------------------
  IAsyncOperation<bool>^ IGTClient::ConnectAsync(double timeoutSec)
  {
    if (m_clientSocket->GetConnected())
    {
      return create_async([]() {return true; });
    }

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
        DataReceiverPump();
      }).then([this](task<void> previousTask)
      {
        try
        {
          previousTask.wait();
        }
        catch (const std::exception& e)
        {
          OutputDebugStringA((std::string("DataReceiverPump crash: ") + e.what()).c_str());
        }
      });

      return true;
    });
  }

  //----------------------------------------------------------------------------
  void IGTClient::Disconnect()
  {
    m_receiverPumpTokenSource.cancel();

    std::lock_guard<std::mutex> guard(m_socketMutex);
    m_clientSocket->CloseSocket();
  }

  //----------------------------------------------------------------------------
  TrackedFrame^ IGTClient::GetTrackedFrame(double lastKnownTimestamp)
  {
    igtl::TrackedFrameMessage::Pointer trackedFrameMsg = nullptr;
    {
      // Retrieve the next available tracked frame reply
      std::lock_guard<std::mutex> guard(m_trackedFrameMessagesMutex);
      if (m_trackedFrameMessages.size() == 0)
      {
        return nullptr;
      }
      trackedFrameMsg = dynamic_cast<igtl::TrackedFrameMessage*>((*m_trackedFrameMessages.rbegin()).GetPointer());
    }

    std::lock_guard<std::mutex> guard(m_framesMutex);
    auto ts = igtl::TimeStamp::New();
    trackedFrameMsg->GetTimeStamp(ts);

    if (ts->GetTimeStamp() <= lastKnownTimestamp)
    {
      auto iter = std::find_if(m_trackedFrames.begin(), m_trackedFrames.end(), [this, lastKnownTimestamp](TrackedFrame ^ frame)
      {
        return fabs(frame->Timestamp - lastKnownTimestamp) < NEGLIGIBLE_DIFFERENCE;
      });
      // No new messages since requested timestamp
      if (iter != m_trackedFrames.end())
      {
        return *iter;
      }
      return nullptr;
    }

    auto frame = ref new TrackedFrame();
    m_trackedFrames.push_back(frame);

    // Fields
    for (auto& pair : trackedFrameMsg->GetMetaData())
    {
      std::wstring keyWideStr(pair.first.begin(), pair.first.end());
      std::wstring valueWideStr(pair.second.second.begin(), pair.second.second.end());
      frame->SetFrameField(keyWideStr, valueWideStr);
    }

    // Image
    std::array<uint16, 3> frameSize = { trackedFrameMsg->GetFrameSize()[0], trackedFrameMsg->GetFrameSize()[1], trackedFrameMsg->GetFrameSize()[2] };
    frame->Frame->SetImageData(trackedFrameMsg->GetImage(), trackedFrameMsg->GetNumberOfComponents(), trackedFrameMsg->GetScalarType(), frameSize);
    frame->Frame->Type = (uint16)trackedFrameMsg->GetImageType();
    frame->Frame->Orientation = (uint16)trackedFrameMsg->GetImageOrientation();

    // Transforms
    frame->SetFrameTransformsInternal(trackedFrameMsg->GetFrameTransforms());
    if (EmbeddedImageTransformName != nullptr)
    {
      frame->SetTransform(ref new Transform(EmbeddedImageTransformName, trackedFrameMsg->GetEmbeddedImageTransform(), trackedFrameMsg->GetEmbeddedImageTransform() != float4x4::identity(), frame->Timestamp));
    }

    // Timestamp
    frame->Timestamp = ts->GetTimeStamp();

    return frame;
  }

  //----------------------------------------------------------------------------
  Windows::Foundation::Collections::IVector<Transform^>^ IGTClient::GetTransforms(double lastKnownTimestamp)
  {
    igtl::TrackingDataMessage::Pointer tdataMsg = nullptr;
    {
      // Retrieve the next available tracked frame reply
      std::lock_guard<std::mutex> guard(m_tdataMessagesMutex);
      if (m_tdataMessages.size() == 0)
      {
        return nullptr;
      }
      tdataMsg = dynamic_cast<igtl::TrackingDataMessage*>((*m_tdataMessages.rbegin()).GetPointer());
    }

    auto ts = igtl::TimeStamp::New();
    tdataMsg->GetTimeStamp(ts);

    if (ts->GetTimeStamp() <= lastKnownTimestamp)
    {
      std::lock_guard<std::mutex> guard(m_transformsMutex);
      auto iter = std::find_if(m_transforms.begin(), m_transforms.end(), [this, lastKnownTimestamp](TransformFrame ^ frame)
      {
        if (frame->Size == 0)
        {
          return false;
        }
        return fabs(frame->GetAt(0)->Timestamp - lastKnownTimestamp) < NEGLIGIBLE_DIFFERENCE;
      });
      // No new messages since requested timestamp
      if (iter != m_transforms.end())
      {
        return *iter;
      }
      return nullptr;
    }

    std::lock_guard<std::mutex> guard(m_transformsMutex);
    auto frame = ref new TransformFrame();
    m_transforms.push_back(frame);

    auto element = igtl::TrackingDataElement::New();
    igtl::Matrix4x4 mat;
    for (auto i = 0; i < tdataMsg->GetNumberOfTrackingDataElements(); ++i)
    {
      auto transform = ref new Transform();
      tdataMsg->GetTrackingDataElement(i, element);
      auto name = std::string(element->GetName());
      auto transformName = ref new TransformName(std::wstring(begin(name), end(name)));
      element->GetMatrix(mat);
      float4x4 matrix;
      XMStoreFloat4x4(&matrix, XMLoadFloat4x4(&DirectX::XMFLOAT4X4(&mat[0][0])));

      transform->Name = transformName;
      transform->Matrix = matrix;
      transform->Valid = (matrix == float4x4::identity());
      transform->Timestamp = ts->GetTimeStamp();
      frame->Append(transform);
    }

    return frame;
  }

  //----------------------------------------------------------------------------
  bool IGTClient::SendMessage(igtl::MessageBase::Pointer packedMessage)
  {
    std::lock_guard<std::mutex> guard(m_socketMutex);
    int success = m_clientSocket->Send(packedMessage->GetBufferPointer(), packedMessage->GetBufferSize());

    if (!success)
    {
      std::cerr << "OpenIGTLink client couldn't send message to server." << std::endl;
      return false;
    }
    return true;
  }

  //----------------------------------------------------------------------------
  bool IGTClient::SendMessage(MessageBasePointerPtr messageBasePointerPtr)
  {
    igtl::MessageBase::Pointer* messageBasePointer = (igtl::MessageBase::Pointer*)(messageBasePointerPtr);
    return SendMessage(*messageBasePointer);
  }

  //----------------------------------------------------------------------------
  void IGTClient::DataReceiverPump()
  {
    LOG_TRACE(L"IGTLinkClient::DataReceiverPump");

    auto headerMsg = m_igtlMessageFactory->CreateHeaderMessage(IGTL_HEADER_VERSION_1);

    auto token = m_receiverPumpTokenSource.get_token();

    while (!token.is_canceled())
    {
      headerMsg->InitBuffer();

      // Receive generic header from the socket
      int numOfBytesReceived = 0;
      {
        std::lock_guard<std::mutex> guard(m_socketMutex);
        if (!m_clientSocket->GetConnected())
        {
          // We've become disconnected while waiting for the socket, we're done here!
          return;
        }
        numOfBytesReceived = m_clientSocket->Receive(headerMsg->GetBufferPointer(), headerMsg->GetBufferSize());
      }
      if (numOfBytesReceived == 0 || numOfBytesReceived != headerMsg->GetBufferSize())
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
        bodyMsg = m_igtlMessageFactory->CreateReceiveMessage(headerMsg);
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

      {
        std::lock_guard<std::mutex> guard(m_socketMutex);
        if (!m_clientSocket->GetConnected())
        {
          // We've become disconnected while waiting for the socket, we're done here!
          return;
        }
        m_clientSocket->Receive(bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize());
      }

      c = bodyMsg->Unpack(1);
      if (!(c & igtl::MessageHeader::UNPACK_BODY))
      {
        LOG_TRACE("Failed to receive reply (invalid body)");
        continue;
      }

      // Accept all messages but status messages, they are used as a keep alive mechanism
      if (typeid(*bodyMsg) == typeid(igtl::TrackedFrameMessage))
      {
        igtl::TrackedFrameMessage* trackedFrameMessage = (igtl::TrackedFrameMessage*)bodyMsg.GetPointer();

        // Post process tracked frame to adjust for unit scale
        trackedFrameMessage->ApplyTransformUnitScaling(m_trackerUnitScale);

        // Save reply
        std::lock_guard<std::mutex> guard(m_trackedFrameMessagesMutex);
        m_trackedFrameMessages.push_back(trackedFrameMessage);
      }
      else if (typeid(*bodyMsg) == typeid(igtl::TrackingDataMessage))
      {
        igtl::TrackingDataMessage* tdataMessage = (igtl::TrackingDataMessage*)bodyMsg.GetPointer();

        // Save reply
        std::lock_guard<std::mutex> guard(m_tdataMessagesMutex);
        m_tdataMessages.push_back(tdataMessage);
      }
      else
      {
        std::lock_guard<std::mutex> guard(m_socketMutex);
        m_clientSocket->Skip(headerMsg->GetBodySizeToRead(), 0);
      }

      PruneIGTMessages();
      PruneUWPTypes();
    }

    return;
  }

  //----------------------------------------------------------------------------
  int IGTClient::SocketReceive(void* data, int length)
  {
    std::lock_guard<std::mutex> guard(m_socketMutex);
    return m_clientSocket->Receive(data, length);
  }

  //----------------------------------------------------------------------------
  void IGTClient::PruneUWPTypes()
  {
    {
      std::lock_guard<std::mutex> guard(m_framesMutex);
      auto oldestTimestamp = GetOldestTrackedFrameTimestamp();
      for (auto iter = begin(m_trackedFrames); iter != end(m_trackedFrames);)
      {
        if ((*iter)->Timestamp < oldestTimestamp)
        {
          iter = m_trackedFrames.erase(iter);
        }
        else
        {
          ++iter;
        }
      }
    }

    {
      std::lock_guard<std::mutex> guard(m_transformsMutex);
      auto oldestTimestamp = GetOldestTDataTimestamp();
      for (auto iter = begin(m_transforms); iter != end(m_transforms);)
      {
        if ((*iter)->Size == 0 || (*iter)->GetAt(0)->Timestamp < oldestTimestamp)
        {
          iter = m_transforms.erase(iter);
        }
        else
        {
          ++iter;
        }
      }
    }
  }

  //----------------------------------------------------------------------------
  void IGTClient::PruneIGTMessages()
  {
    {
      std::lock_guard<std::mutex> guard(m_trackedFrameMessagesMutex);
      if (m_trackedFrameMessages.size() > MESSAGE_LIST_MAX_SIZE)
      {
        // erase the front N results
        uint32 toErase = m_trackedFrameMessages.size() - MESSAGE_LIST_MAX_SIZE;
        m_trackedFrameMessages.erase(begin(m_trackedFrameMessages), begin(m_trackedFrameMessages) + toErase);
      }
    }

    {
      std::lock_guard<std::mutex> guard(m_tdataMessagesMutex);
      if (m_tdataMessages.size() > MESSAGE_LIST_MAX_SIZE)
      {
        // erase the front N results
        uint32 toErase = m_tdataMessages.size() - MESSAGE_LIST_MAX_SIZE;
        m_tdataMessages.erase(begin(m_tdataMessages), begin(m_tdataMessages) + toErase);
      }
    }
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestTrackedFrameTimestamp() const
  {
    // Retrieve the next available tracked frame reply
    std::lock_guard<std::mutex> guard(m_trackedFrameMessagesMutex);
    return GetLatestTimestamp<igtl::TrackedFrameMessage::Pointer>(m_trackedFrameMessages);
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestTrackedFrameTimestamp() const
  {
    // Retrieve the next available tracked frame reply
    std::lock_guard<std::mutex> guard(m_trackedFrameMessagesMutex);
    return GetOldestTimestamp<igtl::TrackedFrameMessage::Pointer>(m_trackedFrameMessages);
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestTDataTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_tdataMessagesMutex);
    return GetLatestTimestamp<igtl::TrackingDataMessage::Pointer>(m_tdataMessages);
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestTDataTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_tdataMessagesMutex);
    return GetOldestTimestamp<igtl::TrackingDataMessage::Pointer>(m_tdataMessages);
  }

  //----------------------------------------------------------------------------
  int IGTClient::ServerPort::get()
  {
    return m_serverPort;
  }

  //----------------------------------------------------------------------------
  void IGTClient::ServerPort::set(int arg)
  {
    m_serverPort = arg;
  }

  //----------------------------------------------------------------------------
  Platform::String^ IGTClient::ServerHost::get()
  {
    return m_serverHost;
  }

  //----------------------------------------------------------------------------
  void IGTClient::ServerHost::set(Platform::String^ arg)
  {
    m_serverHost = arg;
  }

  //----------------------------------------------------------------------------
  int IGTClient::ServerIGTLVersion::get()
  {
    return m_serverIGTLVersion;
  }

  //----------------------------------------------------------------------------
  void IGTClient::ServerIGTLVersion::set(int arg)
  {
    m_serverIGTLVersion = arg;
  }

  //----------------------------------------------------------------------------
  bool IGTClient::Connected::get()
  {
    return m_clientSocket->GetConnected();
  }

  //----------------------------------------------------------------------------
  float IGTClient::TrackerUnitScale::get()
  {
    return m_trackerUnitScale;
  }

  //----------------------------------------------------------------------------
  void IGTClient::TrackerUnitScale::set(float arg)
  {
    m_trackerUnitScale = arg;
  }

  //----------------------------------------------------------------------------
  TransformName^ IGTClient::EmbeddedImageTransformName::get()
  {
    return m_embeddedImageTransformName;
  }

  //----------------------------------------------------------------------------
  void IGTClient::EmbeddedImageTransformName::set(TransformName^ arg)
  {
    m_embeddedImageTransformName = arg;
  }
}