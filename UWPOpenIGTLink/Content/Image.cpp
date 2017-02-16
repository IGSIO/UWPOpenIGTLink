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

// Local includes
#include "pch.h"
#include "Image.h"

// OS includes
#include <robuffer.h>

// STL includes
#include <memory>

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
    m_imageSize = otherImage->m_imageSize;

    AllocateScalars(m_imageSize, m_numberOfScalarComponents, m_scalarType);
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
    switch ((IGTLScalarType)pixelType)
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
  void Image::SetImageData(std::shared_ptr<byte> imageData, uint16 numberOfScalarComponents, IGTLScalarType scalarType, std::array<uint16, 3>& imageSize)
  {
    m_numberOfScalarComponents = numberOfScalarComponents;
    m_scalarType = scalarType;
    m_imageSize = imageSize;
    m_imageData = imageData;
  }

  //----------------------------------------------------------------------------
  std::shared_ptr<byte> Image::GetImageData()
  {
    return m_imageData;
  }

  //----------------------------------------------------------------------------
  void Image::AllocateScalars(const Platform::Array<uint16>^ imageSize, uint16 numberOfScalarComponents, int scalarType)
  {
    std::array<uint16, 3> imgSize = { imageSize[0], imageSize[1], imageSize[2] };
    AllocateScalars(imgSize, numberOfScalarComponents, (IGTLScalarType)scalarType);
  }

  //----------------------------------------------------------------------------
  void Image::AllocateScalars(const std::array<uint16, 3>& imageSize, uint16 numberOfScalarComponents, IGTLScalarType scalarType)
  {
    if (numberOfScalarComponents == m_numberOfScalarComponents && (IGTLScalarType)scalarType == m_scalarType
        && imageSize[0] == m_imageSize[0] && imageSize[1] == m_imageSize[1] && imageSize[2] == m_imageSize[2])
    {
      return;
    }

    m_numberOfScalarComponents = numberOfScalarComponents;
    m_scalarType = (IGTLScalarType)scalarType;
    m_imageSize[0] = imageSize[0];
    m_imageSize[1] = imageSize[1];
    m_imageSize[2] = imageSize[2];

    m_imageData = std::shared_ptr<byte>(new byte[GetImageSizeBytes()], [](byte * p)
    {
      delete[] p;
    });
  }

  //----------------------------------------------------------------------------
  std::array<uint16, 3> Image::GetFrameSize() const
  {
    return m_imageSize;
  }

  //----------------------------------------------------------------------------
  uint32 Image::GetImageSizeBytes()
  {
    return GetNumberOfBytesPerScalar((int)m_scalarType) * m_numberOfScalarComponents * m_imageSize[0] * m_imageSize[1] * m_imageSize[2];
  }

  //----------------------------------------------------------------------------
  int Image::ScalarType::get()
  {
    return m_scalarType;
  }

  //----------------------------------------------------------------------------
  void Image::ScalarType::set(int type)
  {
    m_scalarType = (IGTLScalarType)type;
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
  Platform::Array<uint16>^ Image::FrameSize::get()
  {
    auto frameSize = ref new Platform::Array<uint16>(3);
    frameSize[0] = m_imageSize[0];
    frameSize[1] = m_imageSize[1];
    frameSize[2] = m_imageSize[2];
    return frameSize;
  }

  //----------------------------------------------------------------------------
  void Image::FrameSize::set(const Platform::Array<uint16>^ frameSize)
  {
    m_imageSize[0] = frameSize[0];
    m_imageSize[1] = frameSize[1];
    m_imageSize[2] = frameSize[2];
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