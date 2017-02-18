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

// Local includes
#include "IGTCommon.h"
#include "Image.h"

namespace UWPOpenIGTLink
{
  public ref class VideoFrame sealed
  {
  public:
    VideoFrame();
    virtual ~VideoFrame();

    property FrameSizeABI^ Dimensions { FrameSizeABI ^ get(); }
    property UWPOpenIGTLink::Image^ Image { UWPOpenIGTLink::Image ^ get();  }
    property Windows::Storage::Streams::IBuffer^ ImageData { Windows::Storage::Streams::IBuffer ^ get(); }
    property Windows::Foundation::Numerics::float4x4 EmbeddedImageTransform { Windows::Foundation::Numerics::float4x4 get(); void set(Windows::Foundation::Numerics::float4x4 arg); }
    property uint16 NumberOfScalarComponents { uint16 get(); }
    property int ScalarType { int get(); }
    property uint16 Type { uint16 get(); void set(uint16 arg); }
    property uint16 Orientation { uint16 get(); void set(uint16 arg); }

    // Memory based operations
    bool DeepCopy(VideoFrame^ DataBufferItem);
    bool DeepCopy(UWPOpenIGTLink::Image^ frame, int usImageOrientation, int usImageType);
    bool ShallowCopy(UWPOpenIGTLink::Image^ frame, int usImageOrientation, int usImageType);
    bool AllocateFrame(const FrameSizeABI^ imageSize, int scalarType, uint16 numberOfScalarComponents);
    bool FillBlank();

    // Accessors
    int GetScalarPixelType();

    int GetImageOrientation();
    void SetImageOrientation(int imgOrientation);

    uint32 GetNumberOfScalarComponents();

    int GetImageType();
    void SetImageType(int imgType);

    uint32 GetNumberOfBytesPerScalar();
    uint32 GetNumberOfBytesPerPixel();
    uint64 GetFrameSizeBytes();

    FrameSizeABI^ GetDimensions();

    bool HasImage();
    UWPOpenIGTLink::Image^ GetImage();

    uint32 GetPixelFormat(bool normalized);

    // Helper functions
    static Platform::String^ GetStringFromScalarPixelType(int pixelType);
    static uint32 GetNumberOfBytesPerScalar(int pixelType);
    static int GetUsImageOrientationFromString(Platform::String^ imgOrientationStr);
    static Platform::String^ GetStringFromUsImageOrientation(int imgOrientation);
    static int GetUsImageTypeFromString(Platform::String^ imgTypeStr);
    static Platform::String^ GetStringFromUsImageType(int imgType);

  internal:
    bool AllocateFrame(const FrameSize& imageSize, int scalarType, uint16 numberOfScalarComponents);
    void SetImageData(std::shared_ptr<byte> imageData, uint16 numberOfScalarComponents, IGTL_SCALAR_TYPE scalarType, const FrameSize& imageSize);

    std::shared_ptr<byte> GetImageData();
    bool IsImageValidInternal() const;
    FrameSize GetDimensions() const;

    UWPOpenIGTLink::Image^  m_image;
    US_IMAGE_TYPE           m_imageType;
    US_IMAGE_ORIENTATION    m_imageOrientation;
  };
}