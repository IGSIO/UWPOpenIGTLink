/*====================================================================
Copyright(c) 2017 Adam Rankin


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
#include "TrackedFrame.h"
#include "Transform.h"

// Windows includes
#include <WindowsNumerics.h>
#include <collection.h>
#include <dxgiformat.h>
#include <robuffer.h>

// STL includes
#include <sstream>

using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;

namespace UWPOpenIGTLink
{
  static std::wstring TRANSFORM_POSTFIX = L"Transform";
  static std::wstring TRANSFORM_STATUS_POSTFIX = L"TransformStatus";

  //----------------------------------------------------------------------------
  FrameFieldsABI^ TrackedFrame::Fields::get()
  {
    auto map = ref new Map<Platform::String^, Platform::String^>();
    for (auto pair : m_frameFields)
    {
      map->Insert(ref new Platform::String(pair.first.c_str()), ref new Platform::String(pair.second.c_str()));
    }
    return map;
  }

  //----------------------------------------------------------------------------
  bool TrackedFrame::HasImage()
  {
    return m_frame->HasImage();
  }

  //----------------------------------------------------------------------------
  UWPOpenIGTLink::SharedBytePtr TrackedFrame::GetImageData()
  {
    if (HasImage())
    {
      return m_frame->Image->GetImageData();
    }
    return 0;
  }

  //----------------------------------------------------------------------------
  uint32 TrackedFrame::GetPixelFormat(bool normalized)
  {
    return m_frame->GetPixelFormat(normalized);
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetFrameField(Platform::String^ key, Platform::String^ value)
  {
    m_frameFields[std::wstring(key->Data())] = std::wstring(value->Data());
  }

  //----------------------------------------------------------------------------
  double TrackedFrame::Timestamp::get()
  {
    return m_timestamp;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::Timestamp::set(double arg)
  {
    m_timestamp = arg;
  }

  //----------------------------------------------------------------------------
  TrackedFrame::TrackedFrame()
  {

  }

  //----------------------------------------------------------------------------
  TrackedFrame::~TrackedFrame()
  {

  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetTransform(Transform^ transform)
  {
    auto x = GetTransform(transform->Name);
    if (x != nullptr)
    {
      x->Name = transform->Name;
      x->Matrix = transform->Matrix;
      x->Valid = transform->Valid;
    }
    else
    {
      m_frameTransforms.push_back(transform);
    }
  }

  //----------------------------------------------------------------------------
  Transform^ TrackedFrame::GetTransform(TransformName^ transformName)
  {
    auto iter = std::find_if(m_frameTransforms.begin(), m_frameTransforms.end(), [this, transformName](Transform ^ entry)
    {
      return entry->Name == transformName;
    });
    if (iter == m_frameTransforms.end())
    {
      return nullptr;
    }
    return *iter;
  }

  //----------------------------------------------------------------------------
  Platform::String^ TrackedFrame::GetFrameField(Platform::String^ fieldName)
  {
    std::wstring field(fieldName->Data());
    FrameFields::iterator fieldIterator;
    fieldIterator = m_frameFields.find(field);
    if (fieldIterator != m_frameFields.end())
    {
      return ref new Platform::String(fieldIterator->second.c_str());
    }
    return nullptr;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetFrameSize(const FrameSize& frameSize)
  {
    m_frameSize = frameSize;
  }

  //----------------------------------------------------------------------------
  UWPOpenIGTLink::FrameSize TrackedFrame::GetFrameSize() const
  {
    return m_frameSize;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetFrameField(const std::wstring& fieldName, const std::wstring& value)
  {
    m_frameFields[fieldName] = value;
  }

  //----------------------------------------------------------------------------
  bool TrackedFrame::GetFrameField(const std::wstring& fieldName, std::wstring& value)
  {
    FrameFields::iterator fieldIterator;
    fieldIterator = m_frameFields.find(fieldName);
    if (fieldIterator != m_frameFields.end())
    {
      value = fieldIterator->second;
      return true;
    }
    return false;
  }

  //----------------------------------------------------------------------------
  bool TrackedFrame::IsTransform(const std::wstring& str)
  {
    if (str.length() <= TRANSFORM_POSTFIX.length())
    {
      return false;
    }

    return !str.substr(str.length() - TRANSFORM_POSTFIX.length()).compare(TRANSFORM_POSTFIX);
  }

  //----------------------------------------------------------------------------
  bool TrackedFrame::IsTransformStatus(const std::wstring& str)
  {
    if (str.length() <= TRANSFORM_STATUS_POSTFIX.length())
    {
      return false;
    }

    return !str.substr(str.length() - TRANSFORM_STATUS_POSTFIX.length()).compare(TRANSFORM_STATUS_POSTFIX);
  }

  //----------------------------------------------------------------------------
  FIELD_STATUS TrackedFrame::ConvertFieldStatusFromString(const std::wstring& statusStr)
  {
    FIELD_STATUS status = FIELD_INVALID;
    if (statusStr.compare(L"OK") == 0)
    {
      status = FIELD_OK;
    }

    return status;
  }

  //----------------------------------------------------------------------------
  std::wstring TrackedFrame::ConvertFieldStatusToString(FIELD_STATUS status)
  {
    return status == FIELD_OK ? std::wstring(L"OK") : std::wstring(L"INVALID");
  }

  //----------------------------------------------------------------------------
  TransformListInternal TrackedFrame::GetFrameTransformsInternal()
  {
    return m_frameTransforms;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::SetFrameTransformsInternal(const TransformListInternal& arg)
  {
    m_frameTransforms = arg;
  }

  //----------------------------------------------------------------------------
  TransformListABI^ TrackedFrame::Transforms::get()
  {
    Vector<Transform^>^ vec = ref new Vector<Transform^>();
    for (auto entry : m_frameTransforms)
    {
      vec->Append(entry);
    }
    return vec;
  }

  //----------------------------------------------------------------------------
  void TrackedFrame::Transforms::set(TransformListABI^ arg)
  {
    m_frameTransforms = Windows::Foundation::Collections::to_vector(arg);
  }

  //----------------------------------------------------------------------------
  VideoFrame^ TrackedFrame::Frame::get()
  {
    return m_frame;
  }

  //----------------------------------------------------------------------------
  FrameSizeABI^ TrackedFrame::Dimensions::get()
  {
    return m_frame->Dimensions;
  }
}