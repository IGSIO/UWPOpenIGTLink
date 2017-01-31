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
#include "TrackedFrame.h"

// Windows includes
#include <WindowsNumerics.h>
#include <collection.h>
#include <robuffer.h>

// STL includes
#include <sstream>

using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
  static std::wstring TRANSFORM_POSTFIX = L"Transform";
  static std::wstring TRANSFORM_STATUS_POSTFIX = L"TransformStatus";

  //----------------------------------------------------------------------------
  uint32 TrackedFrame::ImageSizeBytes::get()
  {
    return m_frameSizeBytes;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageSizeBytes::set(uint32 arg)
  {
    m_frameSizeBytes = arg;
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::NumberOfComponents::get()
  {
    return m_numberOfComponents;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::NumberOfComponents::set(uint16 arg)
  {
    m_numberOfComponents = arg;
  }

  //----------------------------------------------------------------------------
  IVectorView<uint16>^ TrackedFrame::FrameSize::get()
  {
    auto vec = ref new Vector<uint16>();
    vec->Append(m_frameSize[0]);
    vec->Append(m_frameSize[1]);
    vec->Append(m_frameSize[2]);
    return vec->GetView();
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::FrameSize::set(IVectorView<uint16>^ arg)
  {
    if (arg->Size > 0)
    {
      m_frameSize[0] = arg->GetAt(0);
    }
    if (arg->Size > 1)
    {
      m_frameSize[1] = arg->GetAt(1);
    }
    if (arg->Size > 2)
    {
      m_frameSize[2] = arg->GetAt(2);
    }
  }

  //----------------------------------------------------------------------------
  IMapView<Platform::String^, Platform::String^>^ TrackedFrame::FrameFields::get()
  {
    auto map = ref new Map<Platform::String^, Platform::String^>();
    for (auto pair : m_frameFields)
    {
      map->Insert(ref new Platform::String(pair.first.c_str()), ref new Platform::String(pair.second.c_str()));
    }
    return map->GetView();
  }

  //----------------------------------------------------------------------------
  bool TrackedFrame::HasImage()
  {
    return m_imageData != nullptr;
  }

  //----------------------------------------------------------------------------
  uint32 TrackedFrame::GetPixelFormat(bool normalized)
  {
    switch (m_numberOfComponents)
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
  void TrackedFrame::SetParameter(Platform::String^ key, Platform::String^ value)
  {
    m_frameFields[std::wstring(key->Data())] = std::wstring(value->Data());
  }

  //----------------------------------------------------------------------------
  IBuffer^ TrackedFrame::ImageData::get()
  {
    Platform::ArrayReference<byte> arraywrapper(m_imageData.get(), m_frameSizeBytes);
    return Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(arraywrapper);
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageData::set(IBuffer^ imageData)
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

  //----------------------------------------------------------------------------
  int32 TrackedFrame::ScalarType::get()
  {
    return (int32)m_scalarType;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ScalarType::set(int32 arg)
  {
    m_scalarType = (IGTLScalarType)arg;
  }

  //----------------------------------------------------------------------------
  SharedBytePtr TrackedFrame::ImageDataSharedPtr::get()
  {
    return (SharedBytePtr)&m_imageData;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageDataSharedPtr::set(SharedBytePtr arg)
  {
    m_imageData = *(std::shared_ptr<byte>*)arg;
  }

  //----------------------------------------------------------------------------
  float4x4 TrackedFrame::EmbeddedImageTransform::get()
  {
    return m_embeddedImageTransform;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::EmbeddedImageTransform::set(float4x4 arg)
  {
    m_embeddedImageTransform = arg;
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::ImageType::get()
  {
    return m_imageType;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageType::set(uint16 arg)
  {
    m_imageType = (US_IMAGE_TYPE)arg;
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::ImageOrientation::get()
  {
    return m_imageOrientation;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageOrientation::set(uint16 arg)
  {
    m_imageOrientation = (US_IMAGE_ORIENTATION)arg;
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::Width::get()
  {
    return m_frameSize[0];
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::Height::get()
  {
    return m_frameSize[1];
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::Depth::get()
  {
    return m_frameSize[2];
  }

  //----------------------------------------------------------------------------
  double TrackedFrame::Timestamp::get()
  {
    return m_timestamp;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::Timestamp::set(double arg)
  {
    m_timestamp = arg;
  }

  //----------------------------------------------------------------------------
  IMapView<Platform::String^, Platform::String^>^ TrackedFrame::GetValidTransforms()
  {
    Map<Platform::String^, Platform::String^>^ outputMap = ref new Map<Platform::String^, Platform::String^>;
    for (auto& entry : m_frameFields)
    {
      if (entry.first.find(L"TransformStatus") != std::wstring::npos)
      {
        if (entry.second.compare(L"OK") != 0)
        {
          continue;
        }
        else
        {
          // This entry is valid, so find the corresponding transform and put it in the list
          //ImageToCroppedImageTransformStatus
          //ImageToCroppedImageTransform
          std::wstring lookupKey(entry.first.substr(0, entry.first.find(L"Status")));
          Platform::String^ refLookupKey = ref new Platform::String(lookupKey.c_str());
          outputMap->Insert(refLookupKey, ref new Platform::String(m_frameFields[lookupKey].c_str()));
        }
      }
    }

    return outputMap->GetView();
  }

  //----------------------------------------------------------------------------
  TransformEntryUWPList^ TrackedFrame::GetTransforms()
  {
    Vector<TransformEntryUWP^>^ vec = ref new Vector<TransformEntryUWP^>();
    for (auto& entryInternal : m_frameTransforms)
    {
      auto entry = ref new TransformEntryUWP();
      entry->Name = std::get<0>(entryInternal);
      entry->Transform = std::get<1>(entryInternal);
      entry->Valid = std::get<2>(entryInternal);
      vec->Append(entry);
    }
    return vec->GetView();
  }

  //----------------------------------------------------------------------------
  float4x4 TrackedFrame::GetTransform(TransformName^ transformName)
  {
    std::wstring transformNameStr;
    try
    {
      transformNameStr = std::wstring(transformName->GetTransformName()->Data());
    }
    catch (Platform::Exception^ e)
    {
      throw e;
    }

    // Append Transform to the end of the transform name
    if (!IsTransform(transformNameStr))
    {
      transformNameStr.append(TRANSFORM_POSTFIX);
    }

    std::wstring value;
    if (!GetCustomFrameField(transformNameStr, value))
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Frame value not found for field: " + transformName->GetTransformName());
    }

    // Find default frame transform
    float vals[16];
    std::wistringstream transformFieldValue(value);
    float item;
    int i = 0;
    while (transformFieldValue >> item && i < 16)
    {
      vals[i++] = item;
    }
    return float4x4(vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], vals[8], vals[9], vals[10], vals[11], vals[12], vals[13], vals[14], vals[15]);
  }

  //----------------------------------------------------------------------------
  int TrackedFrame::GetTransformStatus(TransformName^ transformName)
  {
    TrackedFrameFieldStatus status = FIELD_INVALID;
    std::wstring transformStatusName;
    try
    {
      transformStatusName = std::wstring(transformName->GetTransformName()->Data());
    }
    catch (Platform::Exception^ e)
    {
      throw e;
    }

    // Append TransformStatus to the end of the transform name
    if (IsTransform(transformStatusName))
    {
      transformStatusName.append(L"Status");
    }
    else if (!IsTransformStatus(transformStatusName))
    {
      transformStatusName.append(TRANSFORM_STATUS_POSTFIX);
    }

    std::wstring strStatus;
    if (!GetCustomFrameField(transformStatusName, strStatus))
    {
      throw ref new Platform::Exception(E_FAIL, L"Unable to locate custom frame field: " + transformName->GetTransformName());
    }

    status = TrackedFrame::ConvertFieldStatusFromString(strStatus);

    return status;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::TransposeTransforms()
  {

  }

  //----------------------------------------------------------------------------
  Platform::String^ TrackedFrame::GetCustomFrameField(Platform::String^ fieldName)
  {
    std::wstring field(fieldName->Data());
    FieldMapType::iterator fieldIterator;
    fieldIterator = m_frameFields.find(field);
    if (fieldIterator != m_frameFields.end())
    {
      return ref new Platform::String(fieldIterator->second.c_str());
    }
    return nullptr;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetFrameSize(uint16 frameSize[3])
  {
    m_frameSize[0] = frameSize[0];
    m_frameSize[1] = frameSize[1];
    m_frameSize[2] = frameSize[2];
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetCustomFrameField(const std::wstring& fieldName, const std::wstring& value)
  {
    m_frameFields[fieldName] = value;
  }

  //----------------------------------------------------------------------------
  bool TrackedFrame::GetCustomFrameField(const std::wstring& fieldName, std::wstring& value)
  {
    FieldMapType::iterator fieldIterator;
    fieldIterator = m_frameFields.find(fieldName);
    if (fieldIterator != m_frameFields.end())
    {
      value = fieldIterator->second;
      return true;
    }
    return false;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetEmbeddedImageTransform(const float4x4& matrix)
  {
    m_embeddedImageTransform = matrix;
  }

  //----------------------------------------------------------------------------
  const float4x4& TrackedFrame::GetEmbeddedImageTransform()
  {
    return m_embeddedImageTransform;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetImageData(std::shared_ptr<byte> imageData)
  {
    m_imageData = imageData;
  }

  //----------------------------------------------------------------------------
  std::shared_ptr<byte> TrackedFrame::GetImageData()
  {
    return m_imageData;
  }

  //----------------------------------------------------------------------------
  IVectorView<TransformName^>^ TrackedFrame::GetTransformNameList()
  {
    Vector<TransformName^>^ vec = ref new Vector<TransformName^>();
    for (auto pair : m_frameFields)
    {
      if (IsTransform(pair.first))
      {
        TransformName^ trName = ref new TransformName();
        trName->SetTransformName(ref new Platform::String(pair.first.substr(0, pair.first.length() - TRANSFORM_POSTFIX.length()).c_str()));
        vec->Append(trName);
      }
    }
    return vec->GetView();
  }

  //----------------------------------------------------------------------------
  bool TrackedFrame::IsTransform(const std::wstring& str)
  {
    if (str.length() <= TRANSFORM_POSTFIX.length())
    {
      return false;
    }

    return !str.substr(str.length() - TRANSFORM_POSTFIX.length()).compare(TRANSFORM_POSTFIX);
  }

  //----------------------------------------------------------------------------
  bool TrackedFrame::IsTransformStatus(const std::wstring& str)
  {
    if (str.length() <= TRANSFORM_STATUS_POSTFIX.length())
    {
      return false;
    }

    return !str.substr(str.length() - TRANSFORM_STATUS_POSTFIX.length()).compare(TRANSFORM_STATUS_POSTFIX);
  }

  //----------------------------------------------------------------------------
  TrackedFrameFieldStatus TrackedFrame::ConvertFieldStatusFromString(const std::wstring& statusStr)
  {
    TrackedFrameFieldStatus status = FIELD_INVALID;
    if (statusStr.compare(L"OK") == 0)
    {
      status = FIELD_OK;
    }

    return status;
  }

  //----------------------------------------------------------------------------
  std::wstring TrackedFrame::ConvertFieldStatusToString(TrackedFrameFieldStatus status)
  {
    return status == FIELD_OK ? std::wstring(L"OK") : std::wstring(L"INVALID");
  }

  //----------------------------------------------------------------------------
  const TransformEntryInternalList& TrackedFrame::GetFrameTransformsInternal()
  {
    return m_frameTransforms;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetFrameTransformsInternal(const TransformEntryInternalList& arg)
  {
    m_frameTransforms = arg;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetFrameTransformsInternal(const std::vector<TransformEntryUWP^>& arg)
  {
    m_frameTransforms.clear();
    for (auto& entry : arg)
    {
      m_frameTransforms.push_back(TransformEntryInternal(entry->Name, entry->Transform, entry->Valid));
    }
  }

  //----------------------------------------------------------------------------
  TransformName^ TransformEntryUWP::Name::get()
  {
    return m_transformName;
  }

  //----------------------------------------------------------------------------
  void TransformEntryUWP::Name::set(TransformName^ arg)
  {
    m_transformName = arg;
  }

  //----------------------------------------------------------------------------
  float4x4 TransformEntryUWP::Transform::get()
  {
    return m_transform;
  }

  //----------------------------------------------------------------------------
  void TransformEntryUWP::Transform::set(float4x4 arg)
  {
    m_transform = arg;
  }

  //----------------------------------------------------------------------------
  bool TransformEntryUWP::Valid::get()
  {
    return m_isValid;
  }

  //----------------------------------------------------------------------------
  void TransformEntryUWP::Valid::set(bool arg)
  {
    m_isValid = arg;
  }

  //----------------------------------------------------------------------------
  TransformEntryUWPList^ TrackedFrame::FrameTransforms::get()
  {
    return GetTransforms();
  }

  //----------------------------------------------------------------------------
  void TransformEntryUWP::ScaleTranslationComponent(float scalingFactor)
  {
    m_transform.m14 *= scalingFactor;
    m_transform.m24 *= scalingFactor;
    m_transform.m34 *= scalingFactor;
  }

  //----------------------------------------------------------------------------
  void TransformEntryUWP::Transpose()
  {
    m_transform = transpose(m_transform);
  }

}