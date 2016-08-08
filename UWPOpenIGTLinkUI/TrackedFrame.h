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

// STD includes
#include <map>

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

#ifndef US_IMAGE_ENUMS
#define US_IMAGE_ENUMS
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
#endif

namespace Plus
{
  [Windows::Foundation::Metadata::WebHostHiddenAttribute]
  public ref class TrackedFrame sealed
  {
  public:
    property IMap<Platform::String^, Platform::String^>^ FrameFields {IMap<Platform::String^, Platform::String^>^ get(); }
    property int32 ImageSizeBytes { int32 get(); void set( int32 arg ); }
    property IVectorView<uint32>^ FrameSize { IVectorView<uint32>^ get(); void set( IVectorView<uint32>^ arg ); }
    property uint16 NumberOfComponents { uint16 get(); void set( uint16 arg ); }
    property IBuffer^ ImageData { IBuffer ^ get(); void set( IBuffer ^ arg ); }

  public:
    void SetParameter( Platform::String^ key, Platform::String^ value );
    IMapView<Platform::String^, Platform::String^>^ GetValidTransforms();

  protected private:
    // Tracking/other related fields
    std::map<std::wstring, std::wstring> m_frameFields;

    // Image related fields
    uint8* m_imageData;
    uint32 m_frameSize[3];
    uint16 m_numberOfComponents;
    int32 m_frameSizeBytes;
    US_IMAGE_TYPE ImageType;
    US_IMAGE_ORIENTATION ImageOrientation;
  };
}