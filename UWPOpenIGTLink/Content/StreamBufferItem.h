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
#include "IGTCommon.h"
#include "VideoFrame.h"

// STL includes
#include <map>
#include <vector>

namespace UWPOpenIGTLink
{
  public ref class StreamBufferItem sealed
  {
  public:
    StreamBufferItem();
    virtual ~StreamBufferItem();

    /*! Get timestamp for the current buffer item in global time (global = local + offset) */
    float GetTimestamp(float localTimeOffsetSec);

    /*! Get filtered timestamp in global time (global = local + offset) */
    float GetFilteredTimestamp(float localTimeOffsetSec);
    /*! Set filtered timestamp */
    void SetFilteredTimestamp(float filteredTimestamp);

    /*! Get unfiltered timestamp in global time (global = local + offset) */
    float GetUnfilteredTimestamp(float localTimeOffsetSec);
    /*! Set unfiltered timestamp */
    void SetUnfilteredTimestamp(float unfilteredTimestamp);

    /*!
      Set/get index assigned by the data acquisition system (usually a counter)
      If frames are skipped then the counter should be increased by the number of skipped frames, therefore
      the index difference between subsequent frames be more than 1.
    */
    uint32 GetIndex();
    void SetIndex(uint32 index);

    /*! Set/get unique identifier assigned by the storage buffer */
    BufferItemUidType GetUid();
    void SetUid(BufferItemUidType uid);

    /*! Set custom frame field */
    void SetCustomFrameField(Platform::String^ fieldName, Platform::String^ fieldValue);

    /*! Get custom frame field value */
    Platform::String^ GetCustomFrameField(Platform::String^ fieldName);

    /*! Delete custom frame field */
    bool DeleteCustomFrameField(Platform::String^ fieldName);

    bool DeepCopy(StreamBufferItem^ dataItem);

    VideoFrame^ GetFrame();;

    bool SetMatrix(Windows::Foundation::Numerics::float4x4 matrix);
    Windows::Foundation::Numerics::float4x4 GetMatrix();

    void SetStatus(int status);
    int GetStatus();

    bool HasValidTransformData();
    bool HasValidFieldData();
    bool HasValidVideoData();

    void SetValidTransformData(bool aValid);

  internal:
    /*! Get custom frame field map */
    FrameFields GetCustomFrameFields();
    /*! Set custom frame field */
    void SetCustomFrameFieldInternal(const std::wstring& fieldName, const std::wstring& fieldValue);

  protected private:
    float                                     m_filteredTimeStamp;
    float                                     m_unfilteredTimeStamp;
    uint32                                    m_index;
    BufferItemUidType                         m_uid;
    FrameFields                               m_customFrameFields;
    bool                                      m_validTransformData;
    VideoFrame^                               m_frame = ref new VideoFrame();
    Windows::Foundation::Numerics::float4x4   m_matrix;
    TOOL_STATUS                               m_status;
  };
}