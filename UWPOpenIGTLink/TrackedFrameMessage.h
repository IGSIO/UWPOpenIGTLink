#pragma once

#include <string>

#include "igtlObject.h"
#include "igtlutil/igtl_util.h"
#include "igtlutil/igtl_header.h"
#include "igtlMessageBase.h"
#include "igtl_types.h"
#include "igtl_win32header.h"

namespace igtl
{
  /*!
  \enum US_IMAGE_TYPE
  \brief Defines constant values for ultrasound image type
  \ingroup PlusLibCommon
  */
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

  // This command prevents 4-byte alignment in the struct (which enables m_FrameSize[3])
#pragma pack(1)     /* For 1-byte boundary in memory */

  ///
  ///  \class TrackedFrameMessage
  ///  \brief IGTL message helper class for tracked frame messages
  ///
  class TrackedFrameMessage : public MessageBase
  {
  public:
    typedef TrackedFrameMessage             Self;
    typedef MessageBase                     Superclass;
    typedef SmartPointer<Self>              Pointer;
    typedef SmartPointer<const Self>        ConstPointer;

    igtlTypeMacro( igtl::TrackedFrameMessage, igtl::MessageBase );
    igtlNewMacro( igtl::TrackedFrameMessage );

  public:
    /*! Override clone so that we use the plus igtl factory */
    virtual igtl::MessageBase::Pointer Clone();

    /*! Get Plus TrackedFrame */
    unsigned char* GetImage();
    const std::map<std::string, std::string>& GetCustomFrameFields();
    US_IMAGE_TYPE GetImageType();
    double GetTimestamp();
    const std::vector<int>& GetFrameSize();
    int GetNumberOfComponents();
    int GetImageSizeInBytes();

  protected:
    class TrackedFrameHeader
    {
    public:
      TrackedFrameHeader();

      size_t GetMessageHeaderSize();

      void ConvertEndianness();

      igtl_uint16 m_ScalarType;           /* scalar type */
      igtl_uint16 m_NumberOfComponents;   /* number of scalar components */
      igtl_uint16 m_ImageType;            /* image type */
      igtl_uint16 m_FrameSize[3];         /* entire image volume size */
      igtl_uint32 m_ImageDataSizeInBytes;
      igtl_uint32 m_XmlDataSizeInBytes;
    };

    virtual int  CalculateContentBufferSize();
    virtual int  PackContent();
    virtual int  UnpackContent();

    TrackedFrameMessage();
    ~TrackedFrameMessage();

    std::map<std::string, std::string> CustomFrameFields;
    unsigned char* Image;
    std::vector<int> FrameSize;
    std::string TrackedFrameXmlData;
    US_IMAGE_TYPE ImageType;
    double Timestamp;
    int ScalarType;
    int NumberOfComponents;
    bool ImageValid;

    int ImageSizeInBytes;

    TrackedFrameHeader MessageHeader;
  };

#pragma pack()

}