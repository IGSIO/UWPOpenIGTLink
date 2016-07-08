//
// IGTLConnectorPage.xaml.cpp
// Implementation of the IGTLConnectorPage page
//

#include "pch.h"
#include "IGTLConnectorPage.xaml.h"
#include <collection.h>
#include <string>

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
    , UITimer( nullptr )
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
      // TODO : async retrieveing image (data copy)
      // then query member variable for latest image
      auto reply = this->IGTClient->ParseImageReply( 1.0 );
      this->imageDisplay->Source = reply->ImageSource;
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
      auto task = IGTClient->ConnectAsync( 10.0 );
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

      this->UITimer = ref new WUX::DispatcherTimer();
      this->UITimer->Tick += ref new WF::EventHandler<Object^>( this, &IGTLConnectorPage::onUITimerTick );
      WF::TimeSpan t;
      t.Duration = 100;
      this->UITimer->Interval = t;
      this->UITimer->Start();
    }
    else
    {
      this->connectButton->Content = L"Connect";
      this->statusBarTextBlock->Text = ref new Platform::String( L"Unable to connect." );
      this->statusIcon->Source = ref new WUXM::Imaging::BitmapImage( ref new WF::Uri( "ms-appx:///Assets/glossy-red-button-2400px.png" ) );
    }
  }
}
