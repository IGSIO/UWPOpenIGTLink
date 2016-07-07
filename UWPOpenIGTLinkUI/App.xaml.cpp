//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "IGTLConnectorPage.xaml.h"

namespace UWPOpenIGTLinkUI
{
  /// <summary>
  /// Initializes the singleton application object.  This is the first line of authored code
  /// executed, and as such is the logical equivalent of main() or WinMain().
  /// </summary>
  App::App()
  {
    InitializeComponent();
    Application::Current->Suspending += ref new Windows::UI::Xaml::SuspendingEventHandler( this, &App::OnSuspending );
    Application::Current->Resuming += ref new Windows::Foundation::EventHandler<Platform::Object^>( this, &App::OnResuming );
  }

  /// <summary>
  /// Invoked when the application is launched normally by the end user.  Other entry points
  /// will be used such as when the application is launched to open a specific file.
  /// </summary>
  /// <param name="e">Details about the launch request and process.</param>
  void App::OnLaunched( Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e )
  {
#if _DEBUG
    // Show graphics profiling information while debugging.
    if ( IsDebuggerPresent() )
    {
      // Display the current frame rate counters
      DebugSettings->EnableFrameRateCounter = true;
    }
#endif
    auto rootFrame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>( Windows::UI::Xaml::Window::Current->Content );

    // Do not repeat app initialization when the Window already has content,
    // just ensure that the window is active
    if ( rootFrame == nullptr )
    {
      // Create a Frame to act as the navigation context and associate it with
      // a SuspensionManager key
      rootFrame = ref new Windows::UI::Xaml::Controls::Frame();

      rootFrame->NavigationFailed += ref new Windows::UI::Xaml::Navigation::NavigationFailedEventHandler( this, &App::OnNavigationFailed );

      if ( e->PreviousExecutionState == Windows::ApplicationModel::Activation::ApplicationExecutionState::Terminated )
      {
        // TODO: Restore the saved session state only when appropriate, scheduling the
        // final launch steps after the restore is complete

      }

      if ( e->PrelaunchActivated == false )
      {
        if ( rootFrame->Content == nullptr )
        {
          // When the navigation stack isn't restored navigate to the first page,
          // configuring the new page by passing required information as a navigation
          // parameter
          rootFrame->Navigate( Windows::UI::Xaml::Interop::TypeName( IGTLConnectorPage::typeid ), e->Arguments );
        }
        // Place the frame in the current Window
        Windows::UI::Xaml::Window::Current->Content = rootFrame;
      }
    }
    else
    {
      if ( e->PrelaunchActivated == false )
      {
        if ( rootFrame->Content == nullptr )
        {
          // When the navigation stack isn't restored navigate to the first page,
          // configuring the new page by passing required information as a navigation
          // parameter
          rootFrame->Navigate( Windows::UI::Xaml::Interop::TypeName( IGTLConnectorPage::typeid ), e->Arguments );
        }
      }
    }

    // Now resize the window to fit the frame
    float DPI = Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi;

    Windows::UI::ViewManagement::ApplicationView::PreferredLaunchWindowingMode = Windows::UI::ViewManagement::ApplicationViewWindowingMode::PreferredLaunchViewSize;

    Windows::Foundation::Size desiredSize = Windows::Foundation::Size( ( ( float )1280 * 96.0f / DPI ), ( ( float )720 * 96.0f / DPI ) );

    Windows::UI::ViewManagement::ApplicationView::PreferredLaunchViewSize = desiredSize;

    Windows::UI::Xaml::Window::Current->Activate();

    bool result = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->TryResizeView( desiredSize );
  }

  /// <summary>
  /// Invoked when application execution is being suspended.  Application state is saved
  /// without knowing whether the application will be terminated or resumed with the contents
  /// of memory still intact.
  /// </summary>
  /// <param name="sender">The source of the suspend request.</param>
  /// <param name="e">Details about the suspend request.</param>
  void App::OnSuspending( Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e )
  {
    ( void )sender; // Unused parameter
    ( void )e; // Unused parameter

    //TODO: Save application state and stop any background activity
  }

  /// <summary>
  /// Invoked when Navigation to a certain page fails
  /// </summary>
  /// <param name="sender">The Frame which failed navigation</param>
  /// <param name="e">Details about the navigation failure</param>
  void App::OnNavigationFailed( Platform::Object^ sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs^ e )
  {
    throw ref new Platform::FailureException( "Failed to load Page " + e->SourcePageType.Name );
  }

  /// <summary>
  /// Invoked when application execution is being resumed.  Application state is loaded
  /// </summary>
  /// <param name="sender">The source of the resume request.</param>
  /// <param name="e">Details about the resume request.</param>
  void App::OnResuming( Platform::Object^ sender, Platform::Object^ args )
  {

  }

}