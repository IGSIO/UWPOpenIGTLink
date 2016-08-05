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
#include "TrackedFrameReply.h"

// Windows includes
#include <collection.h>
#include <robuffer.h>

namespace PC = Platform::Collections;
namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WSS = Windows::Storage::Streams;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  bool TrackedFrameMessageCx::Result::get()
  {
    return m_result;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessageCx::Result::set( bool arg )
  {
    m_result = arg;
  }

  //----------------------------------------------------------------------------
  WFC::IMap<Platform::String^, Platform::String^>^ TrackedFrameMessageCx::Parameters::get()
  {
    return m_parameters;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessageCx::Parameters::set( WFC::IMap<Platform::String^, Platform::String^>^ arg )
  {
    m_parameters = arg;
  }

  //----------------------------------------------------------------------------
  WUXM::Imaging::WriteableBitmap^ TrackedFrameMessageCx::ImageSource::get()
  {
    return m_imageSource;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessageCx::ImageSource::set( WUXM::Imaging::WriteableBitmap^ arg )
  {
    m_imageSource = arg;
  }

  //----------------------------------------------------------------------------
  WFC::IMapView<Platform::String^, Platform::String^>^ TrackedFrameMessageCx::GetValidTransforms()
  {
    Platform::Collections::Map<Platform::String^, Platform::String^>^ outputMap = ref new Platform::Collections::Map<Platform::String^, Platform::String^>;
    for ( auto entry : m_parameters )
    {
      std::wstring key( entry->Key->Data() );
      std::wstring value( entry->Value->Data() );

      if ( key.find( L"TransformStatus" ) != std::wstring::npos )
      {
        if ( value.compare( L"OK" ) != 0 )
        {
          continue;
        }
        else
        {
          // This entry is valid, so find the corresponding transform and put it in the list
          //ImageToCroppedImageTransformStatus
          //ImageToCroppedImageTransform
          std::wstring lookupKey( key.substr( 0, key.find( L"Status" ) ) );
          Platform::String^ refLookupKey = ref new Platform::String( lookupKey.c_str() );
          outputMap->Insert( refLookupKey, m_parameters->Lookup( refLookupKey ) );
        }
      }
    }

    return outputMap->GetView();
  }

  //----------------------------------------------------------------------------
  bool TrackedFrameMessage::Result::get()
  {
    return m_result;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::Result::set( bool arg )
  {
    m_result = arg;
  }

  //----------------------------------------------------------------------------
  int32 TrackedFrameMessage::ImageSizeBytes::get()
  {
    return m_imageSizeBytes;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::ImageSizeBytes::set( int32 arg )
  {
    m_imageSizeBytes = arg;
  }

  //----------------------------------------------------------------------------
  int32 TrackedFrameMessage::NumberOfComponents::get()
  {
    return m_numberOfComponents;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::NumberOfComponents::set( int32 arg )
  {
    m_numberOfComponents = arg;
  }

  //----------------------------------------------------------------------------
  WFC::IMap<Platform::String^, Platform::String^>^ TrackedFrameMessage::Parameters::get()
  {
    auto map = ref new PC::Map<Platform::String^, Platform::String^>();
    for (auto pair : m_parameters)
    {
      map->Insert(ref new Platform::String(pair.first.c_str()), ref new Platform::String(pair.second.c_str()));
    }
    return map;
  }

  //----------------------------------------------------------------------------
  WFC::IVectorView<uint32>^ TrackedFrameMessage::GetImageSize()
  {
    auto vec = ref new Platform::Collections::Vector<uint32>();
    vec->Append( m_imageSize[0] );
    vec->Append( m_imageSize[1] );
    vec->Append( m_imageSize[2] );
    return vec->GetView();
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::SetImageSize( uint32 x, uint32 y, uint32 z )
  {
    m_imageSize[0] = x;
    m_imageSize[1] = x;
    m_imageSize[2] = x;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::SetParameter( Platform::String^ key, Platform::String^ value )
  {
    m_parameters[std::wstring( key->Data() )] = std::wstring( value->Data() );
  }

  //----------------------------------------------------------------------------
  WSS::IBuffer^ TrackedFrameMessage::GetImageData()
  {
    Platform::ArrayReference<unsigned char> arraywrapper( ( unsigned char* )m_imageData, m_imageSizeBytes );
    return Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray( arraywrapper );
  }

  //----------------------------------------------------------------------------
  void TrackedFrameMessage::SetImageData( WSS::IBuffer^ imageData )
  {
    // Cast to Object^, then to its underlying IInspectable interface.
    Object^ obj = imageData;
    Microsoft::WRL::ComPtr<IInspectable> insp( reinterpret_cast<IInspectable*>( obj ) );

    // Query the IBufferByteAccess interface.
    Microsoft::WRL::ComPtr<WSS::IBufferByteAccess> bufferByteAccess;
    if( insp.As( &bufferByteAccess ) < 0 )
    {
      throw std::exception( "Cannot cast to IBufferByteAccess." );
    }

    // Retrieve the buffer data.
    if ( bufferByteAccess->Buffer( &m_imageData ) < 0 )
    {
      throw std::exception( "Cannot cast to byte*." );
    }
  }

  //----------------------------------------------------------------------------
  WFC::IMapView<Platform::String^, Platform::String^>^ TrackedFrameMessage::GetValidTransforms()
  {
    Platform::Collections::Map<Platform::String^, Platform::String^>^ outputMap = ref new Platform::Collections::Map<Platform::String^, Platform::String^>;
    for ( auto entry : m_parameters )
    {
      if ( entry.first.find( L"TransformStatus" ) != std::wstring::npos )
      {
        if ( entry.second.compare( L"OK" ) != 0 )
        {
          continue;
        }
        else
        {
          // This entry is valid, so find the corresponding transform and put it in the list
          //ImageToCroppedImageTransformStatus
          //ImageToCroppedImageTransform
          std::wstring lookupKey( entry.first.substr( 0, entry.first.find( L"Status" ) ) );
          Platform::String^ refLookupKey = ref new Platform::String( lookupKey.c_str() );
          outputMap->Insert( refLookupKey, ref new Platform::String( m_parameters[lookupKey].c_str() ) );
        }
      }
    }

    return outputMap->GetView();
  }
}