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

// std includes
#include <string>
#include <sstream>

// WinRT includes
#include <collection.h>

std::ostream& operator<<(std::ostream& os, const std::vector<double>& vec);

namespace UWPOpenIGTLink
{
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