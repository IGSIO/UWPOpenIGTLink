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

#pragma once

namespace WF = Windows::Foundation;
namespace WFC = WF::Collections;
namespace WFM = WF::Metadata;
namespace WSS = Windows::Storage::Streams;
namespace WUXC = Windows::UI::Xaml::Controls;
namespace WUXM = Windows::UI::Xaml::Media;

// STD includes
#include <map>

namespace UWPOpenIGTLink
{
  [Windows::Foundation::Metadata::WebHostHiddenAttribute]
  public ref class TrackedFrameMessageCx sealed
  {
  public:
    property bool Result {bool get(); void set( bool arg ); }
    property WFC::IMap<Platform::String^, Platform::String^>^ Parameters {WFC::IMap<Platform::String^, Platform::String^>^ get(); void set( WFC::IMap<Platform::String^, Platform::String^>^ arg ); }
    property WUXM::Imaging::WriteableBitmap^ ImageSource {WUXM::Imaging::WriteableBitmap ^ get(); void set( WUXM::Imaging::WriteableBitmap ^ arg ); }

    WFC::IMapView<Platform::String^, Platform::String^>^ GetValidTransforms();

  protected private:
    bool m_result;
    WFC::IMap<Platform::String^, Platform::String^>^ m_parameters;
    WUXM::Imaging::WriteableBitmap^ m_imageSource;
  };

  public ref class TrackedFrameMessage sealed
  {
  public:
    property bool Result {bool get(); void set(bool arg); }
    property WFC::IMap<Platform::String^, Platform::String^>^ Parameters {WFC::IMap<Platform::String^, Platform::String^>^ get(); }
    property int32 ImageSizeBytes { int32 get(); void set(int32 arg); }
    property int32 NumberOfComponents { int32 get(); void set(int32 arg); }

    WFC::IVectorView<uint32>^ GetImageSize();
    void SetImageSize(uint32 x, uint32 y, uint32 z);

    void SetParameter(Platform::String^ key, Platform::String^ value);

    WSS::IBuffer^ GetImageData();
    void SetImageData(WSS::IBuffer^ imageData);

    WFC::IMapView<Platform::String^, Platform::String^>^ GetValidTransforms();

  protected private:
    bool m_result;
    std::map<std::wstring, std::wstring> m_parameters;
    uint8* m_imageData;
    uint32 m_imageSize[3];
    int32 m_numberOfComponents;
    int32 m_imageSizeBytes;
  };
}