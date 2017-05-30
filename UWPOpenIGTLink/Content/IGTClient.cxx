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
using namespace Platform::Collections;
using namespace Windows::Data::Xml::Dom;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Security::Cryptography;
using namespace Windows::Storage::Streams;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

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

    m_clientSocket->Control->KeepAlive = true;
    m_clientSocket->Control->NoDelay = false; // true => accumulate data until enough has been queued to occupy a full TCP/IP packet
    m_sendStream = ref new DataWriter(m_clientSocket->OutputStream);
    m_readStream = ref new DataReader(m_clientSocket->InputStream);
  }

  //----------------------------------------------------------------------------
  IGTClient::~IGTClient()
  {
    Disconnect();
  }

  //----------------------------------------------------------------------------
  IAsyncOperation<bool>^ IGTClient::ConnectAsync(double timeoutSec)
  {
    if (m_connected)
    {
      return create_async([]() {return true;});
    }

    m_receiverPumpTokenSource = cancellation_token_source();

    // Connect to the server (by default, the listener we created in the previous step).
    return create_async([this, timeoutSec]() -> task<bool>
    {
      return create_task(m_clientSocket->ConnectAsync(m_hostName, m_serverPort)).then([this](task<void> previousTask)
      {
        try
        {
          // Try getting all exceptions from the continuation chain above this point.
          previousTask.get();
          m_connected = true;
        }
        catch (Platform::Exception^ exception)
        {
          return;
        }
      }).then([this, timeoutSec]() -> bool
      {
        if (!m_connected)
        {
          return false;
        }

        // We're connected, start the data receiver thread
        create_task([this]()
        {
          DataReceiverPump();
        }).then([this](task<void> previousTask)
        {
          try
          {
            previousTask.wait();

            std::lock_guard<std::mutex> guard(m_socketMutex);
            m_sendStream = nullptr;
            m_readStream = nullptr;
            delete m_clientSocket;

            // Recreate blank socket
            m_clientSocket = ref new StreamSocket();
            m_clientSocket->Control->KeepAlive = true;
            m_clientSocket->Control->NoDelay = false;
            m_sendStream = ref new DataWriter(m_clientSocket->OutputStream);
            m_readStream = ref new DataReader(m_clientSocket->InputStream);

            m_connected = false;
          }
          catch (const std::exception& e)
          {
            OutputDebugStringA((std::string("DataReceiverPump crash: ") + e.what()).c_str());
          }
        });

        return true;
      });
    });
  }

  //----------------------------------------------------------------------------
  void IGTClient::Disconnect()
  {
    m_clientSocket->CancelIOAsync();
    m_receiverPumpTokenSource.cancel();
  }

  //----------------------------------------------------------------------------
  TrackedFrame^ IGTClient::GetTrackedFrame(double lastKnownTimestamp)
  {
    igtl::TrackedFrameMessage::Pointer trackedFrameMsg = nullptr;
    {
      // Retrieve the next available tracked frame message
      std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
      if (m_receiveMessages.size() == 0)
      {
        return nullptr;
      }
      for (auto riter = m_receiveMessages.rbegin(); riter != m_receiveMessages.rend(); ++riter)
      {
        if (std::string((*riter)->GetDeviceType()).compare("TRACKEDFRAME") == 0)
        {
          trackedFrameMsg = dynamic_cast<igtl::TrackedFrameMessage*>(riter->GetPointer());
          break;
        }
      }
    }

    if (trackedFrameMsg == nullptr)
    {
      return nullptr;
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
  TransformListABI^ IGTClient::GetTDataFrame(double lastKnownTimestamp)
  {
    igtl::TrackingDataMessage::Pointer tdataMsg(nullptr);
    {
      // Retrieve the next available TDATA message
      std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
      if (m_receiveMessages.size() == 0)
      {
        return nullptr;
      }
      for (auto riter = m_receiveMessages.rbegin(); riter != m_receiveMessages.rend(); ++riter)
      {
        if (std::string((*riter)->GetDeviceType()).compare("TDATA") == 0)
        {
          tdataMsg = dynamic_cast<igtl::TrackingDataMessage*>(riter->GetPointer());
          break;
        }
      }
    }

    if (tdataMsg == nullptr)
    {
      return nullptr;
    }

    auto ts = igtl::TimeStamp::New();
    tdataMsg->GetTimeStamp(ts);

    if (ts->GetTimeStamp() <= lastKnownTimestamp)
    {
      std::lock_guard<std::mutex> guard(m_tdataMutex);
      auto iter = std::find_if(m_tdata.begin(), m_tdata.end(), [this, lastKnownTimestamp](TransformListABI ^ frame)
      {
        if (frame->Size == 0)
        {
          return false;
        }
        return fabs(frame->GetAt(0)->Timestamp - lastKnownTimestamp) < NEGLIGIBLE_DIFFERENCE;
      });
      // No new messages since requested timestamp
      if (iter != m_tdata.end())
      {
        return *iter;
      }
      return nullptr;
    }

    std::lock_guard<std::mutex> guard(m_tdataMutex);
    auto frame = ref new Vector<Transform^>();
    m_tdata.push_back(frame);

    auto element = igtl::TrackingDataElement::New();
    igtl::Matrix4x4 mat;
    for (auto i = 0; i < tdataMsg->GetNumberOfTrackingDataElements(); ++i)
    {
      auto transform = ref new Transform();
      tdataMsg->GetTrackingDataElement(i, element);
      auto name = std::string(element->GetName());
      TransformName^ transformName(nullptr);
      try
      {
        // If the transform name is > 20 characters, name will be ""
        transformName = ref new TransformName(std::wstring(begin(name), end(name)));
      }
      catch (Platform::Exception^ e)
      {
        OutputDebugStringA("Transform being sent from IGT server has an invalid name.\n");
        continue;
      }
      element->GetMatrix(mat);
      float4x4 matrix;
      XMStoreFloat4x4(&matrix, XMLoadFloat4x4(&DirectX::XMFLOAT4X4(&mat[0][0])));

      transform->Name = transformName;
      transform->Matrix = matrix;
      transform->Valid = (matrix != float4x4::identity());
      transform->Timestamp = ts->GetTimeStamp();
      frame->Append(transform);
    }

    return frame;
  }

  //----------------------------------------------------------------------------
  UWPOpenIGTLink::Transform^ IGTClient::GetTransform(TransformName^ name, double lastKnownTimestamp)
  {
    std::wstring wname = name->GetTransformNameInternal();
    std::string nameStr(begin(wname), end(wname));

    igtl::TransformMessage::Pointer transformMessage(nullptr);
    {
      // Retrieve the next available TDATA message
      std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
      if (m_receiveMessages.size() == 0)
      {
        return nullptr;
      }
      for (auto riter = m_receiveMessages.rbegin(); riter != m_receiveMessages.rend(); ++riter)
      {
        if (std::string((*riter)->GetDeviceType()).compare("TRANSFORM") == 0 && nameStr.compare((*riter)->GetDeviceName()) == 0)
        {
          transformMessage = dynamic_cast<igtl::TransformMessage*>(riter->GetPointer());
          break;
        }
      }
    }

    if (transformMessage == nullptr)
    {
      return nullptr;
    }

    auto ts = igtl::TimeStamp::New();
    transformMessage->GetTimeStamp(ts);

    if (ts->GetTimeStamp() <= lastKnownTimestamp)
    {
      std::lock_guard<std::mutex> guard(m_transformsMutex);
      auto iter = std::find_if(m_transforms[wname].begin(), m_transforms[wname].end(), [this, lastKnownTimestamp](Transform ^ transform)
      {
        return fabs(transform->Timestamp - lastKnownTimestamp) < NEGLIGIBLE_DIFFERENCE;
      });

      // No new messages since requested timestamp
      if (iter != m_transforms[wname].end())
      {
        return *iter;
      }
      return nullptr;
    }

    std::lock_guard<std::mutex> guard(m_transformsMutex);
    auto transform = ref new Transform();
    m_transforms[wname].push_back(transform);

    TransformName^ transformName(nullptr);
    try
    {
      // If the transform name is > 20 characters, name will be ""
      transformName = ref new TransformName(wname);
      transform->Name = transformName;
    }
    catch (Platform::Exception^ e)
    {
      OutputDebugStringA("Transform being sent from IGT server has an invalid name.\n");
    }

    igtl::Matrix4x4 mat;
    transformMessage->GetMatrix(mat);
    float4x4 matrix;
    XMStoreFloat4x4(&matrix, XMLoadFloat4x4(&DirectX::XMFLOAT4X4(&mat[0][0])));

    transform->Matrix = matrix;
    transform->Valid = (matrix != float4x4::identity());
    transform->Timestamp = ts->GetTimeStamp();

    return transform;
  }

  //----------------------------------------------------------------------------
  task<bool> IGTClient::SendMessageAsync(igtl::MessageBase::Pointer packedMessage)
  {
    std::lock_guard<std::mutex> guard(m_socketMutex);
    m_sendStream->WriteBytes(Platform::ArrayReference<byte>((byte*)packedMessage->GetBufferPointer(), packedMessage->GetBufferSize()));
    return create_task(m_sendStream->StoreAsync()).then([size = packedMessage->GetBufferSize()](task<uint32> writeTask)
    {
      uint32 bytesWritten;
      try
      {
        bytesWritten = writeTask.get();
        return bytesWritten == size;
      }
      catch (Platform::Exception^ exception)
      {
        return false;
      }
    });
  }

  //----------------------------------------------------------------------------
  IAsyncOperation<bool>^ IGTClient::SendMessageAsync(MessageBasePointerPtr messageBasePointerPtr)
  {
    return create_async([this, messageBasePointerPtr]()
    {
      igtl::MessageBase::Pointer* messageBasePointer = (igtl::MessageBase::Pointer*)(messageBasePointerPtr);
      return SendMessageAsync(*messageBasePointer);
    });
  }

  //----------------------------------------------------------------------------
  void IGTClient::DataReceiverPump()
  {
    LOG_TRACE(L"IGTLinkClient::DataReceiverPump");

    auto headerMsg = m_igtlMessageFactory->CreateHeaderMessage(IGTL_HEADER_VERSION_1);
    auto token = m_receiverPumpTokenSource.get_token();

    while (!token.is_canceled())
    {
      auto start = std::chrono::system_clock::now();
      headerMsg->InitBuffer();

      // Receive generic header from the socket
      int numOfBytesReceived = 0;
      {
        if (SocketReceive(headerMsg->GetBufferPointer(), headerMsg->GetBufferSize()) != headerMsg->GetBufferSize())
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }
      }

      int c = headerMsg->Unpack(1);
      if (!(c & igtl::MessageHeader::UNPACK_HEADER))
      {
        OutputDebugStringA("Failed to receive message (invalid header)\n");
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

      if (bodyMsg.IsNull() || bodyMsg->GetBufferBodySize() == 0)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      if (SocketReceive(bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize()) != bodyMsg->GetBufferBodySize())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      // Accept all messages but status messages, they are used as a keep alive mechanism
      if (typeid(*bodyMsg) == typeid(igtl::TrackedFrameMessage))
      {
        c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          LOG_TRACE("Failed to receive reply (invalid body)");
          continue;
        }

        igtl::TrackedFrameMessage* trackedFrameMessage = (igtl::TrackedFrameMessage*)bodyMsg.GetPointer();

        // Post process tracked frame to adjust for unit scale
        trackedFrameMessage->ApplyTransformUnitScaling(m_trackerUnitScale);

        // Save reply
        std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
        m_receiveMessages.push_back(trackedFrameMessage);
      }
      else if (typeid(*bodyMsg) == typeid(igtl::TrackingDataMessage))
      {
        c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          LOG_TRACE("Failed to receive reply (invalid body)");
          continue;
        }

        auto tdataMessage = (igtl::TrackingDataMessage*)bodyMsg.GetPointer();

        // Post process TDATA to adjust for unit scale
        auto element = igtl::TrackingDataElement::New();
        for (int i = 0; i < tdataMessage->GetNumberOfTrackingDataElements(); ++i)
        {
          tdataMessage->GetTrackingDataElement(i, element);
          igtl::Matrix4x4 mat;
          element->GetMatrix(mat);
          mat[0][3] = mat[0][3] * m_trackerUnitScale;
          mat[1][3] = mat[1][3] * m_trackerUnitScale;
          mat[2][3] = mat[2][3] * m_trackerUnitScale;
          element->SetMatrix(mat);
        }

        // Save reply
        std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
        m_receiveMessages.push_back(tdataMessage);
      }
      else if (typeid(*bodyMsg) == typeid(igtl::TransformMessage))
      {
        c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          LOG_TRACE("Failed to receive reply (invalid body)");
          continue;
        }

        auto transformMessage = (igtl::TransformMessage*)bodyMsg.GetPointer();
        igtl::Matrix4x4 mat;
        transformMessage->GetMatrix(mat);
        mat[0][3] = mat[0][3] * m_trackerUnitScale;
        mat[1][3] = mat[1][3] * m_trackerUnitScale;
        mat[2][3] = mat[2][3] * m_trackerUnitScale;
        transformMessage->SetMatrix(mat);

        // Save reply
        std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
        m_receiveMessages.push_back(transformMessage);
      }

      PruneIGTMessages();
      PruneUWPTypes();

      auto end = std::chrono::system_clock::now();

      std::chrono::duration<double> elapsed_seconds = end - start;
      std::stringstream ss;
      ss << "elapsed seconds: " << elapsed_seconds.count() << std::endl;
      OutputDebugStringA(ss.str().c_str());
    }

    return;
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
          // Messages are stored in increasing timestamp, once we reach one that is newer than limit, we're done
          break;
        }
      }
    }

    {
      std::lock_guard<std::mutex> guard(m_tdataMutex);
      auto oldestTimestamp = GetOldestTDataTimestamp();
      for (auto iter = begin(m_tdata); iter != end(m_tdata);)
      {
        if ((*iter)->Size == 0 || (*iter)->GetAt(0)->Timestamp < oldestTimestamp)
        {
          iter = m_tdata.erase(iter);
        }
        else
        {
          break;
        }
      }
    }

    {
      std::lock_guard<std::mutex> guard(m_transformsMutex);
      for (auto iter = begin(m_transforms); iter != end(m_transforms);)
      {
        auto oldestTimestamp = GetOldestTransformTimestamp(iter->first);
        for (auto dequeIter = begin(iter->second); dequeIter != end(iter->second); ++dequeIter)
        {
          if ((*dequeIter)->Timestamp < oldestTimestamp)
          {
            dequeIter = iter->second.erase(dequeIter);
          }
          else
          {
            break;
          }
        }
      }
    }
  }

  //----------------------------------------------------------------------------
  void IGTClient::PruneIGTMessages()
  {
    std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
    if (m_receiveMessages.size() > MESSAGE_LIST_MAX_SIZE)
    {
      // erase the front N results
      uint32 toErase = m_receiveMessages.size() - MESSAGE_LIST_MAX_SIZE;
      m_receiveMessages.erase(begin(m_receiveMessages), begin(m_receiveMessages) + toErase);
    }
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestTrackedFrameTimestamp() const
  {
    // Retrieve the next available tracked frame reply
    std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
    return GetLatestTimestamp<igtl::TrackedFrameMessage>();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestTrackedFrameTimestamp() const
  {
    // Retrieve the next available tracked frame reply
    std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
    return GetOldestTimestamp<igtl::TrackedFrameMessage>();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestTDataTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
    return GetLatestTimestamp<igtl::TrackingDataMessage>();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestTDataTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
    return GetOldestTimestamp<igtl::TrackingDataMessage>();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestTransformTimestamp(const std::wstring& name) const
  {
    auto str = std::string(begin(name), end(name));

    // Retrieve the next available tracked frame reply
    if (m_receiveMessages.size() > 0)
    {
      igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
      for (auto riter = m_receiveMessages.rbegin(); riter != m_receiveMessages.rend(); riter++)
      {
        if (dynamic_cast<igtl::TransformMessage*>(riter->GetPointer()) != nullptr && std::string((*riter)->GetDeviceName()).compare(str) == 0)
        {
          (*riter)->GetTimeStamp(ts);
          return ts->GetTimeStamp();
        }
      }
    }
    return -1.0;
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestTransformTimestamp(const std::wstring& name) const
  {
    auto str = std::string(begin(name), end(name));

    // Retrieve the next available tracked frame reply
    if (m_receiveMessages.size() > 0)
    {
      igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
      for (auto iter = m_receiveMessages.begin(); iter != m_receiveMessages.end(); iter++)
      {
        if (dynamic_cast<igtl::TransformMessage*>(iter->GetPointer()) != nullptr && std::string((*iter)->GetDeviceName()).compare(str) == 0)
        {
          (*iter)->GetTimeStamp(ts);
          return ts->GetTimeStamp();
        }
      }
    }
    return -1.0;
  }

  //----------------------------------------------------------------------------
  int32 IGTClient::SocketReceive(void* dest, int size)
  {
    std::lock_guard<std::mutex> guard(m_socketMutex);
    auto readTask = create_task(m_readStream->LoadAsync(size));
    int bytesRead(-1);
    try
    {
      bytesRead = readTask.get();
      if (bytesRead != size)
      {
        return bytesRead;
      }

      auto buffer = m_readStream->ReadBuffer(size);
      auto header = GetDataFromIBuffer<byte>(buffer);
      memcpy(dest, header, size);
    }
    catch (...)
    {
      return -1;
    }

    return bytesRead;
  }

  //----------------------------------------------------------------------------
  Platform::String^ IGTClient::ServerPort::get()
  {
    return m_serverPort;
  }

  //----------------------------------------------------------------------------
  void IGTClient::ServerPort::set(Platform::String^ arg)
  {
    m_serverPort = arg;
  }

  //----------------------------------------------------------------------------
  Windows::Networking::HostName^ IGTClient::ServerHost::get()
  {
    return m_hostName;
  }

  //----------------------------------------------------------------------------
  void IGTClient::ServerHost::set(Windows::Networking::HostName^ arg)
  {
    m_hostName = arg;
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
    return m_connected;
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