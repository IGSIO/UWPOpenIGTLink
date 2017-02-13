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
#include "TrackedFrameMessage.h"

// IGT includes
#include "igtlMessageFactory.h"

namespace igtl
{
  //----------------------------------------------------------------------------
  TrackedFrameMessage::TrackedFrameMessage()
    : MessageBase()
    , m_imageValid(false)
  {
    m_SendMessageType = "TRACKEDFRAME";
  }

  //----------------------------------------------------------------------------
  TrackedFrameMessage::~TrackedFrameMessage()
  {

  }

  //----------------------------------------------------------------------------
  igtl::MessageBase::Pointer TrackedFrameMessage::Clone()
  {
    igtl::MessageBase::Pointer clone;
    {
      igtl::MessageFactory::Pointer factory = igtl::MessageFactory::New();
      factory->AddMessageType("TRACKEDFRAME", (igtl::MessageFactory::PointerToMessageBaseNew)&igtl::TrackedFrameMessage::New);
      clone = dynamic_cast<igtl::MessageBase*>(factory->CreateSendMessage(this->GetMessageType(), this->GetHeaderVersion()).GetPointer());
    }

    igtl::TrackedFrameMessage::Pointer msg = dynamic_cast<igtl::TrackedFrameMessage*>(clone.GetPointer());

    int bodySize = this->m_MessageSize - IGTL_HEADER_SIZE;
    msg->InitBuffer();
    msg->CopyHeader(this);
    msg->AllocateBuffer(bodySize);
    if (bodySize > 0)
    {
      msg->CopyBody(this);
    }

    msg->m_MetaDataHeaderEntries = this->m_MetaDataHeaderEntries;
    msg->m_MetaDataMap = this->m_MetaDataMap;
    msg->m_IsExtendedHeaderUnpacked = this->m_IsExtendedHeaderUnpacked;

    return clone;
  }

  //----------------------------------------------------------------------------
  std::shared_ptr<byte> TrackedFrameMessage::GetImage()
  {
    return this->m_image;
  }

  //----------------------------------------------------------------------------
  const std::map<std::wstring, std::wstring>& TrackedFrameMessage::GetCustomFrameFields()
  {
    return this->m_customFrameFields;
  }

