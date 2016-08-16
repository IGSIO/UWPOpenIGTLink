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
#include <WindowsNumerics.h>
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
  void TrackedFrame::NumberOfComponents::set( uint16 arg )
  {
    m_numberOfComponents = arg;
  }

  //----------------------------------------------------------------------------
  IVectorView<uint16>^ TrackedFrame::FrameSize::get()
  {
    auto vec = ref new Vector<uint16>();
    vec->Append( m_frameSize[0] );
    vec->Append( m_frameSize[1] );
    vec->Append( m_frameSize[2] );
    return vec->GetView();
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::FrameSize::set( IVectorView<uint16>^ arg )
  {
    if ( arg->Size > 0 )
    {
      m_frameSize[0] = arg->GetAt( 0 );
    }
    if ( arg->Size > 1 )
    {
      m_frameSize[1] = arg->GetAt( 1 );
    }
    if ( arg->Size > 2 )
    {
      m_frameSize[2] = arg->GetAt( 2 );
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
  void TrackedFrame::SetParameter( Platform::String^ key, Platform::String^ value )
  {
    m_frameFields[std::wstring( key->Data() )] = std::wstring( value->Data() );
  }

  //----------------------------------------------------------------------------
  IBuffer^ TrackedFrame::ImageData::get()
  {
    Platform::ArrayReference<byte> arraywrapper( *m_imageData, m_frameSizeBytes );
    return Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray( arraywrapper );
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageData::set( IBuffer^ imageData )
  {
    if ( imageData == nullptr )
    {
      return;
    }

    unsigned int bufferLength = imageData->Length;

    if ( !( bufferLength > 0 ) )
    {
      return;
    }

    HRESULT hr = S_OK;

    Microsoft::WRL::ComPtr<IUnknown> pUnknown = reinterpret_cast<IUnknown*>( imageData );
    Microsoft::WRL::ComPtr<IBufferByteAccess> spByteAccess;
    hr = pUnknown.As( &spByteAccess );
    if ( FAILED( hr ) )
    {
      return;
    }

    byte* pRawData = nullptr;
    hr = spByteAccess->Buffer( &pRawData );
    if ( FAILED( hr ) )
    {
      return;
    }

    m_imageData = std::make_shared<uint8*>( reinterpret_cast<byte*>( pRawData ) );
  }

  //----------------------------------------------------------------------------
  int32 TrackedFrame::ScalarType::get()
  {
    return ( int32 )m_scalarType;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ScalarType::set( int32 arg )
  {
    m_scalarType = ( IGTLScalarType )arg;
  }

  //----------------------------------------------------------------------------
  SharedBytePtr TrackedFrame::ImageDataSharedPtr::get()
  {
    return ( SharedBytePtr )&m_imageData;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageDataSharedPtr::set( SharedBytePtr arg )
  {
    m_imageData = *( ( std::shared_ptr<byte*>* )arg );
  }

  //----------------------------------------------------------------------------
  float4x4 TrackedFrame::EmbeddedImageTransform::get()
  {
    float4x4 result;
    XMStoreFloat4x4( &result, XMLoadFloat4x4( &m_embeddedImageTransform ) );
    return result;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::EmbeddedImageTransform::set( float4x4 arg )
  {
    XMStoreFloat4x4( &m_embeddedImageTransform, DirectX::XMLoadFloat4x4( &arg ) );
  }

  //----------------------------------------------------------------------------
  uint32 TrackedFrame::PixelFormat::get()
  {
    switch ( m_numberOfComponents )
    {
    case 1:
      switch ( m_scalarType )
      {
      case IGTL_SCALARTYPE_INT8:
        return DXGI_FORMAT_R8_SNORM;
      case IGTL_SCALARTYPE_UINT8:
        return DXGI_FORMAT_R8_UNORM;
      case IGTL_SCALARTYPE_INT16:
        return DXGI_FORMAT_R16_SNORM;
      case IGTL_SCALARTYPE_UINT16:
        return DXGI_FORMAT_R16_UNORM;
      case IGTL_SCALARTYPE_INT32:
        return DXGI_FORMAT_R32_SINT;
      case IGTL_SCALARTYPE_UINT32:
        return DXGI_FORMAT_R32_UINT;
      case IGTL_SCALARTYPE_FLOAT32:
        return DXGI_FORMAT_R32_FLOAT;
      }
      break;
    case 2:
      switch ( m_scalarType )
      {
      case IGTL_SCALARTYPE_INT8:
        return DXGI_FORMAT_R8G8_SNORM;
      case IGTL_SCALARTYPE_UINT8:
        return DXGI_FORMAT_R8G8_UNORM;
      case IGTL_SCALARTYPE_INT16:
        return DXGI_FORMAT_R16G16_SNORM;
      case IGTL_SCALARTYPE_UINT16:
        return DXGI_FORMAT_R16G16_UNORM;
      case IGTL_SCALARTYPE_INT32:
        return DXGI_FORMAT_R32G32_SINT;
      case IGTL_SCALARTYPE_UINT32:
        return DXGI_FORMAT_R32G32_UINT;
      case IGTL_SCALARTYPE_FLOAT32:
        return DXGI_FORMAT_R32G32_FLOAT;
      }
      break;
    case 3:
      switch ( m_scalarType )
      {
      case IGTL_SCALARTYPE_INT8:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
      case IGTL_SCALARTYPE_UINT8:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
      case IGTL_SCALARTYPE_INT16:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
      case IGTL_SCALARTYPE_UINT16:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
      case IGTL_SCALARTYPE_INT32:
        return DXGI_FORMAT_R32G32B32_SINT;
      case IGTL_SCALARTYPE_UINT32:
        return DXGI_FORMAT_R32G32B32_UINT;
      case IGTL_SCALARTYPE_FLOAT32:
        return DXGI_FORMAT_R32G32B32_FLOAT;
      }
    case 4:
      switch ( m_scalarType )
      {
      case IGTL_SCALARTYPE_INT8:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
      case IGTL_SCALARTYPE_UINT8:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
      case IGTL_SCALARTYPE_INT16:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
      case IGTL_SCALARTYPE_UINT16:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
      case IGTL_SCALARTYPE_INT32:
        return DXGI_FORMAT_R32G32B32A32_SINT;
      case IGTL_SCALARTYPE_UINT32:
        return DXGI_FORMAT_R32G32B32A32_UINT;
      case IGTL_SCALARTYPE_FLOAT32:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
      }
      break;
    }
    return DXGI_FORMAT_UNKNOWN;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::PixelFormat::set( uint32 arg )
  {
    DXGI_FORMAT pixelFormat = ( DXGI_FORMAT )arg;
    switch ( arg )
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      m_numberOfComponents = 4;
      m_scalarType = IGTL_SCALARTYPE_FLOAT32;
      break;
    case DXGI_FORMAT_R32G32B32A32_UINT:
      m_numberOfComponents = 4;
      m_scalarType = IGTL_SCALARTYPE_UINT32;
      break;
    case DXGI_FORMAT_R32G32B32A32_SINT:
      m_numberOfComponents = 4;
      m_scalarType = IGTL_SCALARTYPE_INT32;
      break;
    case DXGI_FORMAT_R32G32B32_FLOAT:
      m_numberOfComponents = 3;
      m_scalarType = IGTL_SCALARTYPE_FLOAT32;
      break;
    case DXGI_FORMAT_R32G32B32_UINT:
      m_numberOfComponents = 3;
      m_scalarType = IGTL_SCALARTYPE_UINT32;
      break;
    case DXGI_FORMAT_R32G32B32_SINT:
      m_numberOfComponents = 3;
      m_scalarType = IGTL_SCALARTYPE_INT32;
      break;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
      m_numberOfComponents = 4;
      m_scalarType = IGTL_SCALARTYPE_UINT16;
      break;
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
      m_numberOfComponents = 4;
      m_scalarType = IGTL_SCALARTYPE_INT16;
      break;
    case DXGI_FORMAT_R32G32_FLOAT:
      m_numberOfComponents = 2;
      m_scalarType = IGTL_SCALARTYPE_FLOAT32;
      break;
    case DXGI_FORMAT_R32G32_UINT:
      m_numberOfComponents = 2;
      m_scalarType = IGTL_SCALARTYPE_UINT32;
      break;
    case DXGI_FORMAT_R32G32_SINT:
      m_numberOfComponents = 2;
      m_scalarType = IGTL_SCALARTYPE_INT32;
      break;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
      m_numberOfComponents = 4;
      m_scalarType = IGTL_SCALARTYPE_UINT8;
      break;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
      m_numberOfComponents = 4;
      m_scalarType = IGTL_SCALARTYPE_INT8;
      break;
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
      m_numberOfComponents = 2;
      m_scalarType = IGTL_SCALARTYPE_UINT16;
      break;
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
      m_numberOfComponents = 2;
      m_scalarType = IGTL_SCALARTYPE_INT16;
      break;
    case DXGI_FORMAT_R32_FLOAT:
      m_numberOfComponents = 1;
      m_scalarType = IGTL_SCALARTYPE_FLOAT32;
      break;
    case DXGI_FORMAT_R32_UINT:
      m_numberOfComponents = 1;
      m_scalarType = IGTL_SCALARTYPE_UINT32;
      break;
    case DXGI_FORMAT_R32_SINT:
      m_numberOfComponents = 1;
      m_scalarType = IGTL_SCALARTYPE_INT32;
      break;
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
      m_numberOfComponents = 2;
      m_scalarType = IGTL_SCALARTYPE_UINT8;
      break;
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
      m_numberOfComponents = 2;
      m_scalarType = IGTL_SCALARTYPE_INT8;
      break;
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
      m_numberOfComponents = 1;
      m_scalarType = IGTL_SCALARTYPE_UINT16;
      break;
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
      m_numberOfComponents = 1;
      m_scalarType = IGTL_SCALARTYPE_INT16;
      break;
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
      m_numberOfComponents = 1;
      m_scalarType = IGTL_SCALARTYPE_UINT8;
      break;
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
      m_numberOfComponents = 1;
      m_scalarType = IGTL_SCALARTYPE_INT8;
      break;
    default:
      // Remaining formats are not supported by IGT
      break;
    }
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::ImageType::get()
  {
    return m_imageType;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageType::set( uint16 arg )
  {
    m_imageType = ( US_IMAGE_TYPE )arg;
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::ImageOrientation::get()
  {
    return m_imageOrientation;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::ImageOrientation::set( uint16 arg )
  {
    m_imageOrientation = ( US_IMAGE_ORIENTATION )arg;
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::Width::get()
  {
    return m_frameSize[0];
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::Height::get()
  {
    return m_frameSize[1];
  }

  //----------------------------------------------------------------------------
  uint16 TrackedFrame::Depth::get()
  {
    return m_frameSize[2];
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

  //----------------------------------------------------------------------------
  void TrackedFrame::SetEmbeddedImageTransform( const DirectX::XMFLOAT4X4& matrix )
  {
    m_embeddedImageTransform = matrix;
  }

  //----------------------------------------------------------------------------
  const DirectX::XMFLOAT4X4& TrackedFrame::GetEmbeddedImageTransform()
  {
    return m_embeddedImageTransform;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetImageData( uint8* imageData, const std::array<uint16, 3>& frameSize )
  {
    m_frameSize = frameSize;
    m_imageData = std::make_shared<uint8*>( imageData );
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetImageData( std::shared_ptr<uint8*> imageData )
  {
    m_imageData = imageData;
  }

  //----------------------------------------------------------------------------
  std::shared_ptr<uint8*> TrackedFrame::GetImageData()
  {
    return m_imageData;
  }
}