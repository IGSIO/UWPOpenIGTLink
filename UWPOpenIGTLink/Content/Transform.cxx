/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.

Modified by Adam Rankin, Robarts Research Institute, 2017

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

=========================================================Plus=header=end*/

// Local includes
#include "pch.h"
#include "Transform.h"
#include "TransformName.h"

using namespace Windows::Foundation::Numerics;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  TransformName^ Transform::Name::get()
  {
    return m_transformName;
  }

  //----------------------------------------------------------------------------
  void Transform::Name::set(TransformName^ arg)
  {
    m_transformName = arg;
  }

  //----------------------------------------------------------------------------
  float4x4 Transform::Matrix::get()
  {
    return m_transform;
  }

  //----------------------------------------------------------------------------
  void Transform::Matrix::set(float4x4 arg)
  {
    m_transform = arg;
  }

  //----------------------------------------------------------------------------
  bool Transform::Valid::get()
  {
    return m_isValid;
  }

  //----------------------------------------------------------------------------
  void Transform::Valid::set(bool arg)
  {
    m_isValid = arg;
  }

  //----------------------------------------------------------------------------
  double Transform::Timestamp::get()
  {
    return m_timestamp;
  }

  //----------------------------------------------------------------------------
  void Transform::Timestamp::set(double arg)
  {
    m_timestamp = arg;
  }

  //----------------------------------------------------------------------------
  Transform::Transform(TransformName^ name, Windows::Foundation::Numerics::float4x4 matrix, bool valid, double timestamp)
    : m_transformName(name)
    , m_transform(matrix)
    , m_isValid(valid)
    , m_timestamp(timestamp)
  {
    if (m_transformName == nullptr)
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Null transform name sent to Transform.");
    }
  }

  //----------------------------------------------------------------------------
  void Transform::ScaleTranslationComponent(float scalingFactor)
  {
    m_transform.m14 *= scalingFactor;
    m_transform.m24 *= scalingFactor;
    m_transform.m34 *= scalingFactor;
  }

  //----------------------------------------------------------------------------
  void Transform::Transpose()
  {
    m_transform = transpose(m_transform);
  }
}