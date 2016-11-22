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

namespace UWPOpenIGTLinkUI
{
  public ref class IGTLConnectorPage sealed
  {
  public:
    IGTLConnectorPage();

  protected private:
    void OnUITimerTick(Platform::Object^ sender, Platform::Object^ e);
    void ServerPortTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
    void ServerHostnameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
    void ConnectButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void ProcessConnectionResult(bool result);

    /// Copy the contents of an incoming data buffer to our stored writeable bitmap
    bool IBufferToWriteableBitmap(Windows::Storage::Streams::IBuffer^ data, uint32 width, uint32 height, uint16 numberOfcomponents);

  protected private:
    UWPOpenIGTLink::IGTLinkClient^                          m_IGTClient = ref new UWPOpenIGTLink::IGTLinkClient();
    Windows::UI::Xaml::Media::Imaging::WriteableBitmap^     m_WriteableBitmap = nullptr;
    Windows::UI::Xaml::DispatcherTimer^                     m_UITimer = ref new Windows::UI::Xaml::DispatcherTimer();
  };

}