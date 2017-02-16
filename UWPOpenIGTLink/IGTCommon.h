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

// STL includes
#include <string>
#include <sstream>

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
  typedef uint32 BufferItemUidType;

  enum ToolStatus
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
  enum IGTLScalarType
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

  /*! Tracker item temporal interpolation type */
  enum DATA_ITEM_TEMPORAL_INTERPOLATION
  {
    EXACT_TIME, /*!< only returns the item if the requested timestamp exactly matches the timestamp of an existing element */
    INTERPOLATED, /*!< returns interpolated transform (requires valid transform at the requested timestamp) */
    CLOSEST_TIME /*!< returns the closest item  */
  };

  static const double UNDEFINED_TIMESTAMP = (std::numeric_limits<double>::max)();

  typedef Platform::Collections::Map<Platform::String^, Platform::String^> StringMap;

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
  T VectorMean(const std::vector<T>& vec, T initialValue)
  {
    T output(initialValue);
    for (auto entry : vec)
    {
      output += entry;
    }
    output /= vec.size();

    return output;
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
#define LOG_TRACE(msg) LogMessage(msg, __FILE__, __LINE__);
#else
#define LOG_TRACE(x) do {} while (0)
#endif