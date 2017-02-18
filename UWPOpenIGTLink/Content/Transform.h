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

namespace UWPOpenIGTLink
{
  ref class TransformName;

  public ref class Transform sealed
  {
  public:
    property TransformName^ Name { TransformName ^ get(); void set(TransformName ^ arg); }
    property Windows::Foundation::Numerics::float4x4 Matrix { Windows::Foundation::Numerics::float4x4 get(); void set(Windows::Foundation::Numerics::float4x4 arg); }
    property bool Valid { bool get(); void set(bool arg); }
    property double Timestamp { double get(); void set(double arg); }

    void ScaleTranslationComponent(float scalingFactor);
    void Transpose();

  protected private:
    TransformName^                          m_transformName;
    Windows::Foundation::Numerics::float4x4 m_transform;
    double                                  m_timestamp;
    bool                                    m_isValid;
  };
}