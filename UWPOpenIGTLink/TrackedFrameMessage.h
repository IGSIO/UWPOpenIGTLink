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

// Windows includes
#include <Windows.h>

// STD includes
#include <string>

// IGT includes
#include "igtlObject.h"
#include "igtlutil/igtl_util.h"
#include "igtlutil/igtl_header.h"
#include "igtlMessageBase.h"
#include "igtl_types.h"
#include "igtl_win32header.h"

namespace igtl
{
  /// US_IMAGE_TYPE - Defines constant values for ultrasound image type
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

  /// An enum to wrap the c define values specified in igtl_util.h
  enum ScalarType
  {
    IGTL_SCALARTYPE_UNKNOWN = 0,
    IGTL_SCALARTYPE_INT8 = IGTL_SCALAR_INT8,
    IGTL_SCALARTYPE_UINT8 = IGTL_SCALAR_UINT8,
    IGTL_SCALARTYPE_INT16 = IGTL_SCALAR_INT16,
    IGTL_SCALARTYPE_UINT16 = IGTL_SCALAR_UINT16,
    IGTL_SCALARTYPE_INT32 = IGTL_SCALAR_INT32,
    IGTL_SCALARTYPE_UINT32 = IGTL_SCALAR_UINT32,
    IGTL_SCALARTYPE_FLOAT32 = IGTL_SCALAR_FLOAT32,
    IGTL_SCALARTYPE_FLOAT64 = IGTL_SCALAR_FLOAT64,
    IGTL_SCALARTYPE_COMPLEX = IGTL_SCALAR_COMPLEX
  };

  // This command prevents 4-byte alignment in the struct (which enables m_FrameSize[3])
#pragma pack(1)     /* For 1-byte boundary in memory */

  ///  TrackedFrameMessage - IGTL message helper class for tracked frame messages
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

    /// Accessors to the various parts of the message and message header
    byte* GetImage();
    const std::map<std::string, std::string>& GetCustomFrameFields();
    US_IMAGE_TYPE GetImageType();
    double GetTimestamp();
    igtl_uint16* GetFrameSize();
    igtl_uint16 GetNumberOfComponents();
    igtl_uint32 GetImageSizeInBytes();
    ScalarType GetScalarType();

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

    std::map<std::string, std::string> m_customFrameFields;
    byte* m_image;
    std::string m_trackedFrameXmlData;
    double m_timestamp;
    bool m_imageValid;

    int32 m_imageSizeInBytes;

    TrackedFrameHeader m_messageHeader;
  };

#pragma pack()

}