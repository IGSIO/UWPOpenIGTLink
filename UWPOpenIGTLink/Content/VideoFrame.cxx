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

#include "pch.h"
#include "VideoFrame.h"

// DirectX includes
#include <dxgiformat.h>

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
default::uint32 VideoFrame::GetPixelFormat(bool normalized)
  {
    switch (NumberOfScalarComponents)
    {
      case 1:
        switch ((IGTLScalarType)ScalarType)
        {
          case IGTL_SCALARTYPE_INT8:
            return normalized ? DXGI_FORMAT_R8_SNORM : DXGI_FORMAT_R8_SINT;
          case IGTL_SCALARTYPE_UINT8:
            return normalized ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8_UINT;
          case IGTL_SCALARTYPE_INT16:
            return normalized ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R16_SINT;
          case IGTL_SCALARTYPE_UINT16:
            return normalized ? DXGI_FORMAT_R16_UNORM : DXGI_FORMAT_R16_UINT;
          case IGTL_SCALARTYPE_INT32:
            return DXGI_FORMAT_R32_SINT;
          case IGTL_SCALARTYPE_UINT32:
            return DXGI_FORMAT_R32_UINT;
          case IGTL_SCALARTYPE_FLOAT32:
            return DXGI_FORMAT_R32_FLOAT;
        }
        break;
      case 2:
        switch ((IGTLScalarType)ScalarType)
        {
          case IGTL_SCALARTYPE_INT8:
            return normalized ? DXGI_FORMAT_R8G8_SNORM : DXGI_FORMAT_R8G8_SINT;
          case IGTL_SCALARTYPE_UINT8:
            return normalized ? DXGI_FORMAT_R8G8_UNORM : DXGI_FORMAT_R8G8_UINT;
          case IGTL_SCALARTYPE_INT16:
            return normalized ? DXGI_FORMAT_R16G16_SNORM : DXGI_FORMAT_R16G16_SINT;
          case IGTL_SCALARTYPE_UINT16:
            return normalized ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R16G16_UINT;
          case IGTL_SCALARTYPE_INT32:
            return DXGI_FORMAT_R32G32_SINT;
          case IGTL_SCALARTYPE_UINT32:
            return DXGI_FORMAT_R32G32_UINT;
          case IGTL_SCALARTYPE_FLOAT32:
            return DXGI_FORMAT_R32G32_FLOAT;
        }
        break;
      case 3:
        switch ((IGTLScalarType)ScalarType)
        {
          case IGTL_SCALARTYPE_INT8:
            return normalized ? DXGI_FORMAT_R8G8B8A8_SNORM : DXGI_FORMAT_R8G8B8A8_SINT;
          case IGTL_SCALARTYPE_UINT8:
            return normalized ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UINT;
          case IGTL_SCALARTYPE_INT16:
            return normalized ? DXGI_FORMAT_R16G16B16A16_SNORM : DXGI_FORMAT_R16G16B16A16_SINT;
          case IGTL_SCALARTYPE_UINT16:
            return normalized ? DXGI_FORMAT_R16G16B16A16_UNORM : DXGI_FORMAT_R16G16B16A16_UINT;
          case IGTL_SCALARTYPE_INT32:
            return DXGI_FORMAT_R32G32B32_SINT;
          case IGTL_SCALARTYPE_UINT32:
            return DXGI_FORMAT_R32G32B32_UINT;
          case IGTL_SCALARTYPE_FLOAT32:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        }
      case 4:
        switch ((IGTLScalarType)ScalarType)
        {
          case IGTL_SCALARTYPE_INT8:
            return normalized ? DXGI_FORMAT_R8G8B8A8_SNORM : DXGI_FORMAT_R8G8B8A8_SINT;
          case IGTL_SCALARTYPE_UINT8:
            return normalized ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_SINT;
          case IGTL_SCALARTYPE_INT16:
            return normalized ? DXGI_FORMAT_R16G16B16A16_SNORM : DXGI_FORMAT_R16G16B16A16_SINT;
          case IGTL_SCALARTYPE_UINT16:
            return normalized ? DXGI_FORMAT_R16G16B16A16_UNORM : DXGI_FORMAT_R16G16B16A16_SINT;
          case IGTL_SCALARTYPE_INT32:
            return DXGI_FORMAT_R32G32B32A32_SINT;
          case IGTL_SCALARTYPE_UINT32:
            return DXGI_FORMAT_R32G32B32A32_UINT;
          case IGTL_SCALARTYPE_FLOAT32:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        break;
    }
    return DXGI_FORMAT_UNKNOWN;
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
    if (!IsImageValid())
    {
      OutputDebugStringA("Unable to fill image to blank, image data is NULL.");
      return false;
    }

    m_image->FillBlank();

    return true;
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::AllocateFrame(const Platform::Array<uint16>^ imageSize, int pixType, uint16 numberOfScalarComponents)
  {
    if (!IsImageValid())
    {
      m_image = ref new UWPOpenIGTLink::Image();
    }

    auto frameSize = ref new Platform::Array<uint16>(3);
    frameSize[0] = imageSize[0];
    frameSize[1] = imageSize[1];
    frameSize[2] = imageSize[2];
    if (frameSize[0] > 0 && frameSize[1] > 0 && frameSize[2] == 0)
    {
      OutputDebugStringA("Single slice images should have a dimension of z=1");
      frameSize[2] = 1;
    }

    if (m_image->FrameSize[0] == frameSize[0] &&
        m_image->FrameSize[1] == frameSize[1] &&
        m_image->FrameSize[2] == frameSize[2] &&
        m_image->ScalarType == pixType &&
        m_image->NumberOfScalarComponents == numberOfScalarComponents)
    {
      // already allocated, no change
      return true;
    }

    m_image->AllocateScalars(frameSize, numberOfScalarComponents, pixType);

    return true;
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::AllocateFrame(const std::array<uint16, 3>& imageSize, int scalarType, uint16 numberOfScalarComponents)
  {
    Platform::Array<uint16>^ frameSize = ref new Platform::Array<uint16>(3);
    frameSize[0] = imageSize[0];
    frameSize[1] = imageSize[1];
    frameSize[2] = imageSize[2];
    return AllocateFrame(frameSize, scalarType, numberOfScalarComponents);
  }

  //----------------------------------------------------------------------------
  uint64 VideoFrame::GetFrameSizeBytes()
  {
    if (!IsImageValid())
    {
      return 0;
    }

    return m_image->GetImageSizeBytes();
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::DeepCopyFrom(UWPOpenIGTLink::Image^ frame)
  {
    if (frame == nullptr)
    {
      OutputDebugStringA("Failed to deep copy from image data - input frame is NULL!");
      return false;
    }

    auto frameSize = frame->FrameSize;

    if (!AllocateFrame(frameSize, frame->ScalarType, frame->NumberOfScalarComponents))
    {
      OutputDebugStringA("Failed to allocate memory for plus video frame!");
      return false;
    }

    // TODO : how to copy image data...

    return true;
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::ShallowCopyFrom(UWPOpenIGTLink::Image^ frame)
  {
    if (frame == nullptr)
    {
      OutputDebugStringA("Failed to shallow copy from image data - input frame is NULL!");
      return false;
    }

    m_image = frame;
    return true;
  }

  //----------------------------------------------------------------------------
  bool VideoFrame::IsImageValid()
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
    if (IsImageValid())
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
  Platform::Array<uint16>^ VideoFrame::GetFrameSize()
  {
    if (!IsImageValid())
    {
      OutputDebugStringA("Cannot get buffer pointer, the buffer hasn't been created yet");
      return nullptr;
    }

    return m_image->FrameSize;
  }

  //----------------------------------------------------------------------------
  int VideoFrame::GetScalarPixelType()
  {
    if (!IsImageValid())
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
  void VideoFrame::SetImageData(std::shared_ptr<byte> imageData, uint16 numberOfScalarComponents, IGTLScalarType scalarType, std::array<uint16, 3>& imageSize)
  {
    if (!IsImageValid())
    {
      m_image = ref new UWPOpenIGTLink::Image();
    }

    m_image->SetImageData(imageData, numberOfScalarComponents, scalarType, imageSize);
  }

  //----------------------------------------------------------------------------
  std::shared_ptr<byte> VideoFrame::GetImageData()
  {
    if (!IsImageValid())
    {
      return nullptr;
    }
    return m_image->GetImageData();
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
  void VideoFrame::SetImage(UWPOpenIGTLink::Image^ imageData)
  {
    m_image = imageData;
  }

  //----------------------------------------------------------------------------
#define PIXEL_TO_STRING(pixType) case pixType: return L"##pixType"
  Platform::String^ VideoFrame::GetStringFromScalarPixelType(int pixelType)
  {
    switch ((IGTLScalarType)pixelType)
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
  Platform::Array<uint16>^ VideoFrame::FrameSize::get()
  {
    if (!IsImageValid())
    {
      return nullptr;
    }
    return m_image->FrameSize;
  }

  //----------------------------------------------------------------------------
  uint16 VideoFrame::NumberOfScalarComponents::get()
  {
    if (!IsImageValid())
    {
      return 0;
    }
    return m_image->NumberOfScalarComponents;
  }

  //----------------------------------------------------------------------------
  int VideoFrame::ScalarType::get()
  {
    if (!IsImageValid())
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

  //----------------------------------------------------------------------------
  IBuffer^ VideoFrame::ImageData::get()
  {
    if (!IsImageValid())
    {
      return nullptr;
    }
    return m_image->ImageData;
  }
}