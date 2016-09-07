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

#include <string>

namespace UWPOpenIGTLink
{
  //--------------------------------------------------------
  void LogMessage( const std::string& msg, const char* fileName, int lineNumber )
  {
    std::ostringstream log;
    log << "|TRACE| ";

    log << msg;

    // Add the message to the log
    if ( fileName != NULL )
    {
      log << "| in " << fileName << "(" << lineNumber << ")"; // add filename and line number
    }

    OutputDebugStringA( log.str().c_str() );
  }

  //--------------------------------------------------------
  void LogMessage( const std::wstring& msg, const char* fileName, int lineNumber )
  {
    std::wostringstream log;
    log << L"|TRACE| ";

    log << msg;

    // Add the message to the log
    if ( fileName != NULL )
    {
      log << L"| in " << fileName << L"(" << lineNumber << L")"; // add filename and line number
    }

    OutputDebugStringW( log.str().c_str() );
  }
}

#if defined(ENABLE_LOG_TRACE)
#define LOG_TRACE(msg) LogMessage(msg, __FILE__, __LINE__);
#else
#define LOG_TRACE(x) do {} while (0)
#endif