/*====================================================================
Copyright(c) 2017 Adam Rankin


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
#include "IGTLConnectorPage.xaml.h"

namespace UWPOpenIGTLinkUI
{
  App::App()
  {
    InitializeComponent();
    Application::Current->Suspending += ref new Windows::UI::Xaml::SuspendingEventHandler(this, &App::OnSuspending);
    Application::Current->Resuming += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &App::OnResuming);
  }

  void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
  {
    auto rootFrame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Windows::UI::Xaml::Window::Current->Content);

    if (rootFrame == nullptr)
    {
      rootFrame = ref new Windows::UI::Xaml::Controls::Frame();
      rootFrame->NavigationFailed += ref new Windows::UI::Xaml::Navigation::NavigationFailedEventHandler(this, &App::OnNavigationFailed);

      if (e->PrelaunchActivated == false)
      {
        if (rootFrame->Content == nullptr)
        {
          rootFrame->Navigate(Windows::UI::Xaml::Interop::TypeName(IGTLConnectorPage::typeid), e->Arguments);
        }
        Windows::UI::Xaml::Window::Current->Content = rootFrame;
      }
    }
    else
    {
      if (e->PrelaunchActivated == false)
      {
        if (rootFrame->Content == nullptr)
        {
          rootFrame->Navigate(Windows::UI::Xaml::Interop::TypeName(IGTLConnectorPage::typeid), e->Arguments);
        }
      }
    }

    // Now resize the window to fit the frame
    float DPI = Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi;
    Windows::UI::ViewManagement::ApplicationView::PreferredLaunchWindowingMode = Windows::UI::ViewManagement::ApplicationViewWindowingMode::PreferredLaunchViewSize;
    Windows::Foundation::Size desiredSize = Windows::Foundation::Size(((float)1280 * 96.0f / DPI), ((float)720 * 96.0f / DPI));
    Windows::UI::ViewManagement::ApplicationView::PreferredLaunchViewSize = desiredSize;
    Windows::UI::Xaml::Window::Current->Activate();
    bool result = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->TryResizeView(desiredSize);
  }

  void App::OnSuspending(Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e)
  {
    (void)sender;   // Unused parameter
    (void)e;        // Unused parameter
  }

  void App::OnNavigationFailed(Platform::Object^ sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs^ e)
  {
    throw ref new Platform::FailureException("Failed to load Page " + e->SourcePageType.Name);
  }

  void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
  {
    (void)sender;   // Unused parameter
    (void)args;     // Unused parameter
  }
}