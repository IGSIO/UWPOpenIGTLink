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

// Local includes
#include "pch.h"
#include "Image.h"

// OS includes
#include <robuffer.h>

// DirectX includes
#include <dxgiformat.h>

using namespace Windows::Foundation::Numerics;
using namespace Windows::Storage::Streams;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  Image::Image()
  {

  }

  //----------------------------------------------------------------------------
  Image::~Image()
  {

  }

  //----------------------------------------------------------------------------
  bool Image::DeepCopy(Image^ otherImage)
  {
    m_numberOfScalarComponents = otherImage->m_numberOfScalarComponents;
    m_scalarType = otherImage->m_scalarType;
    m_frameSize = otherImage->m_frameSize;

    AllocateScalars(m_frameSize, m_numberOfScalarComponents, m_scalarType);
    memcpy(m_imageData.get(), otherImage->m_imageData.get(), GetImageSizeBytes());

    return true;
  }

  //----------------------------------------------------------------------------
  bool Image::FillBlank()
  {
    if (m_imageData == nullptr)
    {
      return false;
    }

    memset(m_imageData.get(), 0, GetImageSizeBytes());

    return true;
  }

  //----------------------------------------------------------------------------
  uint32 Image::GetNumberOfBytesPerScalar(int pixelType)
  {
    switch ((IGTL_SCALAR_TYPE)pixelType)
    {
      case IGTL_SCALARTYPE_UNKNOWN:
        return 0;
      case IGTL_SCALARTYPE_INT8:
        return sizeof(int8);
      case IGTL_SCALARTYPE_UINT8:
        return sizeof(uint8);
      case IGTL_SCALARTYPE_INT16:
        return sizeof(int16);
      case IGTL_SCALARTYPE_UINT16:
        return sizeof(uint16);
      case IGTL_SCALARTYPE_INT32:
        return sizeof(int32);
      case IGTL_SCALARTYPE_UINT32:
        return sizeof(uint32);
      case IGTL_SCALARTYPE_FLOAT32:
        return sizeof(float);
      case IGTL_SCALARTYPE_FLOAT64:
        return sizeof(double);
      default:
        return 0;
    }
  }

  //----------------------------------------------------------------------------
  void Image::SetImageData(std::shared_ptr<byte> imageData, uint16 numberOfScalarComponents, IGTL_SCALAR_TYPE scalarType, const FrameSize& frameSize)
  {
    m_numberOfScalarComponents = numberOfScalarComponents;
    m_scalarType = scalarType;
    m_frameSize = frameSize;
    m_imageData = imageData;
  }

  //----------------------------------------------------------------------------
  std::shared_ptr<byte> Image::GetImageDataInternal()
  {
    return m_imageData;
  }

  //----------------------------------------------------------------------------
  void Image::AllocateScalars(const FrameSizeABI^ imageSize, uint16 numberOfScalarComponents, int scalarType)
  {
    std::array<uint16, 3> imgSize = { imageSize[0], imageSize[1], imageSize[2] };
    AllocateScalars(imgSize, numberOfScalarComponents, (IGTL_SCALAR_TYPE)scalarType);
  }

  //----------------------------------------------------------------------------
  void Image::AllocateScalars(const FrameSize& imageSize, uint16 numberOfScalarComponents, IGTL_SCALAR_TYPE scalarType)
  {
    if (numberOfScalarComponents == m_numberOfScalarComponents &&
        (IGTL_SCALAR_TYPE)scalarType == m_scalarType &&
        imageSize[0] == m_frameSize[0] &&
        imageSize[1] == m_frameSize[1] &&
        imageSize[2] == m_frameSize[2])
    {
      return;
    }

    m_numberOfScalarComponents = numberOfScalarComponents;
    m_scalarType = (IGTL_SCALAR_TYPE)scalarType;
    m_frameSize[0] = imageSize[0];
    m_frameSize[1] = imageSize[1];
    m_frameSize[2] = imageSize[2];

    m_imageData = std::shared_ptr<byte>(new byte[GetImageSizeBytes()], [](byte * p)
    {
      delete[] p;
    });
  }

  //----------------------------------------------------------------------------
  FrameSize Image::GetFrameSize() const
  {
    return m_frameSize;
  }

  //----------------------------------------------------------------------------
  uint32 Image::GetImageSizeBytes()
  {
    return GetNumberOfBytesPerScalar((int)m_scalarType) * m_numberOfScalarComponents * m_frameSize[0] * m_frameSize[1] * m_frameSize[2];
  }

  //----------------------------------------------------------------------------
  uint32 Image::GetPixelFormat(bool normalized)
  {
    switch (m_numberOfScalarComponents)
    {
      case 1:
        switch (m_scalarType)
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
        switch (m_scalarType)
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
        switch (m_scalarType)
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
        switch (m_scalarType)
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
  int Image::ScalarType::get()
  {
    return m_scalarType;
  }

  //----------------------------------------------------------------------------
  void Image::ScalarType::set(int type)
  {
    m_scalarType = (IGTL_SCALAR_TYPE)type;
  }

  //----------------------------------------------------------------------------
  uint16 Image::NumberOfScalarComponents::get()
  {
    return m_numberOfScalarComponents;
  }

  //----------------------------------------------------------------------------
  void Image::NumberOfScalarComponents::set(uint16 num)
  {
    m_numberOfScalarComponents = num;
  }

  //----------------------------------------------------------------------------
  double Image::Timestamp::get()
  {
    return m_timestamp;
  }

  //----------------------------------------------------------------------------
  void Image::Timestamp::set(double arg)
  {
    m_timestamp = arg;
  }

  //----------------------------------------------------------------------------
  FrameSizeABI^ Image::Dimensions::get()
  {
    auto frameSize = ref new FrameSizeABI(3);
    frameSize[0] = m_frameSize[0];
    frameSize[1] = m_frameSize[1];
    frameSize[2] = m_frameSize[2];
    return frameSize;
  }

  //----------------------------------------------------------------------------
  void Image::Dimensions::set(const FrameSizeABI^ frameSize)
  {
    m_frameSize[0] = frameSize[0];
    m_frameSize[1] = frameSize[1];
    m_frameSize[2] = frameSize[2];
  }

  //----------------------------------------------------------------------------
  IBuffer^ Image::ImageData::get()
  {
    Platform::ArrayReference<byte> arraywrapper(m_imageData.get(), GetImageSizeBytes());
    return Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(arraywrapper);
  }

  //----------------------------------------------------------------------------
  void Image::ImageData::set(IBuffer^ imageData)
  {
    if (imageData == nullptr)
    {
      return;
    }

    unsigned int bufferLength = imageData->Length;

    if (!(bufferLength > 0))
    {
      return;
    }

    HRESULT hr = S_OK;

    Microsoft::WRL::ComPtr<IUnknown> pUnknown = reinterpret_cast<IUnknown*>(imageData);
    Microsoft::WRL::ComPtr<IBufferByteAccess> spByteAccess;
    hr = pUnknown.As(&spByteAccess);
    if (FAILED(hr))
    {
      return;
    }

    byte* pRawData = nullptr;
    hr = spByteAccess->Buffer(&pRawData);
    if (FAILED(hr))
    {
      return;
    }

    m_imageData = std::shared_ptr<byte>(new byte[bufferLength], std::default_delete<byte[]>());
    memcpy(m_imageData.get(), pRawData, bufferLength * sizeof(byte));
  }
}