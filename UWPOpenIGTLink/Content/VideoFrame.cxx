/*====================================================================
Copyright(c) 2018 Adam Rankin


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
#include "VideoFrame.h"

// DirectX includes
#include <dxgiformat.h>

using namespace Windows::Foundation::Numerics;
using namespace Windows::Storage::Streams;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  VideoFrame::VideoFrame()
    : m_image(nullptr)
    , m_imageType(US_IMG_BRIGHTNESS)
    , m_imageOrientation(US_IMG_ORIENT_MF)
  {
  }

  //----------------------------------------------------------------------------
  VideoFrame::~VideoFrame()
  {

  }

  //----------------------------------------------------------------------------
  bool VideoFrame::DeepCopy(VideoFrame^ videoItem)
  {
    if (videoItem == nullptr)
    {
      OutputDebugStringA("Failed to deep copy video buffer item - buffer item NULL!");
      return false;
    }

    m_imageType = videoItem->m_imageType;
    m_imageOrientation = videoItem->m_imageOrientation;

    if (m_image == nullptr)
    {
      m_image = ref new UWPOpenIGTLink::Image();
    }
    return m_image->DeepCopy(videoItem->m_image);
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::FillBlank()
  {
    if (!HasImage())
    {
      OutputDebugStringA("Unable to fill image to blank, image data is NULL.");
      return false;
    }

    m_image->FillBlank();

    return true;
  }

  //----------------------------------------------------------------------------
  uint32 VideoFrame::GetPixelFormat(bool normalized)
  {
    if (HasImage())
    {
      return m_image->GetPixelFormat(normalized);
    }
    else
    {
      return (uint32)DXGI_FORMAT_UNKNOWN;
    }
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::AllocateFrame(const FrameSizeABI^ imageSize, int scalarType, uint16 numberOfScalarComponents)
  {
    return AllocateFrame(FrameSize{ imageSize[0], imageSize[1], imageSize[2] }, scalarType, numberOfScalarComponents);
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::AllocateFrame(const FrameSize& frameSize, int scalarType, uint16 numberOfScalarComponents)
  {
    if (!HasImage())
    {
      m_image = ref new UWPOpenIGTLink::Image();
    }

    auto nonConstFrameSize = frameSize;
    if (nonConstFrameSize[0] > 0 && nonConstFrameSize[1] > 0 && nonConstFrameSize[2] == 0)
    {
      OutputDebugStringA("Single slice images should have a dimension of z=1");
      nonConstFrameSize[2] = 1;
    }

    if (m_image->Dimensions[0] == nonConstFrameSize[0] &&
        m_image->Dimensions[1] == nonConstFrameSize[1] &&
        m_image->Dimensions[2] == nonConstFrameSize[2] &&
        m_image->ScalarType == scalarType &&
        m_image->NumberOfScalarComponents == numberOfScalarComponents)
    {
      // already allocated, no change
      return true;
    }

    m_image->AllocateScalars(nonConstFrameSize, numberOfScalarComponents, (IGTL_SCALAR_TYPE)scalarType);

    return true;
  }

  //----------------------------------------------------------------------------
  uint64 VideoFrame::GetFrameSizeBytes()
  {
    if (!HasImage())
    {
      return 0;
    }

    return m_image->GetImageSizeBytes();
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::DeepCopy(UWPOpenIGTLink::Image^ image, int usImageOrientation, int usImageType)
  {
    if (image == nullptr)
    {
      OutputDebugStringA("Failed to deep copy from image data - input frame is NULL!");
      return false;
    }

    m_imageOrientation = (US_IMAGE_ORIENTATION)usImageOrientation;
    m_imageType = (US_IMAGE_TYPE)usImageType;
    m_image->DeepCopy(image);

    return true;
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::ShallowCopy(UWPOpenIGTLink::Image^ image, int usImageOrientation, int usImageType)
  {
    if (image == nullptr)
    {
      OutputDebugStringA("Failed to shallow copy from image data - input frame is NULL!");
      return false;
    }

    m_imageOrientation = (US_IMAGE_ORIENTATION)usImageOrientation;
    m_imageType = (US_IMAGE_TYPE)usImageType;
    m_image = image;
    return true;
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::HasImage()
  {
    return m_image != nullptr;
  }

  //----------------------------------------------------------------------------
  uint32 VideoFrame::GetNumberOfBytesPerScalar()
  {
    return VideoFrame::GetNumberOfBytesPerScalar(m_image->ScalarType);
  }

  //----------------------------------------------------------------------------
  uint32 VideoFrame::GetNumberOfBytesPerPixel()
  {
    return GetNumberOfBytesPerScalar() * m_image->NumberOfScalarComponents;
  }

  //----------------------------------------------------------------------------
  int VideoFrame::GetImageOrientation()
  {
    return m_imageOrientation;
  }

  //----------------------------------------------------------------------------
  void VideoFrame::SetImageOrientation(int imgOrientation)
  {
    m_imageOrientation = (US_IMAGE_ORIENTATION)imgOrientation;
  }

  //----------------------------------------------------------------------------
  uint32 VideoFrame::GetNumberOfScalarComponents()
  {
    if (HasImage())
    {
      return m_image->NumberOfScalarComponents;
    }

    return 0;
  }

  //----------------------------------------------------------------------------
  int VideoFrame::GetImageType()
  {
    return m_imageType;
  }

  //----------------------------------------------------------------------------
  void VideoFrame::SetImageType(int imgType)
  {
    m_imageType = (US_IMAGE_TYPE)imgType;
  }

  //----------------------------------------------------------------------------
  FrameSizeABI^ VideoFrame::GetDimensions()
  {
    return Dimensions;
  }

  //----------------------------------------------------------------------------
  FrameSize VideoFrame::GetDimensions() const
  {
    if (!IsImageValidInternal())
    {
      return FrameSize({ 0, 0, 0 });
    }
    return m_image->GetFrameSize();
  }

  //----------------------------------------------------------------------------
  int VideoFrame::GetScalarPixelType()
  {
    if (!HasImage())
    {
      OutputDebugStringA("Cannot get buffer pointer, the buffer hasn't been created yet");
      return IGTL_SCALARTYPE_UNKNOWN;
    }

    return m_image->ScalarType;
  }

  //----------------------------------------------------------------------------
  int VideoFrame::GetUsImageOrientationFromString(Platform::String^ imgOrientationStr)
  {
    US_IMAGE_ORIENTATION imgOrientation = US_IMG_ORIENT_XX;
    if (imgOrientationStr == nullptr)
    {
      return imgOrientation;
    }
    else if (imgOrientationStr == L"UF")
    {
      imgOrientation = US_IMG_ORIENT_UF;
    }
    else if (imgOrientationStr == L"UN")
    {
      imgOrientation = US_IMG_ORIENT_UN;
    }
    else if (imgOrientationStr == L"MF")
    {
      imgOrientation = US_IMG_ORIENT_MF;
    }
    else if (imgOrientationStr == L"MN")
    {
      imgOrientation = US_IMG_ORIENT_MN;
    }
    else if (imgOrientationStr == L"UFA")
    {
      imgOrientation = US_IMG_ORIENT_UFA;
    }
    else if (imgOrientationStr == L"UNA")
    {
      imgOrientation = US_IMG_ORIENT_UNA;
    }
    else if (imgOrientationStr == L"MFA")
    {
      imgOrientation = US_IMG_ORIENT_MFA;
    }
    else if (imgOrientationStr == L"MNA")
    {
      imgOrientation = US_IMG_ORIENT_MNA;
    }
    else if (imgOrientationStr == L"AMF")
    {
      imgOrientation = US_IMG_ORIENT_AMF;
    }
    else if (imgOrientationStr == L"UFD")
    {
      imgOrientation = US_IMG_ORIENT_UFD;
    }
    else if (imgOrientationStr == L"UND")
    {
      imgOrientation = US_IMG_ORIENT_UND;
    }
    else if (imgOrientationStr == L"MFD")
    {
      imgOrientation = US_IMG_ORIENT_MFD;
    }
    else if (imgOrientationStr == L"MND")
    {
      imgOrientation = US_IMG_ORIENT_MND;
    }
    else if (imgOrientationStr == L"FU")
    {
      imgOrientation = US_IMG_ORIENT_FU;
    }
    else if (imgOrientationStr == L"NU")
    {
      imgOrientation = US_IMG_ORIENT_NU;
    }
    else if (imgOrientationStr == L"FM")
    {
      imgOrientation = US_IMG_ORIENT_FM;
    }
    else if (imgOrientationStr == L"NM")
    {
      imgOrientation = US_IMG_ORIENT_NM;
    }

    return imgOrientation;
  }

  //----------------------------------------------------------------------------
  Platform::String^ VideoFrame::GetStringFromUsImageOrientation(int imgOrientation)
  {
    switch ((US_IMAGE_ORIENTATION)imgOrientation)
    {
      case US_IMG_ORIENT_FM:
        return L"FM";
      case US_IMG_ORIENT_NM:
        return L"NM";
      case US_IMG_ORIENT_FU:
        return L"FU";
      case US_IMG_ORIENT_NU:
        return L"NU";
      case US_IMG_ORIENT_UFA:
        return L"UFA";
      case US_IMG_ORIENT_UNA:
        return L"UNA";
      case US_IMG_ORIENT_MFA:
        return L"MFA";
      case US_IMG_ORIENT_MNA:
        return L"MNA";
      case US_IMG_ORIENT_AMF:
        return L"AMF";
      case US_IMG_ORIENT_UFD:
        return L"UFD";
      case US_IMG_ORIENT_UND:
        return L"UND";
      case US_IMG_ORIENT_MFD:
        return L"MFD";
      case US_IMG_ORIENT_MND:
        return L"MND";
      default:
        return L"XX";
    }
  }

  //----------------------------------------------------------------------------
  int VideoFrame::GetUsImageTypeFromString(Platform::String^ imgTypeStr)
  {
    std::wstring imgTypeUpper(imgTypeStr->Data());
    std::transform(imgTypeUpper.begin(), imgTypeUpper.end(), imgTypeUpper.begin(), ::towupper);

    US_IMAGE_TYPE imgType = US_IMG_TYPE_XX;
    if (imgTypeUpper.compare(L"BRIGHTNESS") == 0)
    {
      imgType = US_IMG_BRIGHTNESS;
    }
    else if (imgTypeUpper.compare(L"RF_REAL") == 0)
    {
      imgType = US_IMG_RF_REAL;
    }
    else if (imgTypeUpper.compare(L"RF_IQ_LINE") == 0)
    {
      imgType = US_IMG_RF_IQ_LINE;
    }
    else if (imgTypeUpper.compare(L"RF_I_LINE_Q_LINE") == 0)
    {
      imgType = US_IMG_RF_I_LINE_Q_LINE;
    }
    else if (imgTypeUpper.compare(L"RGB_COLOR") == 0)
    {
      imgType = US_IMG_RGB_COLOR;
    }
    return imgType;
  }

  //----------------------------------------------------------------------------
  Platform::String^ VideoFrame::GetStringFromUsImageType(int imgType)
  {
    switch ((US_IMAGE_TYPE)imgType)
    {
      case US_IMG_BRIGHTNESS:
        return L"BRIGHTNESS";
      case US_IMG_RF_REAL:
        return L"RF_REAL";
      case US_IMG_RF_IQ_LINE:
        return L"RF_IQ_LINE";
      case US_IMG_RF_I_LINE_Q_LINE:
        return L"RF_I_LINE_Q_LINE";
      case US_IMG_RGB_COLOR:
        return L"RGB_COLOR";
      default:
        return L"XX";
    }
  }

  //----------------------------------------------------------------------------
  void VideoFrame::SetImageData(std::shared_ptr<byte> imageData, uint16 numberOfScalarComponents, IGTL_SCALAR_TYPE scalarType, const FrameSize& imageSize)
  {
    if (!HasImage())
    {
      m_image = ref new UWPOpenIGTLink::Image();
    }

    m_image->SetImageData(imageData, numberOfScalarComponents, scalarType, imageSize);
  }

  //----------------------------------------------------------------------------
  std::shared_ptr<byte> VideoFrame::GetImageDataInternal()
  {
    if (!HasImage())
    {
      return nullptr;
    }
    return m_image->GetImageDataInternal();
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::IsImageValidInternal() const
  {
    return m_image != nullptr;
  }

  //----------------------------------------------------------------------------
  uint32 VideoFrame::GetNumberOfBytesPerScalar(int pixelType)
  {
    return UWPOpenIGTLink::Image::GetNumberOfBytesPerScalar(pixelType);
  }

  //----------------------------------------------------------------------------
  UWPOpenIGTLink::Image^ VideoFrame::GetImage()
  {
    return m_image;
  }

  //----------------------------------------------------------------------------
#define PIXEL_TO_STRING(pixType) case pixType: return L"##pixType"
  Platform::String^ VideoFrame::GetStringFromScalarPixelType(int pixelType)
  {
    switch ((IGTL_SCALAR_TYPE)pixelType)
    {
        PIXEL_TO_STRING(IGTL_SCALARTYPE_UNKNOWN);
        PIXEL_TO_STRING(IGTL_SCALARTYPE_INT8);
        PIXEL_TO_STRING(IGTL_SCALARTYPE_UINT8);
        PIXEL_TO_STRING(IGTL_SCALARTYPE_INT16);
        PIXEL_TO_STRING(IGTL_SCALARTYPE_UINT16);
        PIXEL_TO_STRING(IGTL_SCALARTYPE_INT32);
        PIXEL_TO_STRING(IGTL_SCALARTYPE_UINT32);
        PIXEL_TO_STRING(IGTL_SCALARTYPE_FLOAT32);
        PIXEL_TO_STRING(IGTL_SCALARTYPE_FLOAT64);
        PIXEL_TO_STRING(IGTL_SCALARTYPE_COMPLEX);
      default:
        return L"IGTL_SCALARTYPE_UNKNOWN";
    }
  }
#undef PIXEL_TO_STRING

  //----------------------------------------------------------------------------
  double VideoFrame::Timestamp::get()
  {
    if (m_image == nullptr)
    {
      return -1.0;
    }
    return m_image->Timestamp;
  }

  //----------------------------------------------------------------------------
  void VideoFrame::Timestamp::set(double arg)
  {
    if (m_image != nullptr)
    {
      m_image->Timestamp = arg;
    }
  }

  //----------------------------------------------------------------------------
  FrameSizeABI^ VideoFrame::Dimensions::get()
  {
    if (!HasImage())
    {
      return ref new FrameSizeABI(3) { 0, 0, 0 };
    }
    return m_image->Dimensions;
  }

  //----------------------------------------------------------------------------
  uint16 VideoFrame::NumberOfScalarComponents::get()
  {
    if (!HasImage())
    {
      return 0;
    }
    return m_image->NumberOfScalarComponents;
  }

  //----------------------------------------------------------------------------
  float4x4 VideoFrame::EmbeddedImageTransform::get()
  {
    return m_embeddedImageTransform;
  }

  //----------------------------------------------------------------------------
  void VideoFrame::EmbeddedImageTransform::set(float4x4 arg)
  {
    m_embeddedImageTransform = arg;
  }

  //----------------------------------------------------------------------------
  TransformName^ VideoFrame::EmbeddedImageTransformName::get()
  {
    return m_embeddedImageTransformName;
  }

  //----------------------------------------------------------------------------
  void VideoFrame::EmbeddedImageTransformName::set(TransformName^ arg)
  {
    m_embeddedImageTransformName = arg;
  }

  //----------------------------------------------------------------------------
  int VideoFrame::ScalarType::get()
  {
    if (!HasImage())
    {
      return (int)IGTL_SCALARTYPE_UNKNOWN;
    }
    return m_image->ScalarType;
  }

  //----------------------------------------------------------------------------
  uint16 VideoFrame::Type::get()
  {
    return m_imageType;
  }

  //----------------------------------------------------------------------------
  void VideoFrame::Type::set(uint16 arg)
  {
    m_imageType = (US_IMAGE_TYPE)arg;
  }

  //----------------------------------------------------------------------------
  uint16 VideoFrame::Orientation::get()
  {
    return m_imageOrientation;
  }

  //----------------------------------------------------------------------------
  void VideoFrame::Orientation::set(uint16 arg)
  {
    m_imageOrientation = (US_IMAGE_ORIENTATION)arg;
  }

  //----------------------------------------------------------------------------
  UWPOpenIGTLink::Image^ VideoFrame::Image::get()
  {
    return m_image;
  }
}