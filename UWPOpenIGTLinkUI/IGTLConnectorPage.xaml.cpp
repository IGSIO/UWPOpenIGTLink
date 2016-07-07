//
// SettingsPage.xaml.cpp
// Implementation of the settings page
//

#include "pch.h"
#include "IGTLConnectorPage.xaml.h"

namespace WFC = Windows::Foundation::Collections;
namespace WFM = Windows::Foundation::Metadata;
namespace WUXC = Windows::UI::Xaml::Controls;

namespace UWPOpenIGTLinkUI
{

  IGTLConnectorPage::IGTLConnectorPage()
  {
    InitializeComponent();

    IGTClient = ref new UWPOpenIGTLink::IGTLinkClient();

    // Attempt to connect to default parameters (quietly)
  }

  void IGTLConnectorPage::serverPortTextBox_TextChanged( Platform::Object^ sender, WUXC::TextChangedEventArgs^ e )
  {
    WUXC::TextBox^ textBox = dynamic_cast<WUXC::TextBox^>( sender );
    if ( _wtoi( textBox->Text->Data() ) != this->IGTClient->ServerPort )
    {
      this->applyButton->IsEnabled = true;
    }
  }

  void IGTLConnectorPage::serverHostnameTextBox_TextChanged( Platform::Object^ sender, WUXC::TextChangedEventArgs^ e )
  {
    WUXC::TextBox^ textBox = dynamic_cast<WUXC::TextBox^>( sender );
    if ( textBox->Text != this->IGTClient->ServerHost )
    {
      this->applyButton->IsEnabled = true;
    }
  }

  void IGTLConnectorPage::applyButton_Click( Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e )
  {
    auto task = IGTClient->ConnectAsync( 10.0 );
    concurrency::create_task( task ).then( [this]( Platform::String ^ result )
    {
      this->statusBarTextBlock->Text = result;
    } );
  }
}