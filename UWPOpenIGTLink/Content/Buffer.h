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

#pragma once

// Local includes
#include "IGTCommon.h"
#include "Image.h"
#include "StreamBufferItem.h"
#include "TimestampedCircularBuffer.h"
#include "TrackedFrame.h"

namespace UWPOpenIGTLink
{
  ref class Buffer;
  public delegate void InterpolatedAngleExceededThresholdHandler(Buffer^ sender, float interpToA, float interpToB, float threshold);

  public ref class Buffer sealed
  {
  public:
    Buffer();
    virtual ~Buffer();

    event InterpolatedAngleExceededThresholdHandler^ InterpolatedAngleExceededThreshold;

    /*!
    Set the size of the buffer, i.e. the maximum number of
    video frames that it will hold.  The default is 30.
    */
    bool SetBufferSize(BufferItemList::size_type n);
    /*! Get the size of the buffer */
    BufferItemList::size_type GetBufferSize();

    /*!
    Add a frame plus a timestamp to the buffer with frame index.
    If the timestamp is  less than or equal to the previous timestamp,
    or if the frame's format doesn't match the buffer's frame format,
    then the frame is not added to the buffer. If a clip rectangle is defined
    then only that portion of the frame is extracted.
    */
    bool AddItem(Image^ frame,
                 int usImageOrientation,
                 int imageType,
                 uint32 frameNumber,
                 const Platform::Array<int>^ clipRectangleOrigin,
                 const Platform::Array<int>^ clipRectangleSize,
                 FrameFieldsABI^ customFields,
                 float unfilteredTimestamp,
                 float filteredTimestamp);
    /*!
    Add a frame plus a timestamp to the buffer with frame index.
    If the timestamp is  less than or equal to the previous timestamp,
    or if the frame's format doesn't match the buffer's frame format,
    then the frame is not added to the buffer. If a clip rectangle is defined
    then only that portion of the frame is extracted.
    */
    bool AddItem(VideoFrame^ frame,
                 uint32 frameNumber,
                 const Platform::Array<int>^ clipRectangleOrigin,
                 const Platform::Array<int>^ clipRectangleSize,
                 FrameFieldsABI^ customFields,
                 float unfilteredTimestamp,
                 float filteredTimestamp);

    /*!
    Add custom fields to the new item
    If the timestamp is less than or equal to the previous timestamp,
    or if the frame's format doesn't match the buffer's frame format,
    then the frame is not added to the buffer.
    */
    bool AddItem(FrameFieldsABI^ fields,
                 uint32 frameNumber,
                 float unfilteredTimestamp,
                 float filteredTimestamp);

    /*!
    Add a matrix plus status to the list, with an exactly known timestamp value (e.g., provided by a high-precision hardware timer).
    If the timestamp is less than or equal to the previous timestamp, then nothing  will be done.
    If filteredTimestamp argument is undefined then the filtered timestamp will be computed from the input unfiltered timestamp.
    */
    bool AddTimeStampedItem(Windows::Foundation::Numerics::float4x4 matrix,
                            int toolStatus,
                            uint32 frameNumber,
                            float unfilteredTimestamp,
                            FrameFieldsABI^ customFields,
                            float filteredTimestamp);

    /*! Get a frame with the specified frame uid from the buffer */
    StreamBufferItem^ GetStreamBufferItem(BufferItemUidType uid);
    /*! Get the most recent frame from the buffer */
    StreamBufferItem^ GetLatestStreamBufferItem();
    /*! Get the oldest frame from buffer */
    StreamBufferItem^ GetOldestStreamBufferItem();
    /*! Get a frame that was acquired at the specified time from buffer */
    StreamBufferItem^ GetStreamBufferItemFromTime(float time, int interpolation);
    bool ModifyBufferItemFrameField(BufferItemUidType uid, Platform::String^ key, Platform::String^ value);

    /*! Get latest timestamp in the buffer */
    int GetLatestTimeStamp(float* outLatestTimestamp);

    /*! Get oldest timestamp in the buffer */
    int GetOldestTimeStamp(float* outOldestTimestamp);

    /*! Get buffer item timestamp */
    int GetTimeStamp(BufferItemUidType uid, float* outTimestamp);

    /*! Returns true if the latest item contains valid video data */
    bool GetLatestItemHasValidVideoData();

    /*! Returns true if the latest item contains valid transform data */
    bool GetLatestItemHasValidTransformData();

    /*! Returns true if the latest item contains valid field data */
    bool GetLatestItemHasValidFieldData();

    /*! Get the index assigned by the data acquisition system (usually a counter) from the buffer by frame UID. */
    int GetIndex(BufferItemUidType uid, BufferItemList::size_type* outIndex);

    /*!
    Given a timestamp, compute the nearest buffer index
    This assumes that the times moronically increase
    */
    int GetBufferIndexFromTime(float time, BufferItemList::size_type* outBufferIndex);

    /*! Get buffer item unique ID */
    BufferItemUidType GetOldestItemUidInBuffer();
    BufferItemUidType GetLatestItemUidInBuffer();
    int GetItemUidFromTime(float time, BufferItemUidType* outUid);

    /*! Set the local time offset in seconds (global = local + offset) */
    void SetLocalTimeOffsetSec(float offsetSec);
    /*! Get the local time offset in seconds (global = local + offset) */
    float GetLocalTimeOffsetSec();

    /*! Get the number of items in the buffer */
    BufferItemList::size_type GetNumberOfItems();

    /*!
    Get the frame rate from the buffer based on the number of frames in the buffer and the elapsed time.
    Ideal frame rate shows the mean of the frame periods in the buffer based on the frame
    number difference (aka the device frame rate).
    If framePeriodStdevSecPtr is not null, then the standard deviation of the frame period is computed as well (in seconds) and
    stored at the specified address.
    */
    float GetFrameRate(bool ideal, float* outFramePeriodStdevSec);

    /*! Set maximum allowed time difference in seconds between the desired and the closest valid timestamp */
    void SetMaxAllowedTimeDifference(float maxAllowedTimeDifference);
    /*! Get maximum allowed time difference in seconds between the desired and the closest valid timestamp */
    float GetMaxAllowedTimeDifference();

    /*! Make this buffer into a copy of another buffer.  You should Lock both of the buffers before doing this. */
    void DeepCopy(Buffer^ buffer);

    /*! Clear buffer (set the buffer pointer to the first element) */
    void Clear();

    /*! Set number of items used for timestamp filtering (with LSQR minimizer) */
    void SetAveragedItemsForFiltering(int averagedItemsForFiltering);
    int GetAveragedItemsForFiltering();

    /*! Set recording start time */
    void SetStartTime(float startTime);
    /*! Get recording start time */
    float GetStartTime();

    /*! Set the frame size in pixel  */
    bool SetFrameSize(uint16 x, uint16 y, uint16 z);
    /*! Set the frame size in pixel  */
    bool SetFrameSize(const Platform::Array<uint16>^ frameSize);
    /*! Get the frame size in pixel  */
    Platform::Array<uint16>^ GetFrameSize();

    /*! Set the pixel type */
    bool SetPixelType(int pixelType);
    /*! Get the pixel type */
    int GetPixelType();

    /*! Set the number of scalar components */
    bool SetNumberOfScalarComponents(uint16 numberOfScalarComponents);
    /*! Get the number of scalar components*/
    uint16 GetNumberOfScalarComponents();

    /*! Set the image type. Does not convert the pixel values. */
    bool SetImageType(int imageType);
    /*! Get the image type (B-mode, RF, ...) */
    int GetImageType();

    /*! Set the image orientation (MF, MN, ...). Does not reorder the pixels. */
    bool SetImageOrientation(int imageOrientation);
    /*! Get the image orientation (MF, MN, ...) */
    int GetImageOrientation();

    /*! Get the number of bytes per scalar component */
    int GetNumberOfBytesPerScalar();

    /*!
    Get the number of bytes per pixel
    It is the number of bytes per scalar multiplied by the number of scalar components.
    */
    int GetNumberOfBytesPerPixel();

    Platform::String^ GetDescriptiveName();
    void SetDescriptiveName(Platform::String^ descriptiveName);

  protected private:
    /*! Update video buffer by setting the frame format for each frame  */
    bool AllocateMemoryForFrames();

    /*!
    Compares frame format with new frame imaging parameters.
    \return true if current buffer frame format matches the method arguments, otherwise false
    */
    bool CheckFrameFormat(const std::array<uint16, 3>& frameSize, int pixelType, int imgType, uint16 numberOfScalarComponents);

    /*! Returns the two buffer items that are closest previous and next buffer items relative to the specified time. itemA is the closest item */
    Windows::Foundation::Collections::IVectorView<StreamBufferItem^>^ GetPrevNextBufferItemFromTime(float time);

    /*!
    Interpolate the matrix for the given timestamp from the two nearest transforms in the buffer.
    The rotation is interpolated with SLERP interpolation, and the position is interpolated with linear interpolation.
    The flags correspond to the closest element.
    */
    StreamBufferItem^ GetInterpolatedStreamBufferItemFromTime(float time);

    /*! Get tracker buffer item from an exact timestamp */
    StreamBufferItem^ GetStreamBufferItemFromExactTime(float time);

    /*! Get tracker buffer item from the closest timestamp */
    StreamBufferItem^ GetStreamBufferItemFromClosestTime(float time);

  protected private:
    std::array<uint16, 3> FrameSize = { 0, 0, 1 };
    IGTL_SCALAR_TYPE PixelType = IGTL_SCALARTYPE_UINT8;
    uint16 NumberOfScalarComponents = 1;
    US_IMAGE_TYPE ImageType = US_IMG_BRIGHTNESS;
    US_IMAGE_ORIENTATION ImageOrientation = US_IMG_ORIENT_MF;
    float MaxAllowedTimeDifference = 0.5;
    std::wstring DescriptiveName;
    Windows::Globalization::Calendar^ Calendar = ref new Windows::Globalization::Calendar();

    std::recursive_mutex BufferMutex;
    TimestampedCircularBuffer^ StreamBuffer = ref new TimestampedCircularBuffer();
  };
}