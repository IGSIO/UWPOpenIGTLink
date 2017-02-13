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
#include "TransformName.h"
#include "VideoFrame.h"

// IGT includes
#include <igtl_util.h>

// STD includes
#include <array>
#include <map>
#include <memory>

// Windows includes
#include <DirectXMath.h>
#include <collection.h>
#include <dxgiformat.h>

/// US_IMAGE_TYPE - Defines constant values for ultrasound image type
enum US_IMAGE_TYPE
{
  US_IMG_TYPE_XX,    /*!< undefined */
  US_IMG_BRIGHTNESS, /*!< B-mode image */
  US_IMG_RF_REAL,    /*!< RF-mode image, signal is stored as a series of real values */
  US_IMG_RF_IQ_LINE, /*!< RF-mode image, signal is stored as a series of I and Q samples in a line (I1, Q1, I2, Q2, ...) */
  US_IMG_RF_I_LINE_Q_LINE, /*!< RF-mode image, signal is stored as a series of I samples in a line, then Q samples in the next line (I1, I2, ..., Q1, Q2, ...) */
  US_IMG_RGB_COLOR, /*!< RGB24 color image */
  US_IMG_TYPE_LAST   /*!< just a placeholder for range checking, this must be the last defined image type */
};

/// US_IMAGE_ORIENTATION - Defines constant values for ultrasound image orientation
///   The ultrasound image axes are defined as follows:
///     x axis: points towards the x coordinate increase direction
///     y axis: points towards the y coordinate increase direction
///     z axis: points towards the z coordinate increase direction
///   The image orientation can be defined by specifying which transducer axis corresponds to the x, y and z image axes, respectively.
enum US_IMAGE_ORIENTATION
{
  US_IMG_ORIENT_XX,  /*!< undefined */
  US_IMG_ORIENT_UF, /*!< image x axis = unmarked transducer axis, image y axis = far transducer axis */
  US_IMG_ORIENT_UFD = US_IMG_ORIENT_UF, /*!< image x axis = unmarked transducer axis, image y axis = far transducer axis, image z axis = descending transducer axis */
  US_IMG_ORIENT_UFA, /*!< image x axis = unmarked transducer axis, image y axis = far transducer axis, image z axis = ascending transducer axis */
  US_IMG_ORIENT_UN, /*!< image x axis = unmarked transducer axis, image y axis = near transducer axis */
  US_IMG_ORIENT_UNA = US_IMG_ORIENT_UN, /*!< image x axis = unmarked transducer axis, image y axis = near transducer axis, image z axis = ascending transducer axis */
  US_IMG_ORIENT_UND, /*!< image x axis = unmarked transducer axis, image y axis = near transducer axis, image z axis = descending transducer axis */
  US_IMG_ORIENT_MF, /*!< image x axis = marked transducer axis, image y axis = far transducer axis */
  US_IMG_ORIENT_MFA = US_IMG_ORIENT_MF, /*!< image x axis = marked transducer axis, image y axis = far transducer axis, image z axis = ascending transducer axis */
  US_IMG_ORIENT_MFD, /*!< image x axis = marked transducer axis, image y axis = far transducer axis, image z axis = descending transducer axis */
  US_IMG_ORIENT_AMF,
  US_IMG_ORIENT_MN, /*!< image x axis = marked transducer axis, image y axis = near transducer axis */
  US_IMG_ORIENT_MND = US_IMG_ORIENT_MN, /*!< image x axis = marked transducer axis, image y axis = near transducer axis, image z axis = descending transducer axis */
  US_IMG_ORIENT_MNA, /*!< image x axis = marked transducer axis, image y axis = near transducer axis, image z axis = ascending transducer axis */
  US_IMG_ORIENT_FU, /*!< image x axis = far transducer axis, image y axis = unmarked transducer axis (usually for RF frames)*/
  US_IMG_ORIENT_NU, /*!< image x axis = near transducer axis, image y axis = unmarked transducer axis (usually for RF frames)*/
  US_IMG_ORIENT_FM, /*!< image x axis = far transducer axis, image y axis = marked transducer axis (usually for RF frames)*/
  US_IMG_ORIENT_NM, /*!< image x axis = near transducer axis, image y axis = marked transducer axis (usually for RF frames)*/
  US_IMG_ORIENT_LAST   /*!< just a placeholder for range checking, this must be the last defined orientation item */
};

