//
// SettingsPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "IGTLConnectorPage.g.h"

namespace UWPOpenIGTLinkUI
{

  /// <summary>
  /// An empty page that can be used on its own or navigated to within a Frame.
  /// </summary>
  public ref class IGTLConnectorPage sealed
  {
  public:
    IGTLConnectorPage();

  protected:
    virtual void serverPortTextBox_TextChanged( Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e );
    virtual void serverHostnameTextBox_TextChanged( Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e );
    void applyButton_Click( Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e );

  internal:
    UWPOpenIGTLink::IGTLinkClient^ IGTClient;
  };

}