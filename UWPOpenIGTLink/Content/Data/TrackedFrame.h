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
#include "Transform.h"
#include "TransformName.h"
#include "VideoFrame.h"

// IGT includes
#include <igtl_util.h>

namespace UWPOpenIGTLink
{
  public ref class TrackedFrame sealed
  {
  public:
    property FrameFieldsABI^ Fields {FrameFieldsABI ^ get(); }
    property VideoFrame^ Frame { VideoFrame ^ get(); }
    property FrameSizeABI^ Dimensions { FrameSizeABI ^ get(); }
    property TransformListABI^ Transforms { TransformListABI ^ get(); void set(TransformListABI ^ arg); }
    property double Timestamp { double get(); void set(double arg); }

  public:
    TrackedFrame();
    virtual ~TrackedFrame();

    void SetTransform(Transform^ transform);
    Transform^ GetTransform(TransformName^ name);

    void SetFrameField(Platform::String^ key, Platform::String^ value);
    Platform::String^ GetFrameField(Platform::String^ fieldName);

    bool HasImage();
    uint32 GetPixelFormat(bool normalized);

  internal:
    void SetFrameField(const std::wstring& fieldName, const std::wstring& value);
    bool GetFrameField(const std::wstring& fieldName, std::wstring& value);

    void SetFrameSize(const FrameSize& frameSize);
    FrameSize GetFrameSize()const;

    /// Returns true if the input string ends with "Transform", else false
    static bool IsTransform(const std::wstring& str);

    /// Returns true if the input string ends with "TransformStatus", else false
    static bool IsTransformStatus(const std::wstring& str);

    /*! Convert from field status string to field status enum */
    static FIELD_STATUS ConvertFieldStatusFromString(const std::wstring& statusStr);

    /*! Convert from field status enum to field status string */
    static std::wstring ConvertFieldStatusToString(FIELD_STATUS status);

    /*! Get all frame transforms */
    TransformListInternal GetFrameTransformsInternal();
    void SetFrameTransformsInternal(const TransformListInternal& arg);

  protected private:
    // Tracking/other related fields
    FrameFields               m_frameFields;
    TransformListInternal     m_frameTransforms;

    // Image related fields
    VideoFrame^               m_frame = ref new VideoFrame();
    double                    m_timestamp = 0.0;
    FrameSize                 m_frameSize = { 0, 0, 0 };
  };
}