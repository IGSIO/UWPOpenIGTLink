/*====================================================================
Copyright(c) 2017 Adam Rankin


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
#include <igtlPolyDataMessage.h>
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
  const BufferItemList::size_type IGTClient::MESSAGE_LIST_MAX_SIZE = 200;

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

    auto ts = igtl::TimeStamp::New();
    trackedFrameMsg->GetTimeStamp(ts);

    if (ts->GetTimeStamp() <= lastKnownTimestamp)
    {
      return nullptr;
    }

    auto frame = ref new TrackedFrame();

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
      return nullptr;
    }

    auto frame = ref new Vector<Transform^>();

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
      return nullptr;
    }

    auto transform = ref new Transform();

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
  Command^ IGTClient::GetCommandResult(uint32 commandId)
  {
    // Scan received messages, link by command id
    igtl::RTSCommandMessage::Pointer rtsCommandMsg(nullptr);
    {
      // Retrieve the next available TDATA message
      std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
      if (m_receiveMessages.size() == 0)
      {
        return nullptr;
      }
      for (auto riter = m_receiveMessages.rbegin(); riter != m_receiveMessages.rend(); ++riter)
      {
        rtsCommandMsg = dynamic_cast<igtl::RTSCommandMessage*>(riter->GetPointer());
        if (rtsCommandMsg != nullptr && rtsCommandMsg->GetCommandId() == commandId)
        {
          break;
        }
      }
    }

    if (rtsCommandMsg == nullptr)
    {
      return nullptr;
    }

    // Extract result
    auto command = ref new Command();
    std::string result;
    if (!rtsCommandMsg->GetMetaDataElement("Result", result))
    {
      // Message was not sent with metadata, abort
      OutputDebugStringA("Command response was not sent using v3 style metadata. Cannot process.");
      return nullptr;
    }

    command->Result = (result == "true" ? true : false);
    auto cmdName = rtsCommandMsg->GetCommandName();
    command->CommandName = ref new Platform::String(std::wstring(begin(cmdName), end(cmdName)).c_str());
    auto cmdContent = rtsCommandMsg->GetCommandContent();
    command->CommandContent = ref new Platform::String(std::wstring(begin(cmdContent), end(cmdContent)).c_str());
    command->OriginalCommandId = rtsCommandMsg->GetCommandId();
    if (!command->Result)
    {
      std::string cmdError;
      if (!rtsCommandMsg->GetMetaDataElement("Error", cmdError))
      {
        cmdError = "Unknown error. Not sent in metadata.";
      }
      command->ErrorString = ref new Platform::String(std::wstring(begin(cmdError), end(cmdError)).c_str());
    }
    auto map = ref new Platform::Collections::Map<Platform::String^, Platform::String^>();
    for (auto& pair : rtsCommandMsg->GetMetaData())
    {
      map->Insert(ref new Platform::String(std::wstring(begin(pair.first), end(pair.first)).c_str()),
                  ref new Platform::String(std::wstring(begin(pair.second.second), end(pair.second.second)).c_str()));
    }
    command->Parameters = map;

    auto ts = igtl::TimeStamp::New();
    rtsCommandMsg->GetTimeStamp(ts);
    command->Timestamp = ts->GetTimeStamp();

    return command;
  }

  //----------------------------------------------------------------------------
  UWPOpenIGTLink::Polydata^ IGTClient::GetPolydata(Platform::String^ name)
  {
    std::wstring wname(name->Data());
    std::string nameStr(begin(wname), end(wname));

    igtl::PolyDataMessage::Pointer polyMessage(nullptr);
    {
      // Retrieve the next available TDATA message
      std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
      if (m_receiveMessages.size() == 0)
      {
        return nullptr;
      }
      for (auto riter = m_receiveMessages.rbegin(); riter != m_receiveMessages.rend(); ++riter)
      {
        std::string fileName;
        if (!(*riter)->GetMetaDataElement("fileName", fileName))
        {
          continue;
        }

        if (std::string((*riter)->GetDeviceType()).compare("POLYDATA") == 0 && IsEqualInsensitive(wname, name) == 0)
        {
          polyMessage = dynamic_cast<igtl::PolyDataMessage*>(riter->GetPointer());
          break;
        }
      }
    }

    if (polyMessage == nullptr)
    {
      return nullptr;
    }

    auto polydata = ref new Polydata();

    auto ts = igtl::TimeStamp::New();
    polyMessage->GetTimeStamp(ts);
    polydata->Timestamp = ts->GetTimeStamp();

    // If scalars are present
    if (polyMessage->GetPoints() != nullptr)
    {
      auto posList = ref new Platform::Collections::Vector<float3>();
      auto& points = *polyMessage->GetPoints();
      for (auto& point : points)
      {
        assert(point.size() == 3);
        posList->Append(float3(point[0], point[1], point[2]));
      }
      polydata->Positions = posList;
    }

    // If normals are present
    if (polyMessage->GetAttribute(igtl::PolyDataAttribute::POINT_NORMAL) != nullptr)
    {
      auto normList = ref new Platform::Collections::Vector<float3>();
      auto& entries = *polyMessage->GetAttribute(igtl::PolyDataAttribute::POINT_NORMAL);
      for (uint32 i = 0; i < entries.GetSize(); ++i)
      {
        std::vector<float> normal;
        entries.GetNthData(i, normal);
        assert(normal.size() == 3);
        normList->Append(float3(normal[0], normal[1], normal[2]));
      }
      polydata->Normals = normList;
    }

    // If texture coordinates are present
    if (polyMessage->GetAttribute(igtl::PolyDataAttribute::POINT_TCOORDS) != nullptr)
    {
      auto tcoordList = ref new Platform::Collections::Vector<float3>();
      auto& entries = *polyMessage->GetAttribute(igtl::PolyDataAttribute::POINT_TCOORDS);
      for (uint32 i = 0; i < entries.GetSize(); ++i)
      {
        std::vector<float> tcoords;
        entries.GetNthData(i, tcoords);
        assert(tcoords.size() == 3);
        tcoordList->Append(float3(tcoords[0], tcoords[1], tcoords[2]));
      }
      polydata->TextureCoords = tcoordList;
    }

    // TODO: for now, only support polygons? triangle strips?
    if (polyMessage->GetPolygons() != nullptr)
    {
      auto indexList = ref new Platform::Collections::Vector<uint16>();
      auto& indices = *polyMessage->GetPolygons();
      for (uint32 i = 0; i < indices.GetNumberOfCells(); ++i)
      {
        igtl::PolyDataCellArray::Cell cell;
        if (indices.GetCell(i, cell) == 1)
        {
          assert(cell.size() == 3);
          auto iter = cell.begin();
          indexList->Append(*iter++);
          indexList->Append(*iter++);
          indexList->Append(*iter);
        }
      }
      polydata->Indices = indexList;
    }

    return polydata;
  }

  //----------------------------------------------------------------------------
  task<bool> IGTClient::SendMessageAsyncInternal(igtl::MessageBase::Pointer packedMessage)
  {
    if (typeid(*packedMessage) == typeid(igtl::CommandMessage))
    {
      OutputDebugStringA("Do not send a command as a message. Use SendCommandAsync instead.");
      return task_from_result(false);
    }

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
  Concurrency::task<CommandData> IGTClient::SendCommandAsyncInternal(igtl::MessageBase::Pointer packedMessage)
  {
    if (typeid(*packedMessage) != typeid(igtl::CommandMessage))
    {
      OutputDebugStringA("Non command message sent to SendCommandAsync. Aborting.");
      return task_from_result(CommandData() = {0, false});
    }

    // Keep track of requested message
    igtl::CommandMessage* cmdMsg = dynamic_cast<igtl::CommandMessage*>(packedMessage.GetPointer());
    cmdMsg->SetCommandId(m_nextQueryId);

    std::lock_guard<std::mutex> guard(m_socketMutex);
    m_sendStream->WriteBytes(Platform::ArrayReference<byte>((byte*)packedMessage->GetBufferPointer(), packedMessage->GetBufferSize()));
    return create_task(m_sendStream->StoreAsync()).then([this, size = packedMessage->GetBufferSize()](task<uint32> writeTask)
    {
      uint32 bytesWritten;
      try
      {
        bytesWritten = writeTask.get();
        bool success = (bytesWritten == size);

        std::lock_guard<std::mutex> guard(m_queriesMutex);
        m_outstandingQueries.push_back(m_nextQueryId++);
        CommandData command = { m_nextQueryId - 1, success };
        return command;
      }
      catch (Platform::Exception^ exception)
      {
        CommandData command = { 0, false };
        return command;
      }
    });
  }

  //----------------------------------------------------------------------------
  IAsyncOperation<bool>^ IGTClient::SendMessageAsync(MessageBasePointerPtr messageBasePointerPtr)
  {
    return create_async([this, messageBasePointerPtr]()
    {
      igtl::MessageBase::Pointer* messageBasePointer = (igtl::MessageBase::Pointer*)(messageBasePointerPtr);
      return SendMessageAsyncInternal(*messageBasePointer);
    });
  }

  //----------------------------------------------------------------------------
  IAsyncOperation<CommandData>^ IGTClient::SendCommandAsync(Platform::String^ commandName, IMap<Platform::String^, Platform::String^>^ attributes)
  {
    return create_async([this, commandName, attributes]()
    {
      // Construct XML from parameters
      auto doc = ref new XmlDocument();

      // Add root node
      auto elem = doc->CreateElement(L"Command");
      doc->AppendChild(elem);

      auto docElem = doc->DocumentElement;
      docElem->SetAttribute(L"Name", commandName);
      for (auto& pair : attributes)
      {
        docElem->SetAttribute(pair->Key, pair->Value);
      }

      auto message = m_igtlMessageFactory->CreateSendMessage("COMMAND", IGTL_HEADER_VERSION_2);
      auto commandMessage = dynamic_cast<igtl::CommandMessage*>(message.GetPointer());
      commandMessage->SetContentEncoding(IANA_TYPE_US_ASCII);
      std::wstring wCmdName(commandName->Data());
      std::string cmdName(begin(wCmdName), end(wCmdName));
      commandMessage->SetCommandName(cmdName);

      std::wstring wCmdContent(docElem->GetXml()->Data());
      std::string cmdContent(begin(wCmdContent), end(wCmdContent));
      commandMessage->SetCommandContent(cmdContent);

      igtl::MessageBase::Pointer* messageBasePointer = (igtl::MessageBase::Pointer*)(commandMessage);
      return SendCommandAsyncInternal(*messageBasePointer);
    });
  }

  //----------------------------------------------------------------------------
  bool IGTClient::IsCommandComplete(uint32 commandId)
  {
    return std::find(begin(m_outstandingQueries), end(m_outstandingQueries), commandId) == end(m_outstandingQueries);
  }

  //----------------------------------------------------------------------------
  void IGTClient::DataReceiverPump()
  {
    IGT_LOG_TRACE(L"IGTLinkClient::DataReceiverPump");

    auto headerMsg = m_igtlMessageFactory->CreateHeaderMessage(IGTL_HEADER_VERSION_1);
    auto token = m_receiverPumpTokenSource.get_token();

    while (!token.is_canceled())
    {
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
        IGT_LOG_TRACE("Corruption in the message header. Serious error.");
        continue;
      }

      if (bodyMsg.IsNull())
      {
        IGT_LOG_TRACE("Unable to create message of type: " << headerMsg->GetMessageType());
        continue;
      }

      // Accept all messages but status messages, they are used as a keep alive mechanism
      if (typeid(*bodyMsg) == typeid(igtl::TrackedFrameMessage))
      {
        SocketReceive(bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize());

        c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          IGT_LOG_TRACE("Failed to receive reply (invalid body)");
          continue;
        }

        igtl::TrackedFrameMessage* trackedFrameMessage = (igtl::TrackedFrameMessage*)bodyMsg.GetPointer();

        // Post process tracked frame to adjust for unit scale
        trackedFrameMessage->ApplyTransformUnitScaling(m_trackerUnitScale);

        // Save reply
        std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
        m_receiveMessages.push_back(bodyMsg);
      }
      else if (typeid(*bodyMsg) == typeid(igtl::TrackingDataMessage))
      {
        if (bodyMsg->GetBufferBodySize() == 0)
        {
          continue;
        }
        SocketReceive(bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize());

        c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          IGT_LOG_TRACE("Failed to receive reply (invalid body)");
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
        m_receiveMessages.push_back(bodyMsg);
      }
      else if (typeid(*bodyMsg) == typeid(igtl::TransformMessage))
      {
        SocketReceive(bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize());

        c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          IGT_LOG_TRACE("Failed to receive reply (invalid body)");
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
        m_receiveMessages.push_back(bodyMsg);
      }
      else if (typeid(*bodyMsg) == typeid(igtl::PolyDataMessage))
      {
        // We got ourselves a live one! 3D model sent over the network
        SocketReceive(bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize());

        c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          IGT_LOG_TRACE("Failed to receive reply (invalid body)");
          continue;
        }

        // Save reply
        std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
        m_receiveMessages.push_back(bodyMsg);
      }
      else if (typeid(*bodyMsg) == typeid(igtl::RTSCommandMessage))
      {
        SocketReceive(bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize());

        c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          IGT_LOG_TRACE("Failed to receive reply (invalid body)");
          continue;
        }

        auto rtsCmdMsg = (igtl::RTSCommandMessage*)bodyMsg.GetPointer();

        // Clear from outstanding queries
        std::lock_guard<std::mutex> guard(m_queriesMutex);
        for (auto iter = begin(m_outstandingQueries); iter != end(m_outstandingQueries); ++iter)
        {
          if ((*iter) == rtsCmdMsg->GetCommandId())
          {
            m_outstandingQueries.erase(iter);
            break;
          }
        }
      }
      else
      {
        // if the incoming message is not a reply to a command, we discard it and continue
        std::lock_guard<std::mutex> socketGuard(m_socketMutex);
        IGT_LOG_TRACE("Received message: " << bodyMsg->GetMessageType() << " (not processed)");
        SocketReceive(nullptr, bodyMsg->GetBodySizeToRead());
      }

      PruneIGTMessages();
    }

    return;
  }

  //----------------------------------------------------------------------------
  void IGTClient::PruneIGTMessages()
  {
    std::lock_guard<std::mutex> guard(m_receiveMessagesMutex);
    if (m_receiveMessages.size() > MESSAGE_LIST_MAX_SIZE + 50)
    {
      // erase the front N results
      MessageList::size_type toErase = m_receiveMessages.size() - MESSAGE_LIST_MAX_SIZE;
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
      if (dest != nullptr)
      {
        auto header = GetDataFromIBuffer<byte>(buffer);
        memcpy(dest, header, size);
      }
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