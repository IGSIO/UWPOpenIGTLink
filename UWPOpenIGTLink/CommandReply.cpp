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

#include "CommandReply.h"

namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  bool CommandReply::Result::get()
  {
    return m_Result;
  }

  //----------------------------------------------------------------------------
  void CommandReply::Result::set( bool arg )
  {
    m_Result = arg;
  }

  //----------------------------------------------------------------------------
  WFC::IMap<Platform::String^, Platform::String^>^ CommandReply::Parameters::get()
  {
    return m_Parameters;
  }

  //----------------------------------------------------------------------------
  void CommandReply::Parameters::set( WFC::IMap<Platform::String^, Platform::String^>^ arg )
  {
    m_Parameters = arg;
  }

  //----------------------------------------------------------------------------
  int32_t CommandReply::OriginalCommandId::get()
  {
    return m_OriginalCommandId;
  }

  //----------------------------------------------------------------------------
  void CommandReply::OriginalCommandId::set( int32_t arg )
  {
    m_OriginalCommandId = arg;
  }

  //----------------------------------------------------------------------------
  Platform::String^ CommandReply::ErrorString::get()
  {
    return m_ErrorString;
  }

  //----------------------------------------------------------------------------
  void CommandReply::ErrorString::set( Platform::String^ arg )
  {
    m_ErrorString = arg;
  }

  //----------------------------------------------------------------------------
  Platform::String^ CommandReply::CommandContent::get()
  {
    return m_CommandContent;
  }

  //----------------------------------------------------------------------------
  void CommandReply::CommandContent::set( Platform::String^ arg )
  {
    m_CommandContent = arg;
  }

  //----------------------------------------------------------------------------
  Platform::String^ CommandReply::CommandName::get()
  {
    return m_CommandName;
  }

  //----------------------------------------------------------------------------
  void CommandReply::CommandName::set( Platform::String^ arg )
  {
    m_CommandName = arg;
  }
}