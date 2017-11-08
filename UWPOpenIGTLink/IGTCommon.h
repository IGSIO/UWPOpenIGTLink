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

#pragma once

// STL includes
#include <deque>
#include <sstream>
#include <string>

// WinRT includes
#include <collection.h>

// IGTL includes
#include <igtl_util.h>

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec)
{
  for (uint32 i = 0; i < vec.size(); ++i)
  {
    os << vec[i];
    if (i != vec.size() - 1)
    {
      os << " ";
    }
  }
  return os;
}

namespace UWPOpenIGTLink
{
  ref class StreamBufferItem;
  typedef std::deque<StreamBufferItem^> BufferItemList;
  typedef BufferItemList::size_type BufferItemUidType;

  enum TOOL_STATUS
  {
    TOOL_OK,            /*!< Tool OK */
    TOOL_MISSING,       /*!< Tool or tool port is not available */
    TOOL_OUT_OF_VIEW,   /*!< Cannot obtain transform for tool */
    TOOL_OUT_OF_VOLUME, /*!< Tool is not within the sweet spot of system */
    TOOL_SWITCH1_IS_ON, /*!< Various buttons/switches on tool */
    TOOL_SWITCH2_IS_ON, /*!< Various buttons/switches on tool */
    TOOL_SWITCH3_IS_ON, /*!< Various buttons/switches on tool */
    TOOL_REQ_TIMEOUT,   /*!< Request timeout status */
    TOOL_INVALID        /*!< Invalid tool status */
  };

  /// An enum to wrap the c define values specified in igtl_util.h
  enum IGTL_SCALAR_TYPE
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

  enum TIMESTAMP_FILTERING_OPTION
  {
    READ_FILTERED_AND_UNFILTERED_TIMESTAMPS = 0,
    READ_UNFILTERED_COMPUTE_FILTERED_TIMESTAMPS,
    READ_FILTERED_IGNORE_UNFILTERED_TIMESTAMPS
  };

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

  /// US_IMAGE_ORIENTATION - Defines constant values for ultrasound image orientation
  ///   The ultrasound image axes are defined as follows:
  ///     x axis: points towards the x coordinate increase direction
  ///     y axis: points towards the y coordinate increase direction
  ///     z axis: points towards the z coordinate increase direction
  ///   The image orientation can be defined by specifying which transducer axis corresponds to the x, y and z image axes, respectively.
  enum US_IMAGE_ORIENTATION
  {
    US_IMG_ORIENT_XX,  /*!< undefined */
    US_IMG_ORIENT_UF, /*!< image x axis = unmarked transducer axis, image y axis = far transducer axis */
    US_IMG_ORIENT_UFD = US_IMG_ORIENT_UF, /*!< image x axis = unmarked transducer axis, image y axis = far transducer axis, image z axis = descending transducer axis */
    US_IMG_ORIENT_UFA, /*!< image x axis = unmarked transducer axis, image y axis = far transducer axis, image z axis = ascending transducer axis */
    US_IMG_ORIENT_UN, /*!< image x axis = unmarked transducer axis, image y axis = near transducer axis */
    US_IMG_ORIENT_UNA = US_IMG_ORIENT_UN, /*!< image x axis = unmarked transducer axis, image y axis = near transducer axis, image z axis = ascending transducer axis */
    US_IMG_ORIENT_UND, /*!< image x axis = unmarked transducer axis, image y axis = near transducer axis, image z axis = descending transducer axis */
    US_IMG_ORIENT_MF, /*!< image x axis = marked transducer axis, image y axis = far transducer axis */
    US_IMG_ORIENT_MFA = US_IMG_ORIENT_MF, /*!< image x axis = marked transducer axis, image y axis = far transducer axis, image z axis = ascending transducer axis */
    US_IMG_ORIENT_MFD, /*!< image x axis = marked transducer axis, image y axis = far transducer axis, image z axis = descending transducer axis */
    US_IMG_ORIENT_AMF,
    US_IMG_ORIENT_MN, /*!< image x axis = marked transducer axis, image y axis = near transducer axis */
    US_IMG_ORIENT_MND = US_IMG_ORIENT_MN, /*!< image x axis = marked transducer axis, image y axis = near transducer axis, image z axis = descending transducer axis */
    US_IMG_ORIENT_MNA, /*!< image x axis = marked transducer axis, image y axis = near transducer axis, image z axis = ascending transducer axis */
    US_IMG_ORIENT_FU, /*!< image x axis = far transducer axis, image y axis = unmarked transducer axis (usually for RF frames)*/
    US_IMG_ORIENT_NU, /*!< image x axis = near transducer axis, image y axis = unmarked transducer axis (usually for RF frames)*/
    US_IMG_ORIENT_FM, /*!< image x axis = far transducer axis, image y axis = marked transducer axis (usually for RF frames)*/
    US_IMG_ORIENT_NM, /*!< image x axis = near transducer axis, image y axis = marked transducer axis (usually for RF frames)*/
    US_IMG_ORIENT_LAST   /*!< just a placeholder for range checking, this must be the last defined orientation item */
  };

