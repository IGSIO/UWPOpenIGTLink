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

// Local includes
#include "IGTLConnectorPage.g.h"

using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml;

namespace UWPOpenIGTLinkUI
{

  /// <summary>
  /// An empty page that can be used on its own or navigated to within a Frame.
  /// </summary>
  public ref class IGTLConnectorPage sealed
  {
  public:
    IGTLConnectorPage();

  protected private:
    void onUITimerTick( Platform::Object^ sender, Platform::Object^ e );
    void serverPortTextBox_TextChanged( Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e );
    void serverHostnameTextBox_TextChanged( Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e );
    void connectButton_Click( Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e );
    void processConnectionResult( bool result );

    /// Convert a c-style byte array to a managed image object
    bool FromNativePointer( IBuffer^ data, uint32 width, uint32 height, uint16 numberOfcomponents, Imaging::WriteableBitmap^ wbm );

  protected private:
    UWPOpenIGTLink::IGTLinkClient^ IGTClient;

    Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ WriteableBmp;

    DispatcherTimer^ UITimer;
  };

}