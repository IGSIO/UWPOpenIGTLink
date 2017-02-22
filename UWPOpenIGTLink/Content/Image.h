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

// IGTL includes
#include <igtl_util.h>

namespace UWPOpenIGTLink
{
  public ref class Image sealed
  {
  public:
    Image();
    virtual ~Image();

    property FrameSizeABI^ Dimensions { FrameSizeABI ^ get(); void set(const FrameSizeABI ^ arg); }
    property Windows::Storage::Streams::IBuffer^ ImageData { Windows::Storage::Streams::IBuffer ^ get(); void set(Windows::Storage::Streams::IBuffer ^ data); }
    property uint16 NumberOfScalarComponents { uint16 get(); void set(uint16 arg); }
    property int ScalarType { int get(); void set(int arg); }

    bool DeepCopy(Image^ otherImage);
    bool FillBlank();
    void AllocateScalars(const FrameSizeABI^ imageSize, uint16 numberOfScalarComponents, int scalarType);
    uint32 GetImageSizeBytes();
    uint32 GetPixelFormat(bool normalized);
    static uint32 GetNumberOfBytesPerScalar(int scalarType);

    SharedBytePtr GetImageData() { return (SharedBytePtr)&m_imageData; }

  internal:
    void SetImageData(std::shared_ptr<byte> imageData, uint16 numberOfScalarComponents, IGTL_SCALAR_TYPE scalarType, const FrameSize& imageSize);
    std::shared_ptr<byte> GetImageDataInternal();
    void AllocateScalars(const FrameSize& imageSize, uint16 numberOfScalarComponents, IGTL_SCALAR_TYPE pixelType);

    FrameSize GetFrameSize() const;

  protected private:
    FrameSize                                 m_frameSize;
    std::shared_ptr<byte>                     m_imageData;
    uint16                                    m_numberOfScalarComponents;
    IGTL_SCALAR_TYPE                          m_scalarType;
  };
}