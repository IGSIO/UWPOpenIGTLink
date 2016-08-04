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

#include <stdint.h>

namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
  [Windows::Foundation::Metadata::WebHostHiddenAttribute]
  public ref class CommandReply sealed
  {
  public:
    property bool Result {bool get(); void set( bool arg ); }
    property WFC::IMap<Platform::String^, Platform::String^>^ Parameters {WFC::IMap<Platform::String^, Platform::String^>^ get(); void set( WFC::IMap<Platform::String^, Platform::String^>^ arg ); }
    property int32_t OriginalCommandId {int32_t get(); void set( int32_t arg ); }
    property Platform::String^ ErrorString {Platform::String ^ get(); void set( Platform::String ^ arg ); }
    property Platform::String^ CommandContent {Platform::String ^ get(); void set( Platform::String ^ arg ); }
    property Platform::String^ CommandName {Platform::String ^ get(); void set( Platform::String ^ arg ); }

  protected private:
    bool m_Result;
    WFC::IMap<Platform::String^, Platform::String^>^ m_Parameters;
    int32_t m_OriginalCommandId;
    Platform::String^ m_ErrorString;
    Platform::String^ m_CommandContent;
    Platform::String^ m_CommandName;
  };
}