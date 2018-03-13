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

#include "pch.h"
#include "Buffer.h"

namespace
{
  static const float NEGLIGIBLE_TIME_DIFFERENCE = 0.00001f; // in seconds, used for comparing between exact timestamps
  static const float ANGLE_INTERPOLATION_WARNING_THRESHOLD_DEG = 10.f; // if the interpolated orientation differs from both the interpolated orientation by more than this threshold then display a warning
}

using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  Buffer::Buffer()
  {
    this->SetBufferSize(50);
  }

  //----------------------------------------------------------------------------
  Buffer::~Buffer()
  {
  }

  //----------------------------------------------------------------------------
  bool Buffer::AllocateMemoryForFrames()
  {
    std::lock_guard<std::recursive_mutex> guard(this->BufferMutex);
    bool result = true;

    for (uint32 i = 0; i < this->StreamBuffer->GetBufferSize(); ++i)
    {
      if (this->StreamBuffer->GetBufferItemFromBufferIndex(i)->GetFrame()->AllocateFrame(this->GetFrameSize(), this->GetPixelType(), this->GetNumberOfScalarComponents()) != true)
      {
        OutputDebugStringA((std::string("Failed to allocate memory for frame ") + std::to_string(i)).c_str());
        result = false;
      }
    }
    return result;
  }

  //----------------------------------------------------------------------------
  void Buffer::SetLocalTimeOffsetSec(float offsetSec)
  {
    this->StreamBuffer->SetLocalTimeOffsetSec(offsetSec);
  }

  //----------------------------------------------------------------------------
  float Buffer::GetLocalTimeOffsetSec()
  {
    return this->StreamBuffer->GetLocalTimeOffsetSec();
  }

  //----------------------------------------------------------------------------
  Platform::Array<uint16>^ Buffer::GetFrameSize()
  {
    return ref new Platform::Array<uint16>(3) { FrameSize[0], FrameSize[1], FrameSize[2] };
  }

  //----------------------------------------------------------------------------
  int Buffer::GetImageType()
  {
    return this->ImageType;
  }

  //----------------------------------------------------------------------------
  BufferItemList::size_type Buffer::GetBufferSize()
  {
    return this->StreamBuffer->GetBufferSize();
  }

  //----------------------------------------------------------------------------
  bool Buffer::SetBufferSize(BufferItemList::size_type bufsize)
  {
    if (bufsize < 0)
    {
      OutputDebugStringA((std::string("Invalid buffer size requested: ") + std::to_string(bufsize)).c_str());
      return false;
    }
    if (this->StreamBuffer->GetBufferSize() == bufsize)
    {
      // no change
      return true;
    }

    bool result = true;
    if (this->StreamBuffer->SetBufferSize(bufsize) != true)
    {
      result = false;
    }
    if (this->AllocateMemoryForFrames() != true)
    {
      return false;
    }

    return result;
  }

  //----------------------------------------------------------------------------
  bool Buffer::CheckFrameFormat(const std::array<uint16, 3>& frameSize, int pixelType, int imgType, uint16 numberOfScalarComponents)
  {
    // don't add a frame if it doesn't match the buffer frame format
    if (frameSize[0] != this->GetFrameSize()[0] ||
        frameSize[1] != this->GetFrameSize()[1] ||
        frameSize[2] != this->GetFrameSize()[2])
    {
      std::stringstream ss;
      ss << "Frame format and buffer frame format does not match (expected frame size: " <<
         this->GetFrameSize()[0] << "x" << this->GetFrameSize()[1] << "x" << this->GetFrameSize()[2] <<
         "  received: " << frameSize[0] << "x" << frameSize[1] << "x" << frameSize[2] << ")!";
      OutputDebugStringA(ss.str().c_str());
      return false;
    }

    if (pixelType != this->GetPixelType())
    {
      OutputDebugStringW((L"Frame pixel type (" + VideoFrame::GetStringFromScalarPixelType(pixelType) + L") and buffer pixel type (" + VideoFrame::GetStringFromScalarPixelType(this->GetPixelType()) + L") mismatch")->Data());
      return false;
    }

    if (imgType != this->GetImageType())
    {
      OutputDebugStringW((L"Frame image type (" + VideoFrame::GetStringFromUsImageType(imgType) + L") and buffer image type (" + VideoFrame::GetStringFromUsImageType(this->GetImageType()) + L") mismatch")->Data());
      return false;
    }

    if (numberOfScalarComponents != this->GetNumberOfScalarComponents())
    {
      OutputDebugStringA((std::string("Frame number of scalar components (") + std::to_string(numberOfScalarComponents) + ") and buffer number of components (" + std::to_string(this->GetNumberOfScalarComponents()) + ") mismatch").c_str());
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool Buffer::AddItem(Image^ image,
                       int usImageOrientation,
                       int imageType,
                       uint32 frameNumber,
                       const Platform::Array<int>^ clipRectangleOrigin,
                       const Platform::Array<int>^ clipRectangleSize,
                       FrameFieldsABI^ frameFields,
                       float unfilteredTimestamp,
                       float filteredTimestamp)
  {
    if (image == nullptr)
    {
      OutputDebugStringA("Buffer: Unable to add NULL frame to video buffer!");
      return false;
    }

    if (unfilteredTimestamp == UNDEFINED_TIMESTAMP)
    {
      Calendar->SetToNow();
      unfilteredTimestamp = 0.001f * Calendar->GetDateTime().UniversalTime;
    }

    if (filteredTimestamp == UNDEFINED_TIMESTAMP)
    {
      bool filteredTimestampProbablyValid = true;
      if (this->StreamBuffer->CreateFilteredTimeStampForItem(frameNumber, unfilteredTimestamp, &filteredTimestamp, &filteredTimestampProbablyValid) != true)
      {
        OutputDebugStringA((std::string("Failed to create filtered timestamp for video buffer item with item index: ") + std::to_string(frameNumber)).c_str());
        return false;
      }
      if (!filteredTimestampProbablyValid)
      {
        OutputDebugStringA((std::string("Filtered timestamp is probably invalid for video buffer item with item index=") + std::to_string(frameNumber) + ", time=" +
                            std::to_string(unfilteredTimestamp) + ". The item may have been tagged with an inaccurate timestamp, therefore it will not be recorded.").c_str());
        return true;
      }
    }

    auto frameSize = image->GetFrameSize();
    if (!this->CheckFrameFormat(frameSize, image->ScalarType, imageType, image->NumberOfScalarComponents))
    {
      OutputDebugStringA("Buffer: Unable to add frame to video buffer - frame format doesn't match!");
      return false;
    }

    BufferItemList::size_type bufferIndex(0);
    BufferItemUidType itemUid;
    std::lock_guard<std::recursive_mutex> guard(this->BufferMutex);
    if (this->StreamBuffer->PrepareForNewItem(filteredTimestamp, &itemUid, &bufferIndex) != true)
    {
      // Just a debug message, because we want to avoid unnecessary warning messages if the timestamp is the same as last one
      OutputDebugStringA("Buffer: Failed to prepare for adding new frame to video buffer!");
      return false;
    }

    // get the pointer to the correct location in the frame buffer, where this data needs to be copied
    StreamBufferItem^ newObjectInBuffer = this->StreamBuffer->GetBufferItemFromBufferIndex(bufferIndex);
    if (newObjectInBuffer == nullptr)
    {
      OutputDebugStringA("Buffer: Failed to get pointer to video buffer object from the video buffer for the new frame!");
      return false;
    }

    const auto& bufferItemFrameSize = newObjectInBuffer->GetFrame()->GetDimensions();

    if (frameSize[0] != bufferItemFrameSize[0] || frameSize[1] != bufferItemFrameSize[1] || frameSize[2] != bufferItemFrameSize[2])
    {
      std::stringstream ss;
      ss << "Input frame size is different from buffer frame size (input: " << frameSize[0] << "x" << frameSize[1] << "x" << frameSize[2] << ",   buffer: " << bufferItemFrameSize[0] << "x" << bufferItemFrameSize[1] << "x" << bufferItemFrameSize[2] << ")!";
      OutputDebugStringA(ss.str().c_str());
      return false;
    }

    // Shallow set image pointer, ref count will increase
    newObjectInBuffer->GetFrame()->ShallowCopy(image, (US_IMAGE_ORIENTATION)usImageOrientation, (US_IMAGE_TYPE)imageType);

    newObjectInBuffer->SetFilteredTimestamp(filteredTimestamp);
    newObjectInBuffer->SetUnfilteredTimestamp(unfilteredTimestamp);
    newObjectInBuffer->SetIndex(frameNumber);
    newObjectInBuffer->SetUid(itemUid);
    newObjectInBuffer->GetFrame()->SetImageType(imageType);

    // Add custom fields
    for (auto field : frameFields)
    {
      newObjectInBuffer->SetCustomFrameField(field->Key, field->Value);
      std::wstring name(field->Key->Data());
      if (name.find(L"Transform") != std::string::npos)
      {
        newObjectInBuffer->SetValidTransformData(true);
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool Buffer::AddItem(VideoFrame^ frame,
                       uint32 frameNumber,
                       const Platform::Array<int>^ clipRectangleOrigin,
                       const Platform::Array<int>^ clipRectangleSize,
                       FrameFieldsABI^ frameFields,
                       float unfilteredTimestamp,
                       float filteredTimestamp)
  {
    if (frame == nullptr)
    {
      OutputDebugStringA("Buffer: Unable to add NULL frame to video buffer!");
      return false;
    }

    return this->AddItem(frame->Image, frame->Orientation, frame->Type, frameNumber, clipRectangleOrigin, clipRectangleSize, frameFields, unfilteredTimestamp, filteredTimestamp);
  }

  //----------------------------------------------------------------------------
  bool Buffer::AddItem(FrameFieldsABI^ frameFields, uint32 frameNumber, float unfilteredTimestamp, float filteredTimestamp)
  {
    if (frameFields->Size == 0)
    {
      return true;
    }

    if (unfilteredTimestamp == UNDEFINED_TIMESTAMP)
    {
      Calendar->SetToNow();
      unfilteredTimestamp = 0.001f * Calendar->GetDateTime().UniversalTime;
    }
    if (filteredTimestamp == UNDEFINED_TIMESTAMP)
    {
      bool filteredTimestampProbablyValid = true;
      if (this->StreamBuffer->CreateFilteredTimeStampForItem(frameNumber, unfilteredTimestamp, &filteredTimestamp, &filteredTimestampProbablyValid) != true)
      {
        OutputDebugStringA((std::string("Failed to create filtered timestamp for tracker buffer item with item index: ") + std::to_string(frameNumber)).c_str());
        return false;
      }
      if (!filteredTimestampProbablyValid)
      {
        std::stringstream ss;
        ss << "Filtered timestamp is probably invalid for tracker buffer item with item index=" << frameNumber << ", time=" << unfilteredTimestamp << ". The item may have been tagged with an inaccurate timestamp, therefore it will not be recorded.";
        OutputDebugStringA(ss.str().c_str());
        return true;
      }
    }

    BufferItemList::size_type bufferIndex(0);
    BufferItemUidType itemUid;
    std::lock_guard<std::recursive_mutex> guard(this->BufferMutex);
    if (this->StreamBuffer->PrepareForNewItem(filteredTimestamp, &itemUid, &bufferIndex) != true)
    {
      // Just a debug message, because we want to avoid unnecessary warning messages if the timestamp is the same as last one
      OutputDebugStringA("Buffer: Failed to prepare for adding new frame to tracker buffer!");
      return false;
    }

    // get the pointer to the correct location in the tracker buffer, where this data needs to be copied
    StreamBufferItem^ newObjectInBuffer = this->StreamBuffer->GetBufferItemFromBufferIndex(bufferIndex);
    if (newObjectInBuffer == nullptr)
    {
      OutputDebugStringA("Buffer: Failed to get pointer to data buffer object from the tracker buffer for the new frame!");
      return false;
    }

    newObjectInBuffer->SetFilteredTimestamp(filteredTimestamp);
    newObjectInBuffer->SetUnfilteredTimestamp(unfilteredTimestamp);
    newObjectInBuffer->SetIndex(frameNumber);
    newObjectInBuffer->SetUid(itemUid);

    // Add custom fields
    for (auto field : frameFields)
    {
      newObjectInBuffer->SetCustomFrameField(field->Key, field->Value);
      std::wstring name(field->Key->Data());
      if (name.find(L"Transform") != std::string::npos)
      {
        newObjectInBuffer->SetValidTransformData(true);
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool Buffer::AddTimeStampedItem(float4x4 matrix, int toolStatus, uint32 frameNumber, float unfilteredTimestamp, FrameFieldsABI^ customFields, float filteredTimestamp)
  {
    if (unfilteredTimestamp == UNDEFINED_TIMESTAMP)
    {
      Calendar->SetToNow();
      unfilteredTimestamp = 0.001f * Calendar->GetDateTime().UniversalTime;
    }
    if (filteredTimestamp == UNDEFINED_TIMESTAMP)
    {
      bool filteredTimestampProbablyValid = true;
      if (this->StreamBuffer->CreateFilteredTimeStampForItem(frameNumber, unfilteredTimestamp, &filteredTimestamp, &filteredTimestampProbablyValid) != true)
      {
        OutputDebugStringA((std::string("Failed to create filtered timestamp for tracker buffer item with item index: ") + std::to_string(frameNumber)).c_str());
        return false;
      }
      if (!filteredTimestampProbablyValid)
      {
        std::stringstream ss;
        ss << "Filtered timestamp is probably invalid for tracker buffer item with item index=" << frameNumber << ", time=" << unfilteredTimestamp << ". The item may have been tagged with an inaccurate timestamp, therefore it will not be recorded.";
        OutputDebugStringA(ss.str().c_str());
        return true;
      }
    }

    BufferItemList::size_type bufferIndex(0);
    BufferItemUidType itemUid;
    std::lock_guard<std::recursive_mutex> guard(this->BufferMutex);
    if (this->StreamBuffer->PrepareForNewItem(filteredTimestamp, &itemUid, &bufferIndex) != true)
    {
      // Just a debug message, because we want to avoid unnecessary warning messages if the timestamp is the same as last one
      OutputDebugStringA("Buffer: Failed to prepare for adding new frame to tracker buffer!");
      return false;
    }

    // get the pointer to the correct location in the tracker buffer, where this data needs to be copied
    StreamBufferItem^ newObjectInBuffer = this->StreamBuffer->GetBufferItemFromBufferIndex(bufferIndex);
    if (newObjectInBuffer == nullptr)
    {
      OutputDebugStringA("Buffer: Failed to get pointer to data buffer object from the tracker buffer for the new frame!");
      return false;
    }

    bool itemStatus = newObjectInBuffer->SetMatrix(matrix);
    newObjectInBuffer->SetStatus(itemStatus);
    newObjectInBuffer->SetFilteredTimestamp(filteredTimestamp);
    newObjectInBuffer->SetUnfilteredTimestamp(unfilteredTimestamp);
    newObjectInBuffer->SetIndex(frameNumber);
    newObjectInBuffer->SetUid(itemUid);

    // Add custom fields
    for (auto field : customFields)
    {
      newObjectInBuffer->SetCustomFrameField(field->Key, field->Value);
      std::wstring name(field->Key->Data());
      if (name.find(L"Transform") != std::string::npos)
      {
        newObjectInBuffer->SetValidTransformData(true);
      }
    }

    return itemStatus;
  }

  //----------------------------------------------------------------------------
  int Buffer::GetLatestTimeStamp(float* latestTimestamp)
  {
    try
    {
      *latestTimestamp = this->StreamBuffer->GetLatestTimeStamp();
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableAnymoreException&)
    {
      return ITEM_NOT_AVAILABLE_ANYMORE;
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableYetException&)
    {
      return ITEM_NOT_AVAILABLE_YET;
    }

    return ITEM_OK;
  }

  //----------------------------------------------------------------------------
  int Buffer::GetOldestTimeStamp(float* oldestTimestamp)
  {
    try
    {
      *oldestTimestamp = this->StreamBuffer->GetOldestTimeStamp();
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableAnymoreException&)
    {
      return ITEM_NOT_AVAILABLE_ANYMORE;
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableYetException&)
    {
      return ITEM_NOT_AVAILABLE_YET;
    }

    return ITEM_OK;
  }

  //----------------------------------------------------------------------------
  int Buffer::GetTimeStamp(BufferItemUidType uid, float* timestamp)
  {
    try
    {
      *timestamp = this->StreamBuffer->GetTimeStamp(uid);
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableAnymoreException&)
    {
      return ITEM_NOT_AVAILABLE_ANYMORE;
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableYetException&)
    {
      return ITEM_NOT_AVAILABLE_YET;
    }

    return ITEM_OK;
  }

  //----------------------------------------------------------------------------
  int Buffer::GetIndex(BufferItemUidType uid, BufferItemList::size_type* index)
  {
    try
    {
      *index = this->StreamBuffer->GetIndex(uid);
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableAnymoreException&)
    {
      return ITEM_NOT_AVAILABLE_ANYMORE;
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableYetException&)
    {
      return ITEM_NOT_AVAILABLE_YET;
    }

    return ITEM_OK;
  }

  //----------------------------------------------------------------------------
  int Buffer::GetBufferIndexFromTime(float time, BufferItemList::size_type* bufferIndex)
  {
    try
    {
      *bufferIndex = this->StreamBuffer->GetBufferIndexFromTime(time);
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableAnymoreException&)
    {
      return ITEM_NOT_AVAILABLE_ANYMORE;
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableYetException&)
    {
      return ITEM_NOT_AVAILABLE_YET;
    }

    return ITEM_OK;
  }

  //----------------------------------------------------------------------------
  void Buffer::SetAveragedItemsForFiltering(int averagedItemsForFiltering)
  {
    this->StreamBuffer->SetAveragedItemsForFiltering(averagedItemsForFiltering);
  }

  //----------------------------------------------------------------------------
  int Buffer::GetAveragedItemsForFiltering()
  {
    return this->StreamBuffer->GetAveragedItemsForFiltering();
  }

  //----------------------------------------------------------------------------
  void Buffer::SetStartTime(float startTime)
  {
    this->StreamBuffer->SetStartTime(startTime);
  }

  //----------------------------------------------------------------------------
  float Buffer::GetStartTime()
  {
    return this->StreamBuffer->GetStartTime();
  }

  //----------------------------------------------------------------------------
  StreamBufferItem^ Buffer::GetStreamBufferItem(BufferItemUidType uid)
  {
    std::lock_guard<std::recursive_mutex> guard(this->BufferMutex);
    return this->StreamBuffer->GetBufferItemFromUid(uid);
  }

  //----------------------------------------------------------------------------
  StreamBufferItem^ Buffer::GetLatestStreamBufferItem()
  {
    return this->GetStreamBufferItem(this->GetLatestItemUidInBuffer());
  }

  //----------------------------------------------------------------------------
  StreamBufferItem^ Buffer::GetOldestStreamBufferItem()
  {
    return this->GetStreamBufferItem(this->GetOldestItemUidInBuffer());
  }

  //----------------------------------------------------------------------------
  void Buffer::DeepCopy(Buffer^ buffer)
  {
    this->StreamBuffer->DeepCopy(buffer->StreamBuffer);
    if (buffer->GetFrameSize()[0] != -1 && buffer->GetFrameSize()[1] != -1 && buffer->GetFrameSize()[2] != -1)
    {
      this->SetFrameSize(buffer->GetFrameSize());
    }
    this->SetPixelType(buffer->GetPixelType());
    this->SetImageType(buffer->GetImageType());
    this->SetNumberOfScalarComponents(buffer->GetNumberOfScalarComponents());
    this->SetImageOrientation(buffer->GetImageOrientation());
    this->SetBufferSize(buffer->GetBufferSize());
  }

  //----------------------------------------------------------------------------
  void Buffer::Clear()
  {
    this->StreamBuffer->Clear();
  }

  //----------------------------------------------------------------------------
  bool Buffer::SetFrameSize(uint16 x, uint16 y, uint16 z)
  {
    if (x != 0 && y != 0 && z == 0)
    {
      OutputDebugStringA("Single slice images should have a dimension of z=1");
      z = 1;
    }
    if (this->FrameSize[0] == x && this->FrameSize[1] == y && this->FrameSize[2] == z)
    {
      // no change
      return true;
    }
    this->FrameSize[0] = x;
    this->FrameSize[1] = y;
    this->FrameSize[2] = z;
    return AllocateMemoryForFrames();
  }

  //----------------------------------------------------------------------------
  bool Buffer::SetFrameSize(const Platform::Array<uint16>^ frameSize)
  {
    return SetFrameSize(frameSize[0], frameSize[1], frameSize[2]);
  }

  //----------------------------------------------------------------------------
  bool Buffer::SetPixelType(int pixelType)
  {
    if (pixelType == this->PixelType)
    {
      // no change
      return true;
    }
    this->PixelType = (IGTL_SCALAR_TYPE)pixelType;
    return AllocateMemoryForFrames();
  }

  //----------------------------------------------------------------------------
  bool Buffer::SetNumberOfScalarComponents(uint16 numberOfScalarComponents)
  {
    if (numberOfScalarComponents == this->NumberOfScalarComponents)
    {
      // no change
      return true;
    }
    this->NumberOfScalarComponents = numberOfScalarComponents;
    return AllocateMemoryForFrames();
  }

  //----------------------------------------------------------------------------
  uint16 Buffer::GetNumberOfScalarComponents()
  {
    return this->NumberOfScalarComponents;
  }

  //----------------------------------------------------------------------------
  bool Buffer::SetImageType(int imgType)
  {
    if (imgType < US_IMG_TYPE_XX || imgType >= US_IMG_TYPE_LAST)
    {
      OutputDebugStringW((L"Invalid image type attempted to set in the video buffer: " + VideoFrame::GetStringFromUsImageType(imgType))->Data());
      return false;
    }
    this->ImageType = (US_IMAGE_TYPE)imgType;
    return true;
  }

  //----------------------------------------------------------------------------
  bool Buffer::SetImageOrientation(int imgOrientation)
  {
    if (imgOrientation < US_IMG_ORIENT_XX || imgOrientation >= US_IMG_ORIENT_LAST)
    {
      OutputDebugStringW((L"Invalid image orientation attempted to set in the video buffer: " + VideoFrame::GetStringFromUsImageOrientation(imgOrientation))->Data());
      return false;
    }
    this->ImageOrientation = (US_IMAGE_ORIENTATION)imgOrientation;
    for (uint32 frameNumber = 0; frameNumber < this->StreamBuffer->GetBufferSize(); frameNumber++)
    {
      this->StreamBuffer->GetBufferItemFromBufferIndex(frameNumber)->GetFrame()->SetImageOrientation(imgOrientation);
    }
    return true;
  }

  //----------------------------------------------------------------------------
  int Buffer::GetNumberOfBytesPerScalar()
  {
    return VideoFrame::GetNumberOfBytesPerScalar(GetPixelType());
  }

  //----------------------------------------------------------------------------
  int Buffer::GetNumberOfBytesPerPixel()
  {
    return this->GetNumberOfScalarComponents() * VideoFrame::GetNumberOfBytesPerScalar(GetPixelType());
  }

  //----------------------------------------------------------------------------
  // Returns the two buffer items that are closest previous and next buffer items relative to the specified time.
  // outItemA is the closest item
  IVectorView<StreamBufferItem^>^ Buffer::GetPrevNextBufferItemFromTime(float time)
  {
    std::lock_guard<std::recursive_mutex> guard(this->BufferMutex);
    Vector<StreamBufferItem^>^ outVec = ref new Vector<StreamBufferItem^>();
    StreamBufferItem^ itemA;
    StreamBufferItem^ itemB;

    // The returned item is computed by interpolation between outItemA and outItemB in time. The outItemA is the closest item to the requested time.
    // Accept outItemA (the closest item) as is if it is very close to the requested time.
    // Accept interpolation between outItemA and outItemB if all the followings are true:
    //   - both outItemA and outItemB exist and are valid
    //   - time difference between the requested time and outItemA is below a threshold
    //   - time difference between the requested time and outItemB is below a threshold

    // outItemA is the item that is the closest to the requested time, get its UID and time
    BufferItemUidType itemAuid(0);
    ItemStatus status;
    try
    {
      itemAuid = this->StreamBuffer->GetItemUidFromTime(time);
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableAnymoreException&)
    {
      status = ITEM_NOT_AVAILABLE_ANYMORE;
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableYetException&)
    {
      status = ITEM_NOT_AVAILABLE_YET;
    }

    if (status != ITEM_OK)
    {
      switch (status)
      {
        case ITEM_NOT_AVAILABLE_YET:
        {
          std::stringstream ss;
          ss << "Buffer: Cannot get any item from the data buffer for time: " << std::fixed << time << ". Item is not available yet.";
          OutputDebugStringA(ss.str().c_str());
          break;
        }
        case ITEM_NOT_AVAILABLE_ANYMORE:
        {
          std::stringstream ss;
          ss << "Buffer: Cannot get any item from the data buffer for time: " << std::fixed << time << ". Item is not available anymore.";
          OutputDebugStringA(ss.str().c_str());
          break;
        }
        default:
          break;
      }
      return nullptr;
    }

    try
    {
      itemA = this->GetStreamBufferItem(itemAuid);
    }
    catch (const std::exception&)
    {
      OutputDebugStringA((std::string("Buffer: Failed to get data buffer item with id: ") + std::to_string(itemAuid)).c_str());
      return nullptr;
    }

    // If tracker is out of view, etc. then we don't have a valid before and after the requested time, so we cannot do interpolation
    if (itemA->GetStatus() != TOOL_OK)
    {
      // tracker is out of view, ...
      std::stringstream ss;
      ss << "Buffer: Cannot do data interpolation. The closest item to the requested time (time: " << std::fixed << time << ", uid: " << itemAuid << ") is invalid.";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    float itemAtime(0);
    try
    {
      itemAtime = this->StreamBuffer->GetTimeStamp(itemAuid);
    }
    catch (const std::exception&)
    {
      std::stringstream ss;
      ss << "Buffer: Failed to get data buffer timestamp (time: " << std::fixed << time << ", uid: " << itemAuid << ")";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    // If the time difference is negligible then don't interpolate, just return the closest item
    if (fabs(itemAtime - time) < NEGLIGIBLE_TIME_DIFFERENCE)
    {
      //No need for interpolation, it's very close to the closest element
      itemB = itemA;
      outVec->Append(itemA);
      outVec->Append(itemB);
      return outVec->GetView();
    }

    // If the closest item is too far, then we don't do interpolation
    if (fabs(itemAtime - time) > this->GetMaxAllowedTimeDifference())
    {
      std::stringstream ss;
      ss << "Buffer: Cannot perform interpolation, time difference compared to itemA is too big " << std::fixed << fabs(itemAtime - time) << " ( closest item time: " << itemAtime << ", requested time: " << time << ").";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    // Find the closest item on the other side of the timescale (so that time is between itemAtime and itemBtime)
    BufferItemUidType itemBuid(0);
    if (time < itemAtime)
    {
      // itemBtime < time <itemAtime
      itemBuid = itemAuid - 1;
    }
    else
    {
      // itemAtime < time <itemBtime
      itemBuid = itemAuid + 1;
    }
    if (itemBuid < this->GetOldestItemUidInBuffer() || itemBuid > this->GetLatestItemUidInBuffer())
    {
      // outItemB is not available
      std::stringstream ss;
      ss << "Buffer: Cannot perform interpolation, outItemB is not available " << std::fixed << " ( itemBuid: " << itemBuid << ", oldest UID: " << this->GetOldestItemUidInBuffer() << ", latest UID: " << this->GetLatestItemUidInBuffer();
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    // Get item B details
    float itemBtime(0);
    try
    {
      itemBtime = this->StreamBuffer->GetTimeStamp(itemBuid);
    }
    catch (const std::exception&)
    {
      status = ITEM_UNKNOWN_ERROR;
    }

    if (status != ITEM_OK)
    {
      std::stringstream ss;
      ss << "Cannot do interpolation: Failed to get data buffer timestamp with id: " << itemBuid;
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    // If the next closest item is too far, then we don't do interpolation
    if (fabs(itemBtime - time) > this->GetMaxAllowedTimeDifference())
    {
      std::stringstream ss;
      ss << "Buffer: Cannot perform interpolation, time difference compared to outItemB is too big " << std::fixed << fabs(itemBtime - time) << " ( itemBtime: " << itemBtime << ", requested time: " << time << ").";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    // Get the item
    try
    {
      itemB = this->GetStreamBufferItem(itemBuid);
    }
    catch (const std::exception&)
    {
      status = ITEM_UNKNOWN_ERROR;
    }

    if (status != ITEM_OK)
    {
      std::stringstream ss;
      ss << "Buffer: Failed to get data buffer item with Uid: " << itemBuid;
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }
    // If there is no valid element on the other side of the requested time, then we cannot do an interpolation
    if (itemB->GetStatus() != TOOL_OK)
    {
      std::stringstream ss;
      ss << "Buffer: Cannot get a second element (uid=" << itemBuid << ") on the other side of the requested time (" << std::fixed << time << ")";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    outVec->Append(itemA);
    outVec->Append(itemB);
    return outVec->GetView();
  }

  //----------------------------------------------------------------------------
  StreamBufferItem^ Buffer::GetStreamBufferItemFromTime(float time, int interpolation)
  {
    switch ((DATA_ITEM_TEMPORAL_INTERPOLATION)interpolation)
    {
      case EXACT_TIME:
        return GetStreamBufferItemFromExactTime(time);
      case INTERPOLATED:
        return GetInterpolatedStreamBufferItemFromTime(time);
      case CLOSEST_TIME:
        return GetStreamBufferItemFromClosestTime(time);
      default:
        std::stringstream ss;
        ss << "Unknown interpolation type: " << interpolation << ". Defaulting to exact time request.";
        OutputDebugStringA(ss.str().c_str());
        return GetStreamBufferItemFromExactTime(time);
    }
  }

  //----------------------------------------------------------------------------
  bool Buffer::ModifyBufferItemFrameField(BufferItemUidType uid, Platform::String^ key, Platform::String^ value)
  {
    try
    {
      StreamBufferItem^ item = this->StreamBuffer->GetBufferItemFromUid(uid);
      item->SetCustomFrameField(key, value);
    }
    catch (const std::exception&)
    {
      std::stringstream ss;
      ss << "Buffer: Failed to get data buffer timestamp (time: " << std::fixed << time << ")";
      OutputDebugStringA(ss.str().c_str());
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------------------
  StreamBufferItem^ Buffer::GetStreamBufferItemFromExactTime(float time)
  {
    try
    {
      std::lock_guard<std::recursive_mutex> guard(this->BufferMutex);
      StreamBufferItem^ item = GetStreamBufferItemFromClosestTime(time);

      // If the time difference is not negligible then return with failure
      float itemTime = item->GetFilteredTimestamp(this->StreamBuffer->GetLocalTimeOffsetSec());
      if (fabs(itemTime - time) > NEGLIGIBLE_TIME_DIFFERENCE)
      {
        std::stringstream ss;
        ss << "Buffer: Cannot find an item exactly at the requested time (requested time: " << std::fixed << time << ", item time: " << itemTime << ")";
        OutputDebugStringA(ss.str().c_str());
        return nullptr;
      }

      return item;
    }
    catch (const std::exception&)
    {
      std::stringstream ss;
      ss << "Buffer: Failed to get data buffer timestamp (time: " << std::fixed << time << ")";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }
  }

  //----------------------------------------------------------------------------
  StreamBufferItem^ Buffer::GetStreamBufferItemFromClosestTime(float time)
  {
    try
    {
      std::lock_guard<std::recursive_mutex> guard(this->BufferMutex);
      BufferItemUidType uid = this->StreamBuffer->GetItemUidFromTime(time);
      return this->GetStreamBufferItem(uid);
    }
    catch (const std::exception&)
    {
      std::stringstream ss;
      ss << "Buffer: Failed to get data buffer timestamp (time: " << std::fixed << time << ")";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }
  }

  //----------------------------------------------------------------------------
  // Interpolate the matrix for the given timestamp from the two nearest
  // transforms in the buffer.
  // The rotation is interpolated with SLERP interpolation, and the
  // position is interpolated with linear interpolation.
  // The flags correspond to the closest element.
  StreamBufferItem^ Buffer::GetInterpolatedStreamBufferItemFromTime(float time)
  {
    StreamBufferItem^ bufferItem;
    StreamBufferItem^ itemA;
    StreamBufferItem^ itemB;

    auto vec = GetPrevNextBufferItemFromTime(time);
    if (vec != nullptr)
    {
      itemA = vec->GetAt(0);
      itemB = vec->GetAt(1);

      // cannot get two neighbors, so cannot do interpolation
      // it may be normal (e.g., when tracker out of view), so don't return with an error
      try
      {
        bufferItem = GetStreamBufferItemFromClosestTime(time);
        // Update the timestamp to match the requested time
        bufferItem->SetFilteredTimestamp(time);
        bufferItem->SetUnfilteredTimestamp(time);
        bufferItem->SetStatus(TOOL_MISSING);   // if we return at any point due to an error then it means that the interpolation is not successful, so the item is missing
        return bufferItem;
      }
      catch (const std::exception&)
      {
        std::stringstream ss;
        ss << "Buffer: Failed to get data buffer timestamp (time: " << std::fixed << time << ")";
        OutputDebugStringA(ss.str().c_str());
        return nullptr;
      }
    }

    if (itemA->GetUid() == itemB->GetUid())
    {
      // exact match, no need for interpolation
      bufferItem->DeepCopy(itemA);
      return bufferItem;
    }

    //============== Get item weights ==================

    float itemAtime(0);
    try
    {
      itemAtime = this->StreamBuffer->GetTimeStamp(itemA->GetUid());
    }
    catch (const std::exception&)
    {
      std::stringstream ss;
      ss << "Buffer: Failed to get data buffer timestamp (time: " << std::fixed << time << ", uid: " << itemA->GetUid() << ")";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    float itemBtime(0);
    try
    {
      itemBtime = this->StreamBuffer->GetTimeStamp(itemB->GetUid());
    }
    catch (const std::exception&)
    {
      std::stringstream ss;
      ss << "Buffer: Failed to get data buffer timestamp (time: " << std::fixed << time << ", uid: " << itemB->GetUid() << ")";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    if (fabs(itemAtime - itemBtime) < NEGLIGIBLE_TIME_DIFFERENCE)
    {
      // exact time match, no need for interpolation
      bufferItem->DeepCopy(itemA);
      bufferItem->SetFilteredTimestamp(time);
      bufferItem->SetUnfilteredTimestamp(time);
      return bufferItem;
    }

    float itemAweight = fabs(itemBtime - time) / fabs(itemAtime - itemBtime);
    float itemBweight = 1 - itemAweight;

    //============== Get transform matrices ==================
    float4x4 itemAmatrix;
    try
    {
      itemAmatrix = itemA->GetMatrix();
    }
    catch (const std::exception&)
    {
      std::stringstream ss;
      ss << "Failed to get item A matrix";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    float4x4 itemBmatrix;
    try
    {
      itemBmatrix = itemB->GetMatrix();
    }
    catch (const std::exception&)
    {
      std::stringstream ss;
      ss << "Failed to get item B matrix";
      OutputDebugStringA(ss.str().c_str());
      return nullptr;
    }

    float3 aScale;
    float3 aTranslation;
    quaternion aRotation;
    decompose(itemAmatrix, &aScale, &aRotation, &aTranslation);

    float3 bScale;
    float3 bTranslation;
    quaternion bRotation;
    decompose(itemBmatrix, &bScale, &bRotation, &bTranslation);

    //============== Interpolate rotation ==================
    auto translation = transpose(make_float4x4_translation(itemAweight * aTranslation + itemBweight * bTranslation));
    auto scale = transpose(make_float4x4_scale(itemAweight * aScale + itemBweight * bScale));
    auto rotation = transpose(make_float4x4_from_quaternion(slerp(aRotation, bRotation, itemBweight)));

    // TODO : verify correctness
    auto interpolatedMatrix = rotation * translation * scale;

    //============== Interpolate time ==================

    float itemAunfilteredTimestamp = itemA->GetUnfilteredTimestamp(0.0);   // 0.0 because timestamps in the buffer are in local time
    float itemBunfilteredTimestamp = itemB->GetUnfilteredTimestamp(0.0);   // 0.0 because timestamps in the buffer are in local time
    float interpolatedUnfilteredTimestamp = itemAunfilteredTimestamp * itemAweight + itemBunfilteredTimestamp * itemBweight;

    //============== Write interpolated results into the bufferItem ==================

    bufferItem->DeepCopy(itemA);
    bufferItem->SetMatrix(interpolatedMatrix);
    bufferItem->SetFilteredTimestamp(time - this->StreamBuffer->GetLocalTimeOffsetSec());   // global = local + offset => local = global - offset
    bufferItem->SetUnfilteredTimestamp(interpolatedUnfilteredTimestamp);

    float angleDiffA = GetOrientationDifference(interpolatedMatrix, itemAmatrix);
    float angleDiffB = GetOrientationDifference(interpolatedMatrix, itemBmatrix);
    if (fabs(angleDiffA) > ANGLE_INTERPOLATION_WARNING_THRESHOLD_DEG && fabs(angleDiffB) > ANGLE_INTERPOLATION_WARNING_THRESHOLD_DEG)
    {
      InterpolatedAngleExceededThreshold(this, angleDiffA, angleDiffB, ANGLE_INTERPOLATION_WARNING_THRESHOLD_DEG);
    }

    return bufferItem;
  }

  //-----------------------------------------------------------------------------
  bool Buffer::GetLatestItemHasValidVideoData()
  {
    return this->StreamBuffer->GetLatestItemHasValidVideoData();
  }

  //-----------------------------------------------------------------------------
  bool Buffer::GetLatestItemHasValidTransformData()
  {
    return this->StreamBuffer->GetLatestItemHasValidTransformData();
  }

  //----------------------------------------------------------------------------
  bool Buffer::GetLatestItemHasValidFieldData()
  {
    return this->StreamBuffer->GetLatestItemHasValidFieldData();
  }

  //----------------------------------------------------------------------------
  UWPOpenIGTLink::BufferItemUidType Buffer::GetOldestItemUidInBuffer()
  {
    return this->StreamBuffer->GetOldestItemUidInBuffer();
  }

  //----------------------------------------------------------------------------
  UWPOpenIGTLink::BufferItemUidType Buffer::GetLatestItemUidInBuffer()
  {
    return this->StreamBuffer->GetLatestItemUidInBuffer();
  }

  //----------------------------------------------------------------------------
  int Buffer::GetItemUidFromTime(float time, BufferItemUidType* outUid)
  {
    try
    {
      *outUid = this->StreamBuffer->GetItemUidFromTime(time);
      return ITEM_OK;
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableAnymoreException&)
    {
      return ITEM_NOT_AVAILABLE_ANYMORE;
    }
    catch (const UWPOpenIGTLink::ItemNotAvailableYetException&)
    {
      return ITEM_NOT_AVAILABLE_YET;
    }
  }

  //----------------------------------------------------------------------------
  BufferItemList::size_type Buffer::GetNumberOfItems()
  {
    return this->StreamBuffer->GetNumberOfItems();
  }

  //----------------------------------------------------------------------------
  float Buffer::GetFrameRate(bool ideal, float* outFramePeriodStdevSec)
  {
    return this->StreamBuffer->GetFrameRate(ideal, outFramePeriodStdevSec);
  }

  //----------------------------------------------------------------------------
  void Buffer::SetMaxAllowedTimeDifference(float maxAllowedTimeDifference)
  {
    this->MaxAllowedTimeDifference = maxAllowedTimeDifference;
  }

  //----------------------------------------------------------------------------
  float Buffer::GetMaxAllowedTimeDifference()
  {
    return this->MaxAllowedTimeDifference;
  }

  //----------------------------------------------------------------------------
  int Buffer::GetPixelType()
  {
    return (int)this->PixelType;
  }

  //----------------------------------------------------------------------------
  int Buffer::GetImageOrientation()
  {
    return this->ImageOrientation;
  }

  //----------------------------------------------------------------------------
  Platform::String^ Buffer::GetDescriptiveName()
  {
    return ref new Platform::String(this->DescriptiveName.c_str());
  }

  //----------------------------------------------------------------------------
  void Buffer::SetDescriptiveName(Platform::String^ descriptiveName)
  {
    this->DescriptiveName = std::wstring(descriptiveName->Data());
  }

}