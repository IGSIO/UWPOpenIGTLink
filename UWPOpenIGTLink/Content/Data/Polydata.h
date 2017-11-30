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

#pragma once

#include <stdint.h>

namespace UWPOpenIGTLink
{
  public enum class IlluminationModel
  {
    ILLUM_UNKNOWN,
    ILLUM_COLOR_ON_AMBIENT_OFF,
    ILLUM_COLOR_ON_AMBIENT_ON,
    ILLUM_HIGHLIGHT,
    ILLUM_REFLECTION_RAYTRACE,
    ILLUM_GLASS_RAYTRACE,
    ILLUM_FRESNEL_RAYTRACE,
    ILLUM_REFRACTION_RAYTRACE,
    ILLUM_REFRACTION_FRESNEL_RAYTRACE,
    ILLUM_REFLECTION,
    ILLUM_GLASS,
    ILLUM_SHADOW_INVISIBLE_SURFACES
  };

  public ref class Material sealed
  {
  public:
    property Windows::Foundation::Numerics::float4  Ambient {Windows::Foundation::Numerics::float4 get(); void set(Windows::Foundation::Numerics::float4 arg); }
    property Windows::Foundation::Numerics::float4  Diffuse {Windows::Foundation::Numerics::float4 get(); void set(Windows::Foundation::Numerics::float4 arg); }
    property Windows::Foundation::Numerics::float4  Specular {Windows::Foundation::Numerics::float4 get(); void set(Windows::Foundation::Numerics::float4 arg); }
    property Windows::Foundation::Numerics::float4  Emissive {Windows::Foundation::Numerics::float4 get(); void set(Windows::Foundation::Numerics::float4 arg); }
    property float                                  Transparency {float get(); void set(float arg); }
    property float                                  SpecularExponent {float get(); void set(float arg); }
    property IlluminationModel                      Model {IlluminationModel get(); void set(IlluminationModel arg); }
    property Platform::String^                      Name {Platform::String^ get(); void set(Platform::String^ arg); }

  protected private:
    Windows::Foundation::Numerics::float4 m_ambient = { 0.2f, 0.2f, 0.2f, 1.f };
    Windows::Foundation::Numerics::float4 m_diffuse = { 0.8f, 0.8f, 0.8f, 1.f };
    Windows::Foundation::Numerics::float4 m_specular = { 0.f, 0.f, 0.f, 1.f };
    Windows::Foundation::Numerics::float4 m_emissive = { 0.f, 0.f, 0.f, 1.f };
    Windows::Foundation::Numerics::float4x4 m_uvTransform = Windows::Foundation::Numerics::float4x4::identity();
    float m_transparency = 0.f;
    float m_specularExponent = 1.f;
    IlluminationModel m_model = IlluminationModel::ILLUM_COLOR_ON_AMBIENT_ON;
    Platform::String^ m_name = L"default_material";
  };

  public ref class Polydata sealed
  {
  public:
    typedef Windows::Foundation::Collections::IVector<Windows::Foundation::Numerics::float4> Float4List;
    typedef Windows::Foundation::Collections::IVector<Windows::Foundation::Numerics::float3> Float3List;
    typedef Windows::Foundation::Collections::IVector<uint16> IndexList;

    property double       Timestamp {double get(); void set(double arg);}
    property Float3List^  Points {Float3List ^ get(); void set(Float3List ^ arg);}
    property Float3List^  Normals {Float3List ^ get(); void set(Float3List ^ arg);}
    property Float3List^  TextureCoords {Float3List ^ get(); void set(Float3List ^ arg);}
    property Float4List^  Colours {Float4List ^ get(); void set(Float4List ^ arg);}
    property IndexList^   Indices {IndexList ^ get(); void set(IndexList ^ arg);}
    property Material^    Mat {Material^ get(); void set(Material^ arg); }

  protected private:
    typedef Platform::Collections::Vector<Windows::Foundation::Numerics::float4>  Float4ListInternal;
    typedef Platform::Collections::Vector<Windows::Foundation::Numerics::float3>  Float3ListInternal;
    typedef Platform::Collections::Vector<uint16>                                 IndexListInternal;

    double                      m_timestamp = 0.0;
    Float3ListInternal^         m_positions = ref new Float3ListInternal;
    Float3ListInternal^         m_normals = ref new Float3ListInternal;
    Float4ListInternal^         m_colours = ref new Float4ListInternal;
    Float3ListInternal^         m_textureCoords = ref new Float3ListInternal;
    IndexListInternal^          m_indicies = ref new IndexListInternal;
    Material^                   m_material = ref new Material;
  };
}