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
#include "TrackedFrame.h"

// Windows includes
#include <collection.h>
#include <robuffer.h>

using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  int32 TrackedFrame::ImageSizeBytes::get()
  {
    return m_frameSizeBytes;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageSizeBytes::set( int32 arg )
  {
    m_frameSizeBytes = arg;
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::NumberOfComponents::get()
  {
    return m_numberOfComponents;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::NumberOfComponents::set(uint16 arg )
  {
    m_numberOfComponents = arg;
  }

  //----------------------------------------------------------------------------
  IVectorView<uint16>^ TrackedFrame::FrameSize::get()
  {
    auto vec = ref new Vector<uint16>();
    vec->Append(m_frameSize[0]);
    vec->Append(m_frameSize[1]);
    vec->Append(m_frameSize[2]);
    return vec->GetView();
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::FrameSize::set(IVectorView<uint16>^ arg)
  {
    if (arg->Size > 0)
    {
      m_frameSize[0] = arg->GetAt(0);
    }
    if (arg->Size > 1)
    {
      m_frameSize[1] = arg->GetAt(1);
    }
    if (arg->Size > 2)
    {
      m_frameSize[2] = arg->GetAt(2);
    }
  }

  //----------------------------------------------------------------------------
  IMap<Platform::String^, Platform::String^>^ TrackedFrame::FrameFields::get()
  {
    auto map = ref new Map<Platform::String^, Platform::String^>();
    for ( auto pair : m_frameFields )
    {
      map->Insert( ref new Platform::String( pair.first.c_str() ), ref new Platform::String( pair.second.c_str() ) );
    }
    return map;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetParameter(Platform::String^ key, Platform::String^ value )
  {
    m_frameFields[std::wstring( key->Data() )] = std::wstring( value->Data() );
  }

  //----------------------------------------------------------------------------
  IBuffer^ TrackedFrame::ImageData::get()
  {
    Platform::ArrayReference<byte> arraywrapper( m_imageData, m_frameSizeBytes );
    return Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray( arraywrapper );
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageData::set( IBuffer^ imageData )
  {
    // Cast to Object^, then to its underlying IInspectable interface.
    Object^ obj = imageData;
    Microsoft::WRL::ComPtr<IInspectable> insp( reinterpret_cast<IInspectable*>( obj ) );

    // Query the IBufferByteAccess interface.
    Microsoft::WRL::ComPtr<IBufferByteAccess> bufferByteAccess;
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
  int32 TrackedFrame::ScalarType::get()
  {
    return (int32)m_scalarType;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ScalarType::set(int32 arg)
  {
    m_scalarType = (IGTLScalarType)arg;
  }

  //----------------------------------------------------------------------------
  IMapView<Platform::String^, Platform::String^>^ TrackedFrame::GetValidTransforms()
  {
    Map<Platform::String^, Platform::String^>^ outputMap = ref new Map<Platform::String^, Platform::String^>;
    for ( auto entry : m_frameFields )
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
          outputMap->Insert( refLookupKey, ref new Platform::String( m_frameFields[lookupKey].c_str() ) );
        }
      }
    }

    return outputMap->GetView();
  }
}