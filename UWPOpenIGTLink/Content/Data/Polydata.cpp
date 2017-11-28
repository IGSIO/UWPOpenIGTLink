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
#include "Polydata.h"

using namespace Windows::Foundation::Numerics;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  Platform::String^ Material::Name::get()
  {
    return m_name;
  }

  //----------------------------------------------------------------------------
  void Material::Name::set(Platform::String^ arg)
  {
    m_name = arg;
  }

  //----------------------------------------------------------------------------
  float4 Material::Emissive::get()
  {
    return m_emissive;
  }

  //----------------------------------------------------------------------------
  void Material::Emissive::set(float4 arg)
  {
    m_emissive = arg;
  }

  //----------------------------------------------------------------------------
  float4 Material::Specular::get()
  {
    return m_specular;
  }

  //----------------------------------------------------------------------------
  void Material::Specular::set(float4 arg)
  {
    m_specular = arg;
  }

  //----------------------------------------------------------------------------
  float4 Material::Diffuse::get()
  {
    return m_diffuse;
  }

  //----------------------------------------------------------------------------
  void Material::Diffuse::set(float4 arg)
  {
    m_diffuse = arg;
  }

  //----------------------------------------------------------------------------
  float4 Material::Ambient::get()
  {
    return m_ambient;
  }

  //----------------------------------------------------------------------------
  void Material::Ambient::set(float4 arg)
  {
    m_ambient = arg;
  }

  //----------------------------------------------------------------------------
  float Material::Transparency::get()
  {
    return m_transparency;
  }

  //----------------------------------------------------------------------------
  void Material::Transparency::set(float arg)
  {
    m_transparency = arg;
  }

  //----------------------------------------------------------------------------
  float Material::SpecularExponent::get()
  {
    return m_specularExponent;
  }

  //----------------------------------------------------------------------------
  void Material::Model::set(IlluminationModel arg)
  {
    m_model = arg;
  }

  //----------------------------------------------------------------------------
  IlluminationModel Material::Model::get()
  {
    return m_model;
  }

  //----------------------------------------------------------------------------
  void Material::SpecularExponent::set(float arg)
  {
    m_specularExponent = arg;
  }

  //----------------------------------------------------------------------------
  Material^ Polydata::Mat::get()
  {
    return m_material;
  }

  //----------------------------------------------------------------------------
  void Polydata::Mat::set(Material^ arg)
  {
    m_material = arg;
  }

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
  Polydata::Float4List^ Polydata::Colours::get()
  {
    return m_colours;
  }

  //----------------------------------------------------------------------------
  void Polydata::Colours::set(Polydata::Float4List^ arg)
  {
    m_colours->Clear();
    for (auto& entry : arg)
    {
      m_colours->Append(entry);
    }
  }

  //----------------------------------------------------------------------------
  Polydata::Float3List^ Polydata::Points::get()
  {
    return m_positions;
  }

  //----------------------------------------------------------------------------
  void Polydata::Points::set(Polydata::Float3List^ arg)
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