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
#include "Image.h"

namespace UWPOpenIGTLink
{
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

  public ref class VideoFrame sealed
  {
  public:
    VideoFrame();
    virtual ~VideoFrame();

    property Platform::Array<uint16>^ FrameSize { Platform::Array<uint16>^ get(); }
    property UWPOpenIGTLink::Image^ Image { UWPOpenIGTLink::Image ^ get();  }
    property Windows::Storage::Streams::IBuffer^ ImageData { Windows::Storage::Streams::IBuffer ^ get(); }
    property uint16 NumberOfScalarComponents { uint16 get(); }
    property int ScalarType { int get(); }
    property uint16 Type { uint16 get(); void set(uint16 arg); }
    property uint16 Orientation { uint16 get(); void set(uint16 arg); }

    bool AllocateFrame(const Platform::Array<uint16>^ imageSize, int scalarType, uint16 numberOfScalarComponents);
    int GetScalarPixelType();
    int GetImageOrientation();
    void SetImageOrientation(int imgOrientation);
    uint32 GetNumberOfScalarComponents();
    int GetImageType();
    void SetImageType(int imgType);
    uint32 GetNumberOfBytesPerScalar();
    uint32 GetNumberOfBytesPerPixel();
    Platform::Array<uint16>^ GetFrameSize();
    uint64 GetFrameSizeBytes();
    UWPOpenIGTLink::Image^ GetImage();
    bool DeepCopy(VideoFrame^ DataBufferItem);
    bool DeepCopyFrom(UWPOpenIGTLink::Image^ frame);
    bool ShallowCopyFrom(UWPOpenIGTLink::Image^ frame);
    bool IsImageValid();
    bool FillBlank();

    static Platform::String^ GetStringFromScalarPixelType(int pixelType);
    static uint32 GetNumberOfBytesPerScalar(int pixelType);
    static int GetUsImageOrientationFromString(Platform::String^ imgOrientationStr);
    static Platform::String^ GetStringFromUsImageOrientation(int imgOrientation);
    static int GetUsImageTypeFromString(Platform::String^ imgTypeStr);
    static Platform::String^ GetStringFromUsImageType(int imgType);

  internal:
    bool AllocateFrame(const std::array<uint16, 3>& imageSize, int scalarType, uint16 numberOfScalarComponents);
    void SetImageData(std::shared_ptr<byte> imageData, uint16 numberOfScalarComponents, IGTLScalarType scalarType, std::array<uint16, 3>& imageSize);
    std::shared_ptr<byte> GetImageData();
    bool IsImageValidInternal() const;
    std::array<uint16, 3> GetFrameSize() const;
    void SetImage(UWPOpenIGTLink::Image^ imageData, US_IMAGE_ORIENTATION orientation, US_IMAGE_TYPE imageType);

    UWPOpenIGTLink::Image^  m_image;
    US_IMAGE_TYPE           m_imageType;
    US_IMAGE_ORIENTATION    m_imageOrientation;
  };
}