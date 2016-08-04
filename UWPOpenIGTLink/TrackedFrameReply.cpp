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

namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  bool TrackedFrameReply::Result::get()
  {
    return m_Result;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameReply::Result::set( bool arg )
  {
    m_Result = arg;
  }

  //----------------------------------------------------------------------------
  WFC::IMap<Platform::String^, Platform::String^>^ TrackedFrameReply::Parameters::get()
  {
    return m_Parameters;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameReply::Parameters::set( WFC::IMap<Platform::String^, Platform::String^>^ arg )
  {
    m_Parameters = arg;
  }

  //----------------------------------------------------------------------------
  WUXM::Imaging::WriteableBitmap^ TrackedFrameReply::ImageSource::get()
  {
    return m_ImageSource;
  }

  //----------------------------------------------------------------------------
  void TrackedFrameReply::ImageSource::set( WUXM::Imaging::WriteableBitmap^ arg )
  {
    m_ImageSource = arg;
  }

  //----------------------------------------------------------------------------
  WFC::IMapView<Platform::String^, Platform::String^>^ TrackedFrameReply::GetValidTransforms()
  {
    Platform::Collections::Map<Platform::String^, Platform::String^>^ outputMap = ref new Platform::Collections::Map<Platform::String^, Platform::String^>;
    for ( auto entry : m_Parameters )
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
          outputMap->Insert( refLookupKey, m_Parameters->Lookup( refLookupKey ) );
        }
      }
    }

    return outputMap->GetView();
  }
}