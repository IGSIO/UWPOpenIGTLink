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
#include "TrackedFrame.h"
#include "TrackedFrameMessage.h"

// Windows includes
#include <collection.h>
#include <robuffer.h>

// STD includes
#include <string>

using namespace UWPOpenIGTLink;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::Storage::Streams;

namespace
{
  inline void ThrowIfFailed( HRESULT hr )
  {
    if ( FAILED( hr ) )
    {
      throw Platform::Exception::CreateException( hr );
    }
  }

  byte* GetPointerToPixelData( IBuffer^ buffer )
  {
    // Cast to Object^, then to its underlying IInspectable interface.
    Platform::Object^ obj = buffer;
    Microsoft::WRL::ComPtr<IInspectable> insp( reinterpret_cast<IInspectable*>( obj ) );

    // Query the IBufferByteAccess interface.
    Microsoft::WRL::ComPtr<IBufferByteAccess> bufferByteAccess;
    ThrowIfFailed( insp.As( &bufferByteAccess ) );

    // Retrieve the buffer data.
    byte* pixels = nullptr;
    ThrowIfFailed( bufferByteAccess->Buffer( &pixels ) );
    return pixels;
  }
}

namespace UWPOpenIGTLinkUI
{
  //----------------------------------------------------------------------------
  IGTLConnectorPage::IGTLConnectorPage()
    : IGTClient( ref new UWPOpenIGTLink::IGTLinkClient() )
    , UITimer( ref new DispatcherTimer() )
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
      Plus::TrackedFrame^ frame = ref new Plus::TrackedFrame();
      auto found = this->GetOldestTrackedFrame( frame );
      if ( !found )
      {
        return;
      }

      if ( WriteableBmp == nullptr )
      {
        WriteableBmp = ref new Windows::UI::Xaml::Media::Imaging::WriteableBitmap( frame->FrameSize->GetAt( 0 ), frame->FrameSize->GetAt( 1 ) );
      }
      FromNativePointer( frame->ImageData, frame->FrameSize->GetAt( 0 ), frame->FrameSize->GetAt( 1 ), frame->NumberOfComponents, WriteableBmp);

      if ( this->imageDisplay->Source != WriteableBmp)
      {
        this->imageDisplay->Source = WriteableBmp;
      }
      for ( auto transform : frame->GetValidTransforms() )
      {
        text = text + transform->Key + L": " + transform->Value + L"\n";
      }

      this->transformTextBlock->Text = text;
    }
  }

  //----------------------------------------------------------------------------
  void IGTLConnectorPage::serverPortTextBox_TextChanged( Platform::Object^ sender, TextChangedEventArgs^ e )
  {
    TextBox^ textBox = dynamic_cast<TextBox^>( sender );
    if ( _wtoi( textBox->Text->Data() ) != this->IGTClient->ServerPort )
    {
      this->IGTClient->ServerPort = _wtoi( textBox->Text->Data() );
    }
  }

  //----------------------------------------------------------------------------
  void IGTLConnectorPage::serverHostnameTextBox_TextChanged( Platform::Object^ sender, TextChangedEventArgs^ e )
  {
    TextBox^ textBox = dynamic_cast<TextBox^>( sender );
    if ( textBox->Text != this->IGTClient->ServerHost )
    {
      this->IGTClient->ServerHost = textBox->Text;
    }
  }

  //----------------------------------------------------------------------------
  void IGTLConnectorPage::connectButton_Click( Platform::Object^ sender, RoutedEventArgs^ e )
  {
    this->connectButton->IsEnabled = false;

    this->statusIcon->Source = ref new Imaging::BitmapImage( ref new Uri( "ms-appx:///Assets/glossy-yellow-button-2400px.png" ) );
    if ( IGTClient->Connected )
    {
      this->connectButton->Content = L"Disconnecting...";
      IGTClient->Disconnect();

      this->statusBarTextBlock->Text = ref new Platform::String( L"Disconnect successful!" );
      this->statusIcon->Source = ref new Imaging::BitmapImage( ref new Uri( "ms-appx:///Assets/glossy-green-button-2400px.png" ) );
      this->connectButton->Content = L"Connect";
      this->connectButton->IsEnabled = true;
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
      this->statusIcon->Source = ref new Imaging::BitmapImage( ref new Uri( "ms-appx:///Assets/glossy-green-button-2400px.png" ) );
      this->connectButton->Content = L"Disconnect";

      this->UITimer->Tick += ref new EventHandler<Object^>( this, &IGTLConnectorPage::onUITimerTick );
      TimeSpan t;
      t.Duration = 33;
      this->UITimer->Interval = t;
      this->UITimer->Start();
    }
    else
    {
      this->UITimer->Stop();

      this->connectButton->Content = L"Connect";
      this->statusBarTextBlock->Text = ref new Platform::String( L"Unable to connect." );
      this->statusIcon->Source = ref new Imaging::BitmapImage( ref new Uri( "ms-appx:///Assets/glossy-red-button-2400px.png" ) );
    }
  }

  //----------------------------------------------------------------------------
  bool IGTLConnectorPage::GetOldestTrackedFrame( Plus::TrackedFrame^ frame )
  {
    /*
    igtl::MessageBase::Pointer igtMessage = nullptr;
    {
      // Retrieve the next available tracked frame reply
      for ( auto replyIter = IGTClient->MessagesBegin(); replyIter != IGTClient->MessagesEnd(); ++replyIter )
      {
        if ( typeid( *( *replyIter ) ) == typeid( igtl::TrackedFrameMessage ) )
        {
          igtMessage = *replyIter;
          break;
        }
      }
    }

    if ( igtMessage != nullptr )
    {
      igtl::TrackedFrameMessage::Pointer trackedFrameMsg = dynamic_cast<igtl::TrackedFrameMessage*>( igtMessage.GetPointer() );

      for ( auto pair : trackedFrameMsg->GetMetaData() )
      {
        std::wstring keyWideStr( pair.first.begin(), pair.first.end() );
        std::wstring valueWideStr( pair.second.begin(), pair.second.end() );
        frame->FrameFields->Insert( ref new Platform::String( keyWideStr.c_str() ), ref new Platform::String( valueWideStr.c_str() ) );
      }

      for ( auto pair : trackedFrameMsg->GetCustomFrameFields() )
      {
        std::wstring keyWideStr( pair.first.begin(), pair.first.end() );
        std::wstring valueWideStr( pair.second.begin(), pair.second.end() );
        frame->FrameFields->Insert( ref new Platform::String( keyWideStr.c_str() ), ref new Platform::String( valueWideStr.c_str() ) );
      }

      frame->SetImageSize( trackedFrameMsg->GetFrameSize()[0], trackedFrameMsg->GetFrameSize()[1], trackedFrameMsg->GetFrameSize()[2] );
      frame->ImageSizeBytes = trackedFrameMsg->GetImageSizeInBytes();
      Platform::ArrayReference<unsigned char> arraywrapper( ( unsigned char* )trackedFrameMsg->GetImage(), trackedFrameMsg->GetImageSizeInBytes() );
      auto ibuffer = Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray( arraywrapper );
      frame->SetImageData( ibuffer );
      frame->NumberOfComponents = trackedFrameMsg->GetNumberOfComponents();

      return true;
    }
*/
    return false;
  }

  //----------------------------------------------------------------------------
  bool IGTLConnectorPage::FromNativePointer( IBuffer^ data, uint32 width, uint32 height, uint16 numberOfcomponents, Imaging::WriteableBitmap^ wbm )
  {
    // TODO : copy buffer form data to wbm.pixelbuffer
    return true;
  }

}