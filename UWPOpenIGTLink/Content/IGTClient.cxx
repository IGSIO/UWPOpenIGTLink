/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.

Modified by Adam Rankin, Robarts Research Institute, 2017

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

=========================================================Plus=header=end*/

// Local includes
#include "pch.h"
#include "IGTClient.h"
#include "IGTCommon.h"
#include "TrackedFrameMessage.h"

// IGT includes
#include <igtlCommandMessage.h>
#include <igtlCommon.h>
#include <igtlImageMessage.h>
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
  // TODO tune
  const BufferItemList::size_type IGTClient::MESSAGE_LIST_IMAGE_MAX_SIZE = 200;
  const BufferItemList::size_type IGTClient::MESSAGE_LIST_TRACKEDFRAME_MAX_SIZE = 200;
  const BufferItemList::size_type IGTClient::MESSAGE_LIST_COMMANDREPLY_MAX_SIZE = 200;
  const BufferItemList::size_type IGTClient::MESSAGE_LIST_TRANSFORM_MAX_SIZE = 200;
  const BufferItemList::size_type IGTClient::MESSAGE_LIST_POLYDATA_MAX_SIZE = 200;
  const BufferItemList::size_type IGTClient::MESSAGE_LIST_TDATA_MAX_SIZE = 200;

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
      std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
      if (m_receivedTrackedFrameMessages.size() == 0)
      {
        return nullptr;
      }
      trackedFrameMsg = dynamic_cast<igtl::TrackedFrameMessage*>(m_receivedTrackedFrameMessages.rbegin()->GetPointer());
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
  UWPOpenIGTLink::VideoFrame^ IGTClient::GetImage(double lastKnownTimestamp)
  {
    igtl::ImageMessage::Pointer imgMsg = nullptr;
    {
      // Retrieve the next available tracked frame message
      std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
      if (m_receivedImageMessages.size() == 0)
      {
        return nullptr;
      }
      imgMsg = dynamic_cast<igtl::ImageMessage*>(m_receivedImageMessages.rbegin()->GetPointer());
    }

    auto ts = igtl::TimeStamp::New();
    imgMsg->GetTimeStamp(ts);

    if (ts->GetTimeStamp() <= lastKnownTimestamp)
    {
      return nullptr;
    }

    auto frame = ref new VideoFrame();

    // Image
    std::array<int32, 3> frameSize;
    imgMsg->GetDimensions(frameSize[0], frameSize[1], frameSize[2]);
    std::array<uint16, 3> frameSizeUint;
    frameSizeUint[0] = static_cast<uint16>(frameSize[0]); // We are guaranteed that these will fit as the underlying image message stores image size as uint16 (interface shouldn't use int32)
    frameSizeUint[1] = static_cast<uint16>(frameSize[1]);
    frameSizeUint[2] = static_cast<uint16>(frameSize[2]);
    std::shared_ptr<byte> imgData = std::shared_ptr<byte>(new byte[imgMsg->GetImageSize()], [](byte * p) {delete[] p; });
    memcpy((void*)imgData.get(), imgMsg->GetScalarPointer(), imgMsg->GetImageSize());

    frame->SetImageData(imgData, static_cast<uint16>(imgMsg->GetNumComponents()), (IGTL_SCALAR_TYPE)imgMsg->GetScalarType(), frameSizeUint);
    frame->Type = US_IMG_BRIGHTNESS; // Not perfect, but this data isn't transmitted with an image message, could check metadata?
    frame->Orientation = US_IMG_ORIENT_MF;

    frame->EmbeddedImageTransformName = m_embeddedImageTransformName;
    frame->EmbeddedImageTransform = float4x4::identity();

    int   size[3];          // image dimension
    float spacing[3];       // spacing (mm/pixel)
    igtl::Matrix4x4 matrix;
    imgMsg->GetDimensions(size);
    imgMsg->GetSpacing(spacing);
    imgMsg->GetMatrix(matrix);

    float tx = matrix[0][0];
    float ty = matrix[1][0];
    float tz = matrix[2][0];
    float sx = matrix[0][1];
    float sy = matrix[1][1];
    float sz = matrix[2][1];
    float nx = matrix[0][2];
    float ny = matrix[1][2];
    float nz = matrix[2][2];
    float px = matrix[0][3];
    float py = matrix[1][3];
    float pz = matrix[2][3];

    // normalize
    float psi = sqrt(tx * tx + ty * ty + tz * tz);
    float psj = sqrt(sx * sx + sy * sy + sz * sz);
    float psk = sqrt(nx * nx + ny * ny + nz * nz);
    float ntx = tx / psi;
    float nty = ty / psi;
    float ntz = tz / psi;
    float nsx = sx / psj;
    float nsy = sy / psj;
    float nsz = sz / psj;
    float nnx = nx / psk;
    float nny = ny / psk;
    float nnz = nz / psk;

    // Shift the center
    // NOTE: The center of the image should be shifted due to different
    // definitions of image origin between VTK (Slicer) and OpenIGTLink;
    // OpenIGTLink image has its origin at the center, while VTK image
    // has one at the corner.
    float hfovi = spacing[0] * psi * (size[0] - 1) / 2.0;
    float hfovj = spacing[1] * psj * (size[1] - 1) / 2.0;
    float hfovk = spacing[2] * psk * (size[2] - 1) / 2.0;

    float cx = ntx * hfovi + nsx * hfovj + nnx * hfovk;
    float cy = nty * hfovi + nsy * hfovj + nny * hfovk;
    float cz = ntz * hfovi + nsz * hfovj + nnz * hfovk;
    px = px - cx;
    py = py - cy;
    pz = pz - cz;

    // set volume orientation
    float4x4 ijk2ras(float4x4::identity());
    ijk2ras.m11 = ntx * spacing[0];
    ijk2ras.m21 = nty * spacing[0];
    ijk2ras.m31 = ntz * spacing[0];
    ijk2ras.m12 = nsx * spacing[1];
    ijk2ras.m22 = nsy * spacing[1];
    ijk2ras.m32 = nsz * spacing[1];
    ijk2ras.m13 = nnx * spacing[2];
    ijk2ras.m23 = nny * spacing[2];
    ijk2ras.m33 = nnz * spacing[2];
    ijk2ras.m14 = px * m_trackerUnitScale;
    ijk2ras.m24 = py * m_trackerUnitScale;
    ijk2ras.m34 = pz * m_trackerUnitScale;

    frame->EmbeddedImageTransform = ijk2ras;

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
      std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
      if (m_receivedTDataMessages.size() == 0)
      {
        return nullptr;
      }
      tdataMsg = dynamic_cast<igtl::TrackingDataMessage*>(m_receivedTDataMessages.rbegin()->GetPointer());
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
      std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
      if (m_receivedTransformMessages.size() == 0)
      {
        return nullptr;
      }
      for (auto riter = m_receivedTransformMessages.rbegin(); riter != m_receivedTransformMessages.rend(); ++riter)
      {
        if (nameStr.compare((*riter)->GetDeviceName()) == 0)
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
      std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
      if (m_receivedCommandReplyMessages.size() == 0)
      {
        return nullptr;
      }
      for (auto riter = m_receivedCommandReplyMessages.rbegin(); riter != m_receivedCommandReplyMessages.rend(); ++riter)
      {
        rtsCommandMsg = dynamic_cast<igtl::RTSCommandMessage*>(riter->GetPointer());
        if (rtsCommandMsg->GetCommandId() == commandId)
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
      // Retrieve the next available polydata message
      std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
      if (m_receivedPolydataMessages.size() == 0)
      {
        return nullptr;
      }
      for (auto riter = m_receivedPolydataMessages.rbegin(); riter != m_receivedPolydataMessages.rend(); ++riter)
      {
        std::string fileName;
        if (!(*riter)->GetMetaDataElement("fileName", fileName))
        {
          continue;
        }

        if (IsEqualInsensitive(wname, name) == 0)
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
      polydata->Points = posList;
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

    // If colours are present
    if (polyMessage->GetAttribute(igtl::PolyDataAttribute::POINT_RGBA) != nullptr)
    {
      auto colourList = ref new Platform::Collections::Vector<float4>();
      auto& entries = *polyMessage->GetAttribute(igtl::PolyDataAttribute::POINT_RGBA);
      for (uint32 i = 0; i < entries.GetSize(); ++i)
      {
        std::vector<float> rgba;
        entries.GetNthData(i, rgba);
        assert(rgba.size() == 4);
        colourList->Append(float4(rgba[0], rgba[1], rgba[2], rgba[3]));
      }
      polydata->Colours = colourList;
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

    for (auto keyEncPair : polyMessage->GetMetaData())
    {
      auto key = keyEncPair.first;
      auto val = keyEncPair.second.second;
      if (IsEqualInsensitive(key, "Ka"))
      {
        float4 amb(0.f, 0.f, 0.f, 1.f);
        std::istringstream ss(val);
        ss >> amb.x >> amb.y >> amb.z;
        polydata->Mat->Ambient = amb;
      }
      else if (IsEqualInsensitive(key, "Kd"))
      {
        float4 diffuse(0.f, 0.f, 0.f, 1.f);
        std::istringstream ss(val);
        ss >> diffuse.x >> diffuse.y >> diffuse.z;
        polydata->Mat->Diffuse = diffuse;
      }
      else if (IsEqualInsensitive(key, "Ks"))
      {
        float4 spec(0.f, 0.f, 0.f, 1.f);
        std::istringstream ss(val);
        ss >> spec.x >> spec.y >> spec.z;
        polydata->Mat->Specular = spec;
      }
      else if (IsEqualInsensitive(key, "Ke"))
      {
        float4 emissive(0.f, 0.f, 0.f, 1.f);
        std::istringstream ss(val);
        ss >> emissive.x >> emissive.y >> emissive.z;
        polydata->Mat->Emissive = emissive;
      }
      else if (IsEqualInsensitive(key, "Tr"))
      {
        float transparency;
        std::istringstream ss(val);
        ss >> transparency;
        polydata->Mat->Transparency = transparency;
      }
      else if (IsEqualInsensitive(key, "illum"))
      {
        int32 model;
        std::istringstream ss(val);
        ss >> model;
        polydata->Mat->Model = static_cast<IlluminationModel>(model);
      }
      else if (IsEqualInsensitive(key, "Ns"))
      {
        float specExp;
        std::istringstream ss(val);
        ss >> specExp;
        polydata->Mat->SpecularExponent = specExp;
      }
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
  Concurrency::task<CommandData> IGTClient::SendCommandAsyncInternal(igtl::CommandMessage::Pointer packedMessage)
  {
    // Keep track of requested message
    packedMessage->SetCommandId(m_nextQueryId);

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
    if (!Connected)
    {
      return create_async([this]()
      {
        return false;
      });
    }

    return create_async([this, messageBasePointerPtr]()
    {
      igtl::MessageBase::Pointer* messageBasePointer = (igtl::MessageBase::Pointer*)(messageBasePointerPtr);
      return SendMessageAsyncInternal(*messageBasePointer);
    });
  }

  //----------------------------------------------------------------------------
  IAsyncOperation<CommandData>^ IGTClient::SendCommandAsync(Platform::String^ commandName, IMap<Platform::String^, Platform::String^>^ attributes)
  {
    if (!this->Connected)
    {
      return create_async([this]()
      {
        CommandData data;
        data.SentSuccessfully = false;
        return data;
      });
    }

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

      return SendCommandAsyncInternal(commandMessage);
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

    m_receiverPumpTokenSource = cancellation_token_source();

    auto headerMsg = m_igtlMessageFactory->CreateHeaderMessage(IGTL_HEADER_VERSION_1);
    auto token = m_receiverPumpTokenSource.get_token();

    while (!token.is_canceled())
    {
      headerMsg->InitBuffer();

      // Receive generic header from the socket
      int numOfBytesReceived = 0;
      {
        numOfBytesReceived = SocketReceive(headerMsg->GetBufferPointer(), headerMsg->GetBufferSize());
        if (numOfBytesReceived == 0)
        {
          // Graceful disconnect, other end closes the connection
          break;
        }
        if (numOfBytesReceived != headerMsg->GetBufferSize())
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
        std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
        m_receivedTrackedFrameMessages.push_back(bodyMsg);
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
        std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
        m_receivedTDataMessages.push_back(bodyMsg);
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
        std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
        m_receivedTransformMessages.push_back(bodyMsg);
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
        std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
        m_receivedPolydataMessages.push_back(bodyMsg);
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

        {
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

        // Save reply
        std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
        m_receivedCommandReplyMessages.push_back(bodyMsg);
      }
      else if (typeid(*bodyMsg) == typeid(igtl::ImageMessage))
      {
        SocketReceive(bodyMsg->GetBufferBodyPointer(), bodyMsg->GetBufferBodySize());

        c = bodyMsg->Unpack(1);
        if (!(c & igtl::MessageHeader::UNPACK_BODY))
        {
          IGT_LOG_TRACE("Failed to receive reply (invalid body)");
          continue;
        }

        auto imgMsg = (igtl::ImageMessage*)bodyMsg.GetPointer();

        // Save reply
        std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
        m_receivedImageMessages.push_back(bodyMsg);
      }
      else
      {
        // if the incoming message is not a reply to a command, we discard it and continue
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
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedImageMessages.size() > MESSAGE_LIST_IMAGE_MAX_SIZE + 4)
    {
      // erase the front N results
      MessageList::size_type toErase = m_receivedImageMessages.size() - MESSAGE_LIST_IMAGE_MAX_SIZE;
      m_receivedImageMessages.erase(begin(m_receivedImageMessages), begin(m_receivedImageMessages) + toErase);
    }

    if (m_receivedTrackedFrameMessages.size() > MESSAGE_LIST_TRACKEDFRAME_MAX_SIZE + 4)
    {
      // erase the front N results
      MessageList::size_type toErase = m_receivedTrackedFrameMessages.size() - MESSAGE_LIST_TRACKEDFRAME_MAX_SIZE;
      m_receivedTrackedFrameMessages.erase(begin(m_receivedTrackedFrameMessages), begin(m_receivedTrackedFrameMessages) + toErase);
    }

    if (m_receivedCommandReplyMessages.size() > MESSAGE_LIST_COMMANDREPLY_MAX_SIZE)
    {
      // erase the front N results
      MessageList::size_type toErase = m_receivedCommandReplyMessages.size() - MESSAGE_LIST_COMMANDREPLY_MAX_SIZE;
      m_receivedCommandReplyMessages.erase(begin(m_receivedCommandReplyMessages), begin(m_receivedCommandReplyMessages) + toErase);
    }

    if (m_receivedTransformMessages.size() > MESSAGE_LIST_TRANSFORM_MAX_SIZE + 30)
    {
      // erase the front N results
      MessageList::size_type toErase = m_receivedTransformMessages.size() - MESSAGE_LIST_TRANSFORM_MAX_SIZE;
      m_receivedTransformMessages.erase(begin(m_receivedTransformMessages), begin(m_receivedTransformMessages) + toErase);
    }

    if (m_receivedPolydataMessages.size() > MESSAGE_LIST_POLYDATA_MAX_SIZE + 2)
    {
      // erase the front N results
      MessageList::size_type toErase = m_receivedPolydataMessages.size() - MESSAGE_LIST_POLYDATA_MAX_SIZE;
      m_receivedPolydataMessages.erase(begin(m_receivedPolydataMessages), begin(m_receivedPolydataMessages) + toErase);
    }

    if (m_receivedTDataMessages.size() > MESSAGE_LIST_TDATA_MAX_SIZE + 15)
    {
      // erase the front N results
      MessageList::size_type toErase = m_receivedTDataMessages.size() - MESSAGE_LIST_TDATA_MAX_SIZE;
      m_receivedTDataMessages.erase(begin(m_receivedTDataMessages), begin(m_receivedTDataMessages) + toErase);
    }
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestTrackedFrameTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedTrackedFrameMessages.size() == 0)
    {
      return -1;
    }

    igtl::TrackedFrameMessage* img = dynamic_cast<igtl::TrackedFrameMessage*>((*m_receivedTrackedFrameMessages.end()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestTrackedFrameTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedTrackedFrameMessages.size() == 0)
    {
      return -1;
    }

    igtl::TrackedFrameMessage* img = dynamic_cast<igtl::TrackedFrameMessage*>((*m_receivedTrackedFrameMessages.begin()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestTDataTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedTDataMessages.size() == 0)
    {
      return -1;
    }

    igtl::TrackingDataMessage* img = dynamic_cast<igtl::TrackingDataMessage*>((*m_receivedTDataMessages.end()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestTDataTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedTDataMessages.size() == 0)
    {
      return -1;
    }

    igtl::TrackingDataMessage* img = dynamic_cast<igtl::TrackingDataMessage*>((*m_receivedTDataMessages.begin()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestPolydataTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedPolydataMessages.size() == 0)
    {
      return -1;
    }

    igtl::PolyDataMessage* img = dynamic_cast<igtl::PolyDataMessage*>((*m_receivedPolydataMessages.end()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestPolydataTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedPolydataMessages.size() == 0)
    {
      return -1;
    }

    igtl::PolyDataMessage* img = dynamic_cast<igtl::PolyDataMessage*>((*m_receivedPolydataMessages.begin()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestImageTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedImageMessages.size() == 0)
    {
      return -1;
    }

    igtl::ImageMessage* img = dynamic_cast<igtl::ImageMessage*>((*m_receivedImageMessages.end()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestImageTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedImageMessages.size() == 0)
    {
      return -1;
    }

    igtl::ImageMessage* img = dynamic_cast<igtl::ImageMessage*>((*m_receivedImageMessages.begin()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestCommandReplyTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedCommandReplyMessages.size() == 0)
    {
      return -1;
    }

    igtl::RTSCommandMessage* img = dynamic_cast<igtl::RTSCommandMessage*>((*m_receivedCommandReplyMessages.end()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetOldestCommandReplyTimestamp() const
  {
    std::lock_guard<std::mutex> guard(m_receivedMessagesMutex);
    if (m_receivedCommandReplyMessages.size() == 0)
    {
      return -1;
    }

    igtl::RTSCommandMessage* img = dynamic_cast<igtl::RTSCommandMessage*>((*m_receivedCommandReplyMessages.begin()).GetPointer());
    auto ts = igtl::TimeStamp::New();
    img->GetTimeStamp(ts);
    return ts->GetTimeStamp();
  }

  //----------------------------------------------------------------------------
  double IGTClient::GetLatestTransformTimestamp(const std::wstring& name) const
  {
    auto str = std::string(begin(name), end(name));

    // Retrieve the next available tracked frame reply
    if (m_receivedTransformMessages.size() > 0)
    {
      igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
      for (auto riter = m_receivedTransformMessages.rbegin(); riter != m_receivedTransformMessages.rend(); riter++)
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
    if (m_receivedTransformMessages.size() > 0)
    {
      igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
      for (auto iter = m_receivedTransformMessages.begin(); iter != m_receivedTransformMessages.end(); iter++)
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
    auto loadTask = create_task(m_readStream->LoadAsync(size));
    int bytesLoaded(-1);
    try
    {
      bytesLoaded = loadTask.get();
      if (bytesLoaded != size)
      {
        return bytesLoaded;
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

    return bytesLoaded;
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