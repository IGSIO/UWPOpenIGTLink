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
#include "pch.h"
#include "Polydata.h"

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  double Polydata::Timestamp::get()
  {
    return m_timestamp;
  }

  //----------------------------------------------------------------------------
  void Polydata::Timestamp::set(double arg)
  {
    m_timestamp = arg;
  }

  //----------------------------------------------------------------------------
  Polydata::Float3List^ Polydata::Positions::get()
  {
    return m_positions;
  }

  //----------------------------------------------------------------------------
  void Polydata::Positions::set(Polydata::Float3List^ arg)
  {
    m_positions->Clear();
    for (auto& entry : arg)
    {
      m_positions->Append(entry);
    }
  }

  //----------------------------------------------------------------------------
  Polydata::Float3List^ Polydata::Normals::get()
  {
    return m_normals;
  }

  //----------------------------------------------------------------------------
  void Polydata::Normals::set(Polydata::Float3List^ arg)
  {
    m_normals->Clear();
    for (auto& entry : arg)
    {
      m_normals->Append(entry);
    }
  }

  //----------------------------------------------------------------------------
  Polydata::Float3List^ Polydata::TextureCoords::get()
  {
    return m_textureCoords;
  }

  //----------------------------------------------------------------------------
  void Polydata::TextureCoords::set(Polydata::Float3List^ arg)
  {
    m_textureCoords->Clear();
    for (auto& entry : arg)
    {
      m_textureCoords->Append(entry);
    }
  }

  //----------------------------------------------------------------------------
  Polydata::IndexList^ Polydata::Indices::get()
  {
    return m_indicies;
  }

  //----------------------------------------------------------------------------
  void Polydata::Indices::set(Polydata::IndexList^ arg)
  {
    m_indicies->Clear();
    for (auto& entry : arg)
    {
      m_indicies->Append(entry);
    }
  }
}