  //----------------------------------------------------------------------------
  const std::vector<UWPOpenIGTLink::TransformEntryUWP^>& TrackedFrameMessage::GetFrameTransforms()
  {
    return m_frameTransforms;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::SetFrameTransforms(const std::vector<UWPOpenIGTLink::TransformEntryUWP^>& transforms)
  {
    m_frameTransforms = transforms;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::ApplyTransformUnitScaling(float scalingFactor)
  {
    for (auto& transform : m_frameTransforms)
    {
      transform->ScaleTranslationComponent(scalingFactor);
    }
  }

  //----------------------------------------------------------------------------
  US_IMAGE_TYPE TrackedFrameMessage::GetImageType()
  {
    return (US_IMAGE_TYPE)m_messageHeader.m_ImageType;
  }

  //----------------------------------------------------------------------------
  igtl_uint16* TrackedFrameMessage::GetFrameSize()
  {
    return this->m_messageHeader.m_FrameSize;
  }

  //----------------------------------------------------------------------------
  igtl_uint16 TrackedFrameMessage::GetNumberOfComponents()
  {
    return this->m_messageHeader.m_NumberOfComponents;
  }

  //----------------------------------------------------------------------------
  igtl_uint32 TrackedFrameMessage::GetImageSizeInBytes()
  {
    return this->m_messageHeader.m_ImageDataSizeInBytes;
  }

  //----------------------------------------------------------------------------
  US_IMAGE_ORIENTATION TrackedFrameMessage::GetImageOrientation()
  {
    return (US_IMAGE_ORIENTATION)this->m_messageHeader.m_ImageOrientation;
  }

  //----------------------------------------------------------------------------
  UWPOpenIGTLink::IGTLScalarType TrackedFrameMessage::GetScalarType()
  {
    return (UWPOpenIGTLink::IGTLScalarType)this->m_messageHeader.m_ScalarType;
  }

  //----------------------------------------------------------------------------
  double TrackedFrameMessage::GetTimestamp()
  {
    return m_timestamp;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::SetEmbeddedImageTransform(const float4x4& matrix)
  {
    m_messageHeader.m_EmbeddedImageTransform[0][0] = matrix.m11;
    m_messageHeader.m_EmbeddedImageTransform[0][1] = matrix.m12;
    m_messageHeader.m_EmbeddedImageTransform[0][2] = matrix.m13;
    m_messageHeader.m_EmbeddedImageTransform[0][3] = matrix.m14;

    m_messageHeader.m_EmbeddedImageTransform[1][0] = matrix.m11;
    m_messageHeader.m_EmbeddedImageTransform[1][1] = matrix.m12;
    m_messageHeader.m_EmbeddedImageTransform[1][2] = matrix.m13;
    m_messageHeader.m_EmbeddedImageTransform[1][3] = matrix.m14;

    m_messageHeader.m_EmbeddedImageTransform[2][0] = matrix.m11;
    m_messageHeader.m_EmbeddedImageTransform[2][1] = matrix.m12;
    m_messageHeader.m_EmbeddedImageTransform[2][2] = matrix.m13;
    m_messageHeader.m_EmbeddedImageTransform[2][3] = matrix.m14;

    m_messageHeader.m_EmbeddedImageTransform[3][0] = matrix.m11;
    m_messageHeader.m_EmbeddedImageTransform[3][1] = matrix.m12;
    m_messageHeader.m_EmbeddedImageTransform[3][2] = matrix.m13;
    m_messageHeader.m_EmbeddedImageTransform[3][3] = matrix.m14;
  }

  //----------------------------------------------------------------------------
  float4x4 TrackedFrameMessage::GetEmbeddedImageTransform()
  {
    float4x4 matrix;
    matrix.m11 = m_messageHeader.m_EmbeddedImageTransform[0][0];
    matrix.m12 = m_messageHeader.m_EmbeddedImageTransform[0][1];
    matrix.m13 = m_messageHeader.m_EmbeddedImageTransform[0][2];
    matrix.m14 = m_messageHeader.m_EmbeddedImageTransform[0][3];

    matrix.m21 = m_messageHeader.m_EmbeddedImageTransform[1][0];
    matrix.m22 = m_messageHeader.m_EmbeddedImageTransform[1][1];
    matrix.m23 = m_messageHeader.m_EmbeddedImageTransform[1][2];
    matrix.m24 = m_messageHeader.m_EmbeddedImageTransform[1][3];

    matrix.m31 = m_messageHeader.m_EmbeddedImageTransform[2][0];
    matrix.m32 = m_messageHeader.m_EmbeddedImageTransform[2][1];
    matrix.m33 = m_messageHeader.m_EmbeddedImageTransform[2][2];
    matrix.m34 = m_messageHeader.m_EmbeddedImageTransform[2][3];

    matrix.m41 = m_messageHeader.m_EmbeddedImageTransform[3][0];
    matrix.m42 = m_messageHeader.m_EmbeddedImageTransform[3][1];
    matrix.m43 = m_messageHeader.m_EmbeddedImageTransform[3][2];
    matrix.m44 = m_messageHeader.m_EmbeddedImageTransform[3][3];
    return matrix;
  }

  //----------------------------------------------------------------------------
  int TrackedFrameMessage::CalculateContentBufferSize()
  {
    return this->m_messageHeader.GetMessageHeaderSize()
           + this->m_messageHeader.m_ImageDataSizeInBytes
           + this->m_messageHeader.m_XmlDataSizeInBytes;
  }

  //----------------------------------------------------------------------------
  int TrackedFrameMessage::PackContent()
  {
    AllocateBuffer();

    // Copy header
    TrackedFrameHeader* header = (TrackedFrameHeader*)(this->m_Content);
    header->m_ScalarType = this->m_messageHeader.m_ScalarType;
    header->m_NumberOfComponents = this->m_messageHeader.m_NumberOfComponents;
    header->m_ImageType = this->m_messageHeader.m_ImageType;
    header->m_FrameSize[0] = this->m_messageHeader.m_FrameSize[0];
    header->m_FrameSize[1] = this->m_messageHeader.m_FrameSize[1];
    header->m_FrameSize[2] = this->m_messageHeader.m_FrameSize[2];
    header->m_ImageDataSizeInBytes = this->m_messageHeader.m_ImageDataSizeInBytes;
    header->m_XmlDataSizeInBytes = this->m_messageHeader.m_XmlDataSizeInBytes;
    header->m_ImageOrientation = this->m_messageHeader.m_ImageOrientation;
    memcpy(header->m_EmbeddedImageTransform, this->m_messageHeader.m_EmbeddedImageTransform, sizeof(igtl::Matrix4x4));

    // Copy xml data
    char* xmlData = (char*)(this->m_Content + header->GetMessageHeaderSize());
    strncpy(xmlData, this->m_trackedFrameXmlData.c_str(), this->m_trackedFrameXmlData.size());
    header->m_XmlDataSizeInBytes = this->m_messageHeader.m_XmlDataSizeInBytes;

    // Copy image data
    void* imageData = (void*)(this->m_Content + header->GetMessageHeaderSize() + header->m_XmlDataSizeInBytes);
    memcpy(imageData, m_image.get(), this->m_messageHeader.m_ImageDataSizeInBytes);

    // Set timestamp
    igtl::TimeStamp::Pointer timestamp = igtl::TimeStamp::New();
    timestamp->GetTime();
    this->SetTimeStamp(timestamp);

    // Convert header endian
    header->ConvertEndianness();

    return 1;
  }

  //----------------------------------------------------------------------------
  int TrackedFrameMessage::UnpackContent()
  {
    TrackedFrameHeader* header = (TrackedFrameHeader*)(this->m_Content);

    // Convert header endian
    header->ConvertEndianness();

    // Copy header
    this->m_messageHeader.m_ScalarType = header->m_ScalarType;
    this->m_messageHeader.m_NumberOfComponents = header->m_NumberOfComponents;
    this->m_messageHeader.m_ImageType = header->m_ImageType;
    this->m_messageHeader.m_FrameSize[0] = header->m_FrameSize[0];
    this->m_messageHeader.m_FrameSize[1] = header->m_FrameSize[1];
    this->m_messageHeader.m_FrameSize[2] = header->m_FrameSize[2];
    this->m_messageHeader.m_ImageDataSizeInBytes = header->m_ImageDataSizeInBytes;
    this->m_messageHeader.m_XmlDataSizeInBytes = header->m_XmlDataSizeInBytes;
    this->m_messageHeader.m_ImageOrientation = header->m_ImageOrientation;
    memcpy(this->m_messageHeader.m_EmbeddedImageTransform, header->m_EmbeddedImageTransform, sizeof(igtl::Matrix4x4));

    // Copy xml data
    char* xmlData = (char*)(this->m_Content + header->GetMessageHeaderSize());
    this->m_trackedFrameXmlData.assign(xmlData, header->m_XmlDataSizeInBytes);

    Windows::Data::Xml::Dom::XmlDocument document;
    document.LoadXml(ref new Platform::String(std::wstring(m_trackedFrameXmlData.begin(), m_trackedFrameXmlData.end()).c_str()));

    auto rootAttributes = document.GetElementsByTagName(L"TrackedFrame")->Item(0)->Attributes;

    igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
    this->GetTimeStamp(ts);
    this->m_timestamp = ts->GetTimeStamp();

    this->m_imageValid = dynamic_cast<Platform::String^>(rootAttributes->GetNamedItem(L"ImageDataValid")->NodeValue) == L"true";

    if (this->m_imageValid)
    {
      // Make a duplicate of the image that is reference counted, this allows it to persist in memory while it's needed
      this->m_image = std::shared_ptr<byte>(new byte[header->m_ImageDataSizeInBytes], std::default_delete<byte[]>());
      memcpy(m_image.get(), this->m_Content + header->GetMessageHeaderSize() + header->m_XmlDataSizeInBytes, header->m_ImageDataSizeInBytes);
    }

    for (unsigned int i = 0; i < document.GetElementsByTagName(L"TrackedFrame")->Item(0)->ChildNodes->Size; ++i)
    {
      auto childNode = document.GetElementsByTagName(L"TrackedFrame")->Item(0)->ChildNodes->Item(i);

      if (childNode->NodeName == L"CustomFrameField")
      {
        std::wstring nameWide(dynamic_cast<Platform::String^>(childNode->Attributes->GetNamedItem(L"Name")->NodeValue)->Data());
        std::wstring valueWide(dynamic_cast<Platform::String^>(childNode->Attributes->GetNamedItem(L"Value")->NodeValue)->Data());
        this->m_customFrameFields[nameWide] = valueWide;
      }
    }

    // Convert custom frame fields storing transforms, to transform entries
    //    We do this in a second loop so that status' are available as well
    for (auto iter = m_customFrameFields.begin(); iter != m_customFrameFields.end();)
    {
      auto name = iter->first;
      auto value = iter->second;
      if (UWPOpenIGTLink::TrackedFrame::IsTransform(name))
      {
        auto entry = ref new UWPOpenIGTLink::TransformEntryUWP();

        std::wistringstream wiss(value);
        float transform[16];
        for (int i = 0; i < 16; ++i)
        {
          wiss >> transform[i];
        }
        DirectX::XMFLOAT4X4 matdx(transform);
        float4x4 result;
        XMStoreFloat4x4(&result, XMLoadFloat4x4(&matdx));
        entry->Transform = result;

        entry->Name = ref new UWPOpenIGTLink::TransformName(ref new Platform::String(name.c_str()));

        auto statusStr = name;
        statusStr.append(L"Status");
        entry->Valid = m_customFrameFields[statusStr] == L"OK";

        m_frameTransforms.push_back(entry);
        iter = m_customFrameFields.erase(iter);
      }
      else
      {
        ++iter;
      }
    }

    // Remove all status fields
    for (auto iter = m_customFrameFields.begin(); iter != m_customFrameFields.end();)
    {
      if (UWPOpenIGTLink::TrackedFrame::IsTransformStatus(iter->first))
      {
        iter = m_customFrameFields.erase(iter);
      }
      else
      {
        ++iter;
      }
    }

    return 1;
  }

  //----------------------------------------------------------------------------
  TrackedFrameMessage::TrackedFrameHeader::TrackedFrameHeader()
    : m_ScalarType()
    , m_NumberOfComponents(0)
    , m_ImageType(0)
    , m_ImageDataSizeInBytes(0)
    , m_XmlDataSizeInBytes(0)
    , m_ImageOrientation(0)
  {
    m_FrameSize[0] = m_FrameSize[1] = m_FrameSize[2] = 0;

    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        m_EmbeddedImageTransform[i][j] = (i == j) ? 1.f : 0.f;
      }
    }
  }

  //----------------------------------------------------------------------------
  size_t TrackedFrameMessage::TrackedFrameHeader::GetMessageHeaderSize()
  {
    size_t headersize = 0;
    headersize += sizeof(igtl_uint16);        // m_ScalarType
    headersize += sizeof(igtl_uint16);        // m_NumberOfComponents
    headersize += sizeof(igtl_uint16);        // m_ImageType
    headersize += sizeof(igtl_uint16) * 3;    // m_FrameSize[3]
    headersize += sizeof(igtl_uint32);        // m_ImageDataSizeInBytes
    headersize += sizeof(igtl_uint32);        // m_XmlDataSizeInBytes
    headersize += sizeof(igtl_uint16);        // m_ImageOrientation
    headersize += sizeof(igtl::Matrix4x4);    // m_EmbeddedImageTransform[4][4]

    return headersize;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::TrackedFrameHeader::ConvertEndianness()
  {
    if (igtl_is_little_endian())
    {
      m_ScalarType = BYTE_SWAP_INT16(m_ScalarType);
      m_NumberOfComponents = BYTE_SWAP_INT16(m_NumberOfComponents);
      m_ImageType = BYTE_SWAP_INT16(m_ImageType);
      m_FrameSize[0] = BYTE_SWAP_INT16(m_FrameSize[0]);
      m_FrameSize[1] = BYTE_SWAP_INT16(m_FrameSize[1]);
      m_FrameSize[2] = BYTE_SWAP_INT16(m_FrameSize[2]);
      m_ImageDataSizeInBytes = BYTE_SWAP_INT32(m_ImageDataSizeInBytes);
      m_XmlDataSizeInBytes = BYTE_SWAP_INT32(m_XmlDataSizeInBytes);
      m_ImageOrientation = BYTE_SWAP_INT16(m_ImageOrientation);
    }
  }

}