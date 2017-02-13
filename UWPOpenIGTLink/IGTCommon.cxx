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

#include "pch.h"
#include "IGTCommon.h"

namespace UWPOpenIGTLink
{

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
}

//----------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const std::vector<double>& vec)
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
