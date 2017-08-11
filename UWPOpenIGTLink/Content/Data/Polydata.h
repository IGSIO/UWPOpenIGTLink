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

#include <stdint.h>

namespace UWPOpenIGTLink
{
  public ref class Polydata sealed
  {
  public:
    typedef Windows::Foundation::Collections::IVector<Windows::Foundation::Numerics::float3> Float3List;
    typedef Windows::Foundation::Collections::IVector<uint16> IndexList;

    property double       Timestamp {double get(); void set(double arg); }
    property Float3List^  Positions {Float3List ^ get(); void set(Float3List ^ arg);}
    property Float3List^  Normals {Float3List ^ get(); void set(Float3List ^ arg);}
    property Float3List^  TextureCoords {Float3List ^ get(); void set(Float3List ^ arg);}
    property IndexList^   Indices {IndexList ^ get(); void set(IndexList ^ arg);}

  protected private:
    typedef Platform::Collections::Vector<Windows::Foundation::Numerics::float3>  Float3ListInternal;
    typedef Platform::Collections::Vector<uint16>                                 IndexListInternal;

    double                      m_timestamp = 0.0;
    Float3ListInternal^         m_positions = ref new Float3ListInternal;
    Float3ListInternal^         m_normals = ref new Float3ListInternal;
    Float3ListInternal^         m_textureCoords = ref new Float3ListInternal;
    IndexListInternal^          m_indicies = ref new IndexListInternal;
  };
}