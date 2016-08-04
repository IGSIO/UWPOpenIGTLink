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
#include "IGTLConnectorPage.xaml.h"

// Windows includes
#include <collection.h>

// STD includes
#include <string>

using namespace UWPOpenIGTLink;
namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WUX = Windows::UI::Xaml;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;

namespace UWPOpenIGTLinkUI
{
  //----------------------------------------------------------------------------
  IGTLConnectorPage::IGTLConnectorPage()
    : IGTClient( ref new UWPOpenIGTLink::IGTLinkClient() )
    , UITimer( ref new WUX::DispatcherTimer() )
  {
    InitializeComponent();

    // Attempt to connect to default parameters (quietly)
    auto task = IGTClient->ConnectAsync( 2.0 );
    this->connectButton->IsEnabled = false;
    this->connectButton->Content = L"Connecting...";
    concurrency::create_task( task ).then( [this]( bool result )
    {
      processConnectionResult( result );
    }, concurrency::task_continuation_context::use_current() );
  }

  //----------------------------------------------------------------------------
  void IGTLConnectorPage::onUITimerTick( Platform::Object^ sender, Platform::Object^ e )
  {
    if ( IGTClient->Connected )
    {
      Platform::String^ text = ref new Platform::String();
      TrackedFrameReply^ reply = ref new TrackedFrameReply();
      auto found = this->IGTClient->ParseTrackedFrameReply( reply );
      if ( !found )
      {
        return;
      }

      if ( reply->Result != false )
      {
        if ( this->imageDisplay->Source != reply->ImageSource )
        {
          this->imageDisplay->Source = reply->ImageSource;
        }
        for ( auto transform : reply->GetValidTransforms() )
        {
          text = text + transform->Key + L": " + transform->Value + L"\n";
        }
      }

      this->transformTextBlock->Text = text;
    }
  }

  //----------------------------------------------------------------------------
  void IGTLConnectorPage::serverPortTextBox_TextChanged( Platform::Object^ sender, WUXC::TextChangedEventArgs^ e )
  {
    WUXC::TextBox^ textBox = dynamic_cast<WUXC::TextBox^>( sender );
    if ( _wtoi( textBox->Text->Data() ) != this->IGTClient->ServerPort )
    {
      this->IGTClient->ServerPort = _wtoi( textBox->Text->Data() );
    }
  }

  //----------------------------------------------------------------------------
  void IGTLConnectorPage::serverHostnameTextBox_TextChanged( Platform::Object^ sender, WUXC::TextChangedEventArgs^ e )
  {
    WUXC::TextBox^ textBox = dynamic_cast<WUXC::TextBox^>( sender );
    if ( textBox->Text != this->IGTClient->ServerHost )
    {
      this->IGTClient->ServerHost = textBox->Text;
    }
  }

  //----------------------------------------------------------------------------
  void IGTLConnectorPage::connectButton_Click( Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e )
  {
    this->connectButton->IsEnabled = false;

    this->statusIcon->Source = ref new WUXM::Imaging::BitmapImage( ref new WF::Uri( "ms-appx:///Assets/glossy-yellow-button-2400px.png" ) );
    if ( IGTClient->Connected )
    {
      this->connectButton->Content = L"Disconnecting...";
      auto disconAction = IGTClient->DisconnectAsync();
      auto task = concurrency::create_task( disconAction ).then( [this]()
      {
        this->statusBarTextBlock->Text = ref new Platform::String( L"Disconnect successful!" );
        this->statusIcon->Source = ref new WUXM::Imaging::BitmapImage( ref new WF::Uri( "ms-appx:///Assets/glossy-green-button-2400px.png" ) );
        this->connectButton->Content = L"Connect";
        this->connectButton->IsEnabled = true;
      }, concurrency::task_continuation_context::use_current() );
    }
    else
    {
      this->connectButton->Content = L"Connecting...";
      auto task = IGTClient->ConnectAsync( 5.0 );
      concurrency::create_task( task ).then( [this]( bool result )
      {
        processConnectionResult( result );
      }, concurrency::task_continuation_context::use_current() );
    }
  }

  //----------------------------------------------------------------------------
  void IGTLConnectorPage::processConnectionResult( bool result )
  {
    this->connectButton->IsEnabled = true;
    if ( result )
    {
      this->statusBarTextBlock->Text = ref new Platform::String( L"Success! Connected to " ) + IGTClient->ServerHost + ref new Platform::String( L":" ) + ref new Platform::String( std::to_wstring( IGTClient->ServerPort ).c_str() );
      this->statusIcon->Source = ref new WUXM::Imaging::BitmapImage( ref new WF::Uri( "ms-appx:///Assets/glossy-green-button-2400px.png" ) );
      this->connectButton->Content = L"Disconnect";

      this->UITimer->Tick += ref new WF::EventHandler<Object^>( this, &IGTLConnectorPage::onUITimerTick );
      WF::TimeSpan t;
      t.Duration = 33;
      this->UITimer->Interval = t;
      this->UITimer->Start();
    }
    else
    {
      this->UITimer->Stop();

      this->connectButton->Content = L"Connect";
      this->statusBarTextBlock->Text = ref new Platform::String( L"Unable to connect." );
      this->statusIcon->Source = ref new WUXM::Imaging::BitmapImage( ref new WF::Uri( "ms-appx:///Assets/glossy-red-button-2400px.png" ) );
    }
  }
}