  /*! Tracker item temporal interpolation type */
  enum DATA_ITEM_TEMPORAL_INTERPOLATION
  {
    EXACT_TIME, /*!< only returns the item if the requested timestamp exactly matches the timestamp of an existing element */
    INTERPOLATED, /*!< returns interpolated transform (requires valid transform at the requested timestamp) */
    CLOSEST_TIME /*!< returns the closest item  */
  };

  enum FIELD_STATUS
  {
    FIELD_OK,       /// Field is valid
    FIELD_INVALID   /// Field is invalid
  };

  static const double UNDEFINED_TIMESTAMP = (std::numeric_limits<double>::max)();

  ref class Transform;
  typedef Windows::Foundation::Collections::IVector<Transform^> TransformListABI;
  typedef std::vector<Transform^> TransformListInternal;
  typedef Platform::Collections::Map<Platform::String^, Platform::String^> StringMap;
  typedef Windows::Foundation::Collections::IMap<Platform::String^, Platform::String^> FrameFieldsABI;
  typedef std::map<std::wstring, std::wstring> FrameFields;
  typedef Platform::Array<uint16> FrameSizeABI;
  typedef std::array<uint16, 3> FrameSize;

#ifdef _WIN64
  typedef int64 SharedBytePtr;
  typedef int64 MessageBasePointerPtr;
#else
  typedef int32 SharedBytePtr;
  typedef int32 MessageBasePointerPtr;
#endif

#define RETRY_UNTIL_TRUE(command_, numberOfRetryAttempts_, delayBetweenRetryAttemptsMSec_) \
  { \
    bool success = false; \
    int numOfTries = 0; \
    while ( !success && numOfTries < numberOfRetryAttempts_ ) \
    { \
      success = (command_);   \
      if (success)  \
      { \
        /* command successfully completed, continue without waiting */ \
        break; \
      } \
      /* command failed, wait for some time and retry */ \
      numOfTries++;   \
      std::this_thread::sleep_for(std::chrono::milliseconds(delayBetweenRetryAttemptsMSec_)); \
    } \
  }

  //----------------------------------------------------------------------------
  bool IsEqualInsensitive(std::string const& a, std::string const& b);
  bool IsEqualInsensitive(std::wstring const& a, std::wstring const& b);
  bool IsEqualInsensitive(std::wstring const& a, Platform::String^ b);
  bool IsEqualInsensitive(Platform::String^ a, std::wstring const& b);
  bool IsEqualInsensitive(Platform::String^ a, Platform::String^ b);

  //--------------------------------------------------------
  void LogMessage(const std::string& msg, const char* fileName, int lineNumber);

  //--------------------------------------------------------
  void LogMessage(const std::wstring& msg, const char* fileName, int lineNumber);

  //--------------------------------------------------------
  std::wstring PrintMatrix(const Windows::Foundation::Numerics::float4x4& matrix);

  //----------------------------------------------------------------------------
  float GetOrientationDifference(const Windows::Foundation::Numerics::float4x4& aMatrix, const Windows::Foundation::Numerics::float4x4& bMatrix);

  //--------------------------------------------------------
  template<typename T>
  float VectorMean(const std::vector<T>& vec, T initialValue)
  {
    float output = static_cast<float>(initialValue);
    for (auto entry : vec)
    {
      output += entry;
    }
    output /= vec.size();

    return output;
  }

  //------------------------------------------------------------------------
  template <typename t = byte>
  t * GetDataFromIBuffer(Windows::Storage::Streams::IBuffer^ container)
  {
    if (container == nullptr)
    {
      return nullptr;
    }

    unsigned int bufferLength = container->Length;

    if (!(bufferLength > 0))
    {
      return nullptr;
    }

    HRESULT hr = S_OK;

    Microsoft::WRL::ComPtr<IUnknown> pUnknown = reinterpret_cast<IUnknown*>(container);
    Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> spByteAccess;
    hr = pUnknown.As(&spByteAccess);
    if (FAILED(hr))
    {
      return nullptr;
    }

    byte* pRawData = nullptr;
    hr = spByteAccess->Buffer(&pRawData);
    if (FAILED(hr))
    {
      return nullptr;
    }

    return reinterpret_cast<t*>(pRawData);
  }

  //--------------------------------------------------------
  class ItemNotAvailableAnymoreException : public std::exception
  {
  public:
    ItemNotAvailableAnymoreException() {};
    virtual ~ItemNotAvailableAnymoreException() {};
  };

  //--------------------------------------------------------
  class ItemNotAvailableYetException : public std::exception
  {
  public:
    ItemNotAvailableYetException() {};
    virtual ~ItemNotAvailableYetException() {};
  };
}

#if defined(ENABLE_LOG_TRACE)
  #define IGT_LOG_TRACE(msg) LogMessage(msg, __FILE__, __LINE__);
#else
  #define IGT_LOG_TRACE(x) do {} while (0)
#endif