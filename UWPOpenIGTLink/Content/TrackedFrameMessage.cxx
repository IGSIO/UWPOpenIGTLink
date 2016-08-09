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
#include "TrackedFrameMessage.h"

// IGT includes
#include "igtlMessageFactory.h"

namespace igtl
{
  //----------------------------------------------------------------------------
  TrackedFrameMessage::TrackedFrameMessage()
    : MessageBase()
    , m_imageValid( false )
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
      factory->AddMessageType( "TRACKEDFRAME", ( igtl::MessageFactory::PointerToMessageBaseNew )&igtl::TrackedFrameMessage::New );
      clone = dynamic_cast<igtl::MessageBase*>( factory->CreateSendMessage( this->GetMessageType(), this->GetHeaderVersion() ).GetPointer() );
    }

    igtl::TrackedFrameMessage::Pointer msg = dynamic_cast<igtl::TrackedFrameMessage*>( clone.GetPointer() );

    int bodySize = this->m_MessageSize - IGTL_HEADER_SIZE;
    msg->InitBuffer();
    msg->CopyHeader( this );
    msg->AllocateBuffer( bodySize );
    if ( bodySize > 0 )
    {
      msg->CopyBody( this );
    }

    msg->m_MetaDataHeaderEntries = this->m_MetaDataHeaderEntries;
    msg->m_MetaDataMap = this->m_MetaDataMap;
    msg->m_IsExtendedHeaderUnpacked = this->m_IsExtendedHeaderUnpacked;

    return clone;
  }

  //----------------------------------------------------------------------------
  byte* TrackedFrameMessage::GetImage()
  {
    return this->m_image;
  }

  //----------------------------------------------------------------------------
  const std::map<std::string, std::string>& TrackedFrameMessage::GetCustomFrameFields()
  {
    return this->m_customFrameFields;
  }

  //----------------------------------------------------------------------------
  US_IMAGE_TYPE TrackedFrameMessage::GetImageType()
  {
    return (US_IMAGE_TYPE)m_messageHeader.m_ImageType;
  }

  //----------------------------------------------------------------------------
  double TrackedFrameMessage::GetTimestamp()
  {
    return this->m_timestamp;
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
  IGTLScalarType TrackedFrameMessage::GetScalarType()
  {
    return (IGTLScalarType)this->m_messageHeader.m_ScalarType;
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
    return 1;
  }

  //----------------------------------------------------------------------------
  int TrackedFrameMessage::UnpackContent()
  {
    TrackedFrameHeader* header = ( TrackedFrameHeader* )( this->m_Content );

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

    // Copy xml data
    char* xmlData = ( char* )( this->m_Content + header->GetMessageHeaderSize() );
    this->m_trackedFrameXmlData.assign( xmlData, header->m_XmlDataSizeInBytes );

    Windows::Data::Xml::Dom::XmlDocument document;
    document.LoadXml( ref new Platform::String( std::wstring( m_trackedFrameXmlData.begin(), m_trackedFrameXmlData.end() ).c_str() ) );

    auto rootAttributes = document.GetElementsByTagName( L"TrackedFrame" )->Item( 0 )->Attributes;

    this->m_timestamp = _wtof( dynamic_cast<Platform::String^>( rootAttributes->GetNamedItem( L"Timestamp" )->NodeValue )->Data() );
    this->m_imageValid = dynamic_cast<Platform::String^>( rootAttributes->GetNamedItem( L"ImageDataValid" )->NodeValue ) == L"true";

    if ( this->m_imageValid )
    {
      this->m_imageSizeInBytes = header->m_ImageDataSizeInBytes;
      this->m_image = (byte*)(this->m_Content + header->GetMessageHeaderSize() + header->m_XmlDataSizeInBytes);
    }

    for( unsigned int i = 0; i < document.GetElementsByTagName( L"TrackedFrame" )->Item( 0 )->ChildNodes->Size; ++i )
    {
      auto childNode = document.GetElementsByTagName( L"TrackedFrame" )->Item( 0 )->ChildNodes->Item( i );

      if ( childNode->NodeName == L"CustomFrameField" )
      {
        std::wstring nameWide( dynamic_cast<Platform::String^>( childNode->Attributes->GetNamedItem( L"Name" )->NodeValue )->Data() );
        std::wstring valueWide( dynamic_cast<Platform::String^>( childNode->Attributes->GetNamedItem( L"Value" )->NodeValue )->Data() );

        this->m_customFrameFields[std::string( nameWide.begin(), nameWide.end() )] = std::string( valueWide.begin(), valueWide.end() );
      }
    }

    return 1;
  }

  //----------------------------------------------------------------------------
  TrackedFrameMessage::TrackedFrameHeader::TrackedFrameHeader() : m_ScalarType()
    , m_NumberOfComponents( 0 )
    , m_ImageType( 0 )
    , m_ImageDataSizeInBytes( 0 )
    , m_XmlDataSizeInBytes( 0 )
  {
    m_FrameSize[0] = m_FrameSize[1] = m_FrameSize[2] = 0;
  }

  //----------------------------------------------------------------------------
  size_t TrackedFrameMessage::TrackedFrameHeader::GetMessageHeaderSize()
  {
    size_t headersize = 0;
    headersize += sizeof( igtl_uint16 );      // m_ScalarType
    headersize += sizeof( igtl_uint16 );      // m_NumberOfComponents
    headersize += sizeof( igtl_uint16 );      // m_ImageType
    headersize += sizeof( igtl_uint16 ) * 3;  // m_FrameSize[3]
    headersize += sizeof( igtl_uint32 );      // m_ImageDataSizeInBytes
    headersize += sizeof( igtl_uint32 );      // m_XmlDataSizeInBytes

    return headersize;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::TrackedFrameHeader::ConvertEndianness()
  {
    if ( igtl_is_little_endian() )
    {
      m_ScalarType = BYTE_SWAP_INT16( m_ScalarType );
      m_NumberOfComponents = BYTE_SWAP_INT16( m_NumberOfComponents );
      m_ImageType = BYTE_SWAP_INT16( m_ImageType );
      m_FrameSize[0] = BYTE_SWAP_INT16( m_FrameSize[0] );
      m_FrameSize[1] = BYTE_SWAP_INT16( m_FrameSize[1] );
      m_FrameSize[2] = BYTE_SWAP_INT16( m_FrameSize[2] );
      m_ImageDataSizeInBytes = BYTE_SWAP_INT32( m_ImageDataSizeInBytes );
      m_XmlDataSizeInBytes = BYTE_SWAP_INT32( m_XmlDataSizeInBytes );
    }
  }

}