namespace UWPOpenIGTLink
{
  enum TrackedFrameFieldStatus
  {
    FIELD_OK,       /// Field is valid
    FIELD_INVALID   /// Field is invalid
  };

  public ref class TransformEntryUWP sealed
  {
  public:
    property TransformName^ Name { TransformName ^ get(); void set(TransformName ^ arg); }
    property Windows::Foundation::Numerics::float4x4 Transform { Windows::Foundation::Numerics::float4x4 get(); void set(Windows::Foundation::Numerics::float4x4 arg); }
    property bool Valid { bool get(); void set(bool arg); }

    void ScaleTranslationComponent(float scalingFactor);
    void Transpose();

  private:
    TransformName^ m_transformName;
    Windows::Foundation::Numerics::float4x4 m_transform;
    bool m_isValid;
  };

  typedef Windows::Foundation::Collections::IVectorView<TransformEntryUWP^> TransformEntryUWPList;
  typedef std::tuple<TransformName^, Windows::Foundation::Numerics::float4x4, bool> TransformEntryInternal;
  typedef std::vector<TransformEntryInternal> TransformEntryInternalList;

  public ref class TrackedFrame sealed
  {
  public:
    typedef std::map<std::wstring, std::wstring> FieldMapType;

    property Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ FrameFields {Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ get(); }
    property VideoFrame^ Frame { VideoFrame ^ get(); }
    property TransformEntryUWPList^ FrameTransforms { TransformEntryUWPList ^ get(); }
    property Windows::Foundation::Numerics::float4x4 EmbeddedImageTransform { Windows::Foundation::Numerics::float4x4 get(); void set(Windows::Foundation::Numerics::float4x4 arg); }
    property double Timestamp { double get(); void set(double arg); }

    int GetTransformStatus(TransformName^ transformName);
    Windows::Foundation::Collections::IMapView<Platform::String^, Platform::String^>^ GetValidTransforms();
    TransformEntryUWPList^ GetTransforms();
    Windows::Foundation::Numerics::float4x4 GetTransform(TransformName^ transformName);
    Windows::Foundation::Collections::IVectorView<TransformName^>^ TrackedFrame::GetTransformNameList();

    void SetFrameField(Platform::String^ key, Platform::String^ value);
    Platform::String^ GetFrameField(Platform::String^ fieldName);

    bool HasImage();

  internal:
    void SetEmbeddedImageTransform(const Windows::Foundation::Numerics::float4x4& matrix);
    const Windows::Foundation::Numerics::float4x4& GetEmbeddedImageTransform();

    void SetFrameField(const std::wstring& fieldName, const std::wstring& value);
    bool GetFrameField(const std::wstring& fieldName, std::wstring& value);

    /// Returns true if the input string ends with "Transform", else false
    static bool IsTransform(const std::wstring& str);

    /// Returns true if the input string ends with "TransformStatus", else false
    static bool IsTransformStatus(const std::wstring& str);

    /*! Convert from field status string to field status enum */
    static TrackedFrameFieldStatus ConvertFieldStatusFromString(const std::wstring& statusStr);

    /*! Convert from field status enum to field status string */
    static std::wstring ConvertFieldStatusToString(TrackedFrameFieldStatus status);

    /*! Get all frame transforms */
    const TransformEntryInternalList& GetFrameTransformsInternal();
    void SetFrameTransformsInternal(const TransformEntryInternalList& arg);
    void SetFrameTransformsInternal(const std::vector<TransformEntryUWP^>& arg);

  protected private:
    // Tracking/other related fields
    FieldMapType                                    m_frameFields;
    TransformEntryInternalList                      m_frameTransforms;

    // Image related fields
    VideoFrame^                                     m_frame = ref new VideoFrame();
    double                                          m_timestamp = 0.0;
    std::array<uint16, 3>                           m_frameSize = { 0, 0, 0 };
    Windows::Foundation::Numerics::float4x4         m_embeddedImageTransform = Windows::Foundation::Numerics::float4x4::identity();
  };
}