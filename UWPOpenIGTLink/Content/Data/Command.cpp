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
#include "pch.h"
#include "Command.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  bool Command::Result::get()
  {
    return m_Result;
  }

  //----------------------------------------------------------------------------
  void Command::Result::set( bool arg )
  {
    m_Result = arg;
  }

  //----------------------------------------------------------------------------
  IMap<String^, String^>^ Command::Parameters::get()
  {
    return m_Parameters;
  }

  //----------------------------------------------------------------------------
  void Command::Parameters::set( IMap<String^, String^>^ arg )
  {
    m_Parameters = arg;
  }

  //----------------------------------------------------------------------------
  uint32_t Command::OriginalCommandId::get()
  {
    return m_OriginalCommandId;
  }

  //----------------------------------------------------------------------------
  void Command::OriginalCommandId::set( uint32_t arg )
  {
    m_OriginalCommandId = arg;
  }

  //----------------------------------------------------------------------------
  String^ Command::ErrorString::get()
  {
    return m_ErrorString;
  }

  //----------------------------------------------------------------------------
  void Command::ErrorString::set( String^ arg )
  {
    m_ErrorString = arg;
  }

  //----------------------------------------------------------------------------
  String^ Command::CommandContent::get()
  {
    return m_CommandContent;
  }

  //----------------------------------------------------------------------------
  void Command::CommandContent::set( String^ arg )
  {
    m_CommandContent = arg;
  }

  //----------------------------------------------------------------------------
  String^ Command::CommandName::get()
  {
    return m_CommandName;
  }

  //----------------------------------------------------------------------------
  void Command::CommandName::set( String^ arg )
  {
    m_CommandName = arg;
  }
}