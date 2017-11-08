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

#include "pch.h"
#include "IGTCommon.h"

// STL includes
#include <limits>

using namespace Windows::Foundation::Numerics;

namespace
{
  //----------------------------------------------------------------------------
  bool compare_pred(std::string::value_type a, std::string::value_type b)
  {
    return ::tolower(a) == ::tolower(b);
  }

  //----------------------------------------------------------------------------
  bool wcompare_pred(std::wstring::value_type a, std::wstring::value_type b)
  {
    return ::towlower(a) == ::towlower(b);
  }
}

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  bool IsEqualInsensitive(std::string const& a, std::string const& b)
  {
    if (a.length() == b.length())
    {
      return std::equal(begin(b), end(b), begin(a), compare_pred);
    }
    else
    {
      return false;
    }
  }

  //----------------------------------------------------------------------------
  bool IsEqualInsensitive(std::wstring const& a, std::wstring const& b)
  {
    if (a.length() == b.length())
    {
      return std::equal(begin(b), end(b), begin(a), wcompare_pred);
    }
    else
    {
      return false;
    }
  }

  //----------------------------------------------------------------------------
  bool IsEqualInsensitive(std::wstring const& a, Platform::String^ b)
  {
    if (a.length() == b->Length())
    {
      return std::equal(begin(b), end(b), begin(a), wcompare_pred);
    }
    else
    {
      return false;
    }
  }

  //----------------------------------------------------------------------------
  bool IsEqualInsensitive(Platform::String^ a, std::wstring const& b)
  {
    std::wstring awStr(a->Data());
    if (awStr.length() == b.length())
    {
      return std::equal(begin(b), end(b), begin(awStr), wcompare_pred);
    }
    else
    {
      return false;
    }
  }

  //----------------------------------------------------------------------------
  bool IsEqualInsensitive(Platform::String^ a, Platform::String^ b)
  {
    std::wstring awStr(a->Data());
    std::wstring bwStr(b->Data());
    if (awStr.length() == bwStr.length())
    {
      return std::equal(begin(bwStr), end(bwStr), begin(awStr), wcompare_pred);
    }
    else
    {
      return false;
    }
  }

  //----------------------------------------------------------------------------
  void LogMessage(const std::string& msg, const char* fileName, int lineNumber)
  {
    std::ostringstream log;
    log << "|TRACE| ";

    log << msg;

    // Add the message to the log
    if (fileName != NULL)
    {
      log << "| in " << fileName << "(" << lineNumber << ")"; // add filename and line number
    }

    OutputDebugStringA(log.str().c_str());
  }

  //----------------------------------------------------------------------------
  void LogMessage(const std::wstring& msg, const char* fileName, int lineNumber)
  {
    std::wostringstream log;
    log << L"|TRACE| ";

    log << msg;

    // Add the message to the log
    if (fileName != NULL)
    {
      log << L"| in " << fileName << L"(" << lineNumber << L")"; // add filename and line number
    }

    OutputDebugStringW(log.str().c_str());
  }

  //----------------------------------------------------------------------------
  std::wstring PrintMatrix(const Windows::Foundation::Numerics::float4x4& matrix)
  {
    std::wostringstream woss;
    woss << matrix.m11 << " "
         << matrix.m12 << " "
         << matrix.m13 << " "
         << matrix.m14 << "    "
         << matrix.m21 << " "
         << matrix.m22 << " "
         << matrix.m23 << " "
         << matrix.m24 << "    "
         << matrix.m31 << " "
         << matrix.m32 << " "
         << matrix.m33 << " "
         << matrix.m34 << "    "
         << matrix.m41 << " "
         << matrix.m42 << " "
         << matrix.m43 << " "
         << matrix.m44 << std::endl;
    return woss.str();
  }

  //----------------------------------------------------------------------------
  float GetOrientationDifference(const float4x4& aMatrix, const float4x4& bMatrix)
  {
    quaternion aQuat = make_quaternion_from_rotation_matrix(aMatrix);
    quaternion bQuat = make_quaternion_from_rotation_matrix(bMatrix);

    return dot(aQuat, bQuat);
  }
}