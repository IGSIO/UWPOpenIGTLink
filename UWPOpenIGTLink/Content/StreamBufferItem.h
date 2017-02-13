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
#include "VideoFrame.h"

#include <map>
#include <vector>

namespace UWPOpenIGTLink
{
  typedef uint64 BufferItemUidType;

  enum ToolStatus
  {
    TOOL_OK,            /*!< Tool OK */
    TOOL_MISSING,       /*!< Tool or tool port is not available */
    TOOL_OUT_OF_VIEW,   /*!< Cannot obtain transform for tool */
    TOOL_OUT_OF_VOLUME, /*!< Tool is not within the sweet spot of system */
    TOOL_SWITCH1_IS_ON, /*!< Various buttons/switches on tool */
    TOOL_SWITCH2_IS_ON, /*!< Various buttons/switches on tool */
    TOOL_SWITCH3_IS_ON, /*!< Various buttons/switches on tool */
    TOOL_REQ_TIMEOUT,   /*!< Request timeout status */
    TOOL_INVALID        /*!< Invalid tool status */
  };

  public ref class StreamBufferItem sealed
  {
  internal:
    typedef std::map<std::wstring, std::wstring> FieldMapType;

  public:
    StreamBufferItem();
    virtual ~StreamBufferItem();

    /*! Get timestamp for the current buffer item in global time (global = local + offset) */
    double GetTimestamp(double localTimeOffsetSec);

    /*! Get filtered timestamp in global time (global = local + offset) */
    double GetFilteredTimestamp(double localTimeOffsetSec);
    /*! Set filtered timestamp */
    void SetFilteredTimestamp(double filteredTimestamp);

    /*! Get unfiltered timestamp in global time (global = local + offset) */
    double GetUnfilteredTimestamp(double localTimeOffsetSec);
    /*! Set unfiltered timestamp */
    void SetUnfilteredTimestamp(double unfilteredTimestamp);

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

  protected:
    void SetValidTransformData(bool aValid);

  internal:
    /*! Get custom frame field map */
    FieldMapType& GetCustomFrameFieldMap();

  protected private:
    double                                    m_filteredTimeStamp;
    double                                    m_unfilteredTimeStamp;
    uint32                                    m_index;
    BufferItemUidType                         m_uid;
    FieldMapType                              m_customFrameFields;
    bool                                      m_validTransformData;
    VideoFrame^                               m_frame = ref new VideoFrame();
    Windows::Foundation::Numerics::float4x4   m_matrix;
    ToolStatus                                m_status;
  };
}