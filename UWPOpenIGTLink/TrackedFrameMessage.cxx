#include "TrackedFrameMessage.h"
#include "igtlMessageFactory.h"

namespace igtl
{
  //----------------------------------------------------------------------------
  TrackedFrameMessage::TrackedFrameMessage()
    : MessageBase()
  {
    this->m_SendMessageType = "TRACKEDFRAME";
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
  unsigned char* TrackedFrameMessage::GetImage()
  {
    return this->Image;
  }

  //----------------------------------------------------------------------------
  std::map<std::string, std::string>& TrackedFrameMessage::GetCustomFrameFields()
  {
    return this->CustomFrameFields;
  }

  //----------------------------------------------------------------------------
  igtl::US_IMAGE_TYPE TrackedFrameMessage::GetImageType()
  {
    return this->ImageType;
  }

  //----------------------------------------------------------------------------
  igtl::US_IMAGE_ORIENTATION TrackedFrameMessage::GetImageOrientation()
  {
    return this->ImageOrientation;
  }

  //----------------------------------------------------------------------------
  igtl::Matrix4x4* TrackedFrameMessage::GetIJKToRAS()
  {
    return &( this->IJKtoRAS );
  }

  //----------------------------------------------------------------------------
  igtl::TimeStamp::Pointer TrackedFrameMessage::GetTimestamp()
  {
    return this->Timestamp;
  }

  //----------------------------------------------------------------------------
  int TrackedFrameMessage::CalculateContentBufferSize()
  {
    return this->MessageHeader.GetMessageHeaderSize()
           + this->MessageHeader.m_ImageDataSizeInBytes
           + this->MessageHeader.m_XmlDataSizeInBytes;
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
    this->MessageHeader.m_ScalarType = header->m_ScalarType;
    this->MessageHeader.m_NumberOfComponents = header->m_NumberOfComponents;
    this->MessageHeader.m_ImageType = header->m_ImageType;
    this->MessageHeader.m_FrameSize[0] = header->m_FrameSize[0];
    this->MessageHeader.m_FrameSize[1] = header->m_FrameSize[1];
    this->MessageHeader.m_FrameSize[2] = header->m_FrameSize[2];
    this->MessageHeader.m_ImageDataSizeInBytes = header->m_ImageDataSizeInBytes;
    this->MessageHeader.m_XmlDataSizeInBytes = header->m_XmlDataSizeInBytes;

    // Copy xml data
    char* xmlData = ( char* )( this->m_Content + header->GetMessageHeaderSize() );
    this->TrackedFrameXmlData.assign( xmlData, header->m_XmlDataSizeInBytes );

    // TODO : copy custom frame fields from xml data

    // Copy image data
    void* imageData = ( void* )( this->m_Content + header->GetMessageHeaderSize() + header->m_XmlDataSizeInBytes );
    int frameSize[3] = { header->m_FrameSize[0], header->m_FrameSize[1], header->m_FrameSize[2] };
    this->Image = ( unsigned char* )malloc( frameSize[0] * frameSize[1] * frameSize[2] * igtl_get_scalar_size( header->m_ScalarType ) * header->m_NumberOfComponents );
    this->ImageType = ( US_IMAGE_TYPE )header->m_ImageType;

    memcpy( ( void* )( this->Image ), imageData, header->m_ImageDataSizeInBytes );

    // Set timestamp
    this->Timestamp = igtl::TimeStamp::New();
    this->GetTimeStamp( this->Timestamp );

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