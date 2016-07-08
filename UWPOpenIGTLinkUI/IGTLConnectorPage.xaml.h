//
// IGTLConnectorPage.xaml.h
// Declaration of the IGTLConnectorPage class.
//

#pragma once

#include "IGTLConnectorPage.g.h"


namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WUX = Windows::UI::Xaml;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;

namespace UWPOpenIGTLinkUI
{

  /// <summary>
  /// An empty page that can be used on its own or navigated to within a Frame.
  /// </summary>
  public ref class IGTLConnectorPage sealed
  {
  public:
    IGTLConnectorPage();

  private:
    void onUITimerTick(Platform::Object^ sender, Platform::Object^ e);
    void serverPortTextBox_TextChanged( Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e );
    void serverHostnameTextBox_TextChanged( Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e );
    void connectButton_Click( Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e );
    void processConnectionResult(bool result);

  protected private:
    UWPOpenIGTLink::IGTLinkClient^ IGTClient;

    WUX::DispatcherTimer^ UITimer;
  };

}