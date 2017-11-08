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

#include "pch.h"
#include "StreamBufferItem.h"

using namespace Windows::Foundation::Numerics;

namespace UWPOpenIGTLink
{
  StreamBufferItem::StreamBufferItem()
    : m_filteredTimeStamp(0)
    , m_unfilteredTimeStamp(0)
    , m_index(0)
    , m_uid(0)
    , m_validTransformData(false)
    , m_matrix(float4x4::identity())
    , m_status(TOOL_OK)
  {
  }

  //----------------------------------------------------------------------------
  StreamBufferItem::~StreamBufferItem()
  {
  }

  //----------------------------------------------------------------------------
  void StreamBufferItem::SetCustomFrameField(Platform::String^ fieldName, Platform::String^ fieldValue)
  {
    m_customFrameFields[std::wstring(fieldName->Data())] = std::wstring(fieldValue->Data());
  }

  //----------------------------------------------------------------------------
  bool StreamBufferItem::DeepCopy(StreamBufferItem^ dataItem)
  {
    if (dataItem == nullptr)
    {
      OutputDebugStringA("Failed to deep copy data buffer item - buffer item NULL!");
      return false;
    }

    m_filteredTimeStamp = dataItem->m_filteredTimeStamp;
    m_unfilteredTimeStamp = dataItem->m_unfilteredTimeStamp;
    m_index = dataItem->m_index;
    m_uid = dataItem->m_uid;
    m_validTransformData = dataItem->m_validTransformData;
    m_matrix = dataItem->m_matrix;
    m_status = dataItem->m_status;

    m_frame->DeepCopy(dataItem->m_frame);

    return true;
  }

  //----------------------------------------------------------------------------
  bool StreamBufferItem::SetMatrix(float4x4 matrix)
  {
    m_validTransformData = true;

    m_matrix = matrix;

    return true;
  }

  //----------------------------------------------------------------------------
  float4x4 StreamBufferItem::GetMatrix()
  {
    return m_matrix;
  }

  //----------------------------------------------------------------------------
  void StreamBufferItem::SetStatus(int status)
  {
    m_status = (TOOL_STATUS)status;
  }

  //----------------------------------------------------------------------------
  int StreamBufferItem::GetStatus()
  {
    return m_status;
  }

  //----------------------------------------------------------------------------
  bool StreamBufferItem::HasValidFieldData()
  {
    return m_customFrameFields.size() > 0;
  }

  //----------------------------------------------------------------------------
  float StreamBufferItem::GetTimestamp(float localTimeOffsetSec)
  {
    return GetFilteredTimestamp(localTimeOffsetSec);
  }

  //----------------------------------------------------------------------------
  float StreamBufferItem::GetFilteredTimestamp(float localTimeOffsetSec)
  {
    return m_filteredTimeStamp + localTimeOffsetSec;
  }

  //----------------------------------------------------------------------------
  void StreamBufferItem::SetFilteredTimestamp(float filteredTimestamp)
  {
    m_filteredTimeStamp = filteredTimestamp;
  }

  //----------------------------------------------------------------------------
  float StreamBufferItem::GetUnfilteredTimestamp(float localTimeOffsetSec)
  {
    return m_unfilteredTimeStamp + localTimeOffsetSec;
  }

  //----------------------------------------------------------------------------
  void StreamBufferItem::SetUnfilteredTimestamp(float unfilteredTimestamp)
  {
    m_unfilteredTimeStamp = unfilteredTimestamp;
  }

  //----------------------------------------------------------------------------
  uint32 UWPOpenIGTLink::StreamBufferItem::GetIndex()
  {
    return m_index;
  }

  //----------------------------------------------------------------------------
  void StreamBufferItem::SetIndex(uint32 index)
  {
    m_index = index;
  }

  //----------------------------------------------------------------------------
  BufferItemUidType StreamBufferItem::GetUid()
  {
    return m_uid;
  }

  //----------------------------------------------------------------------------
  void StreamBufferItem::SetUid(BufferItemUidType uid)
  {
    m_uid = uid;
  }

  //----------------------------------------------------------------------------
  Platform::String^ StreamBufferItem::GetCustomFrameField(Platform::String^ fieldName)
  {
    if (fieldName == nullptr)
    {
      return nullptr;
    }

    FrameFields::iterator fieldIterator;
    fieldIterator = m_customFrameFields.find(std::wstring(fieldName->Data()));
    if (fieldIterator != m_customFrameFields.end())
    {
      return ref new Platform::String(fieldIterator->second.c_str());
    }
    return nullptr;
  }

  //----------------------------------------------------------------------------
  bool StreamBufferItem::DeleteCustomFrameField(Platform::String^ fieldName)
  {
    if (fieldName == nullptr)
    {
      return false;
    }

    FrameFields::iterator field = m_customFrameFields.find(std::wstring(fieldName->Data()));
    if (field != m_customFrameFields.end())
    {
      m_customFrameFields.erase(field);
      return true;
    }
    return false;
  }

  //----------------------------------------------------------------------------
  VideoFrame^ StreamBufferItem::GetFrame()
  {
    return m_frame;
  }

  //----------------------------------------------------------------------------
  void StreamBufferItem::SetValidTransformData(bool aValid)
  {
    m_validTransformData = aValid;
  }

  //----------------------------------------------------------------------------
  FrameFields StreamBufferItem::GetCustomFrameFields()
  {
    return m_customFrameFields;
  }

  //----------------------------------------------------------------------------
  void StreamBufferItem::SetCustomFrameFieldInternal(const std::wstring& fieldName, const std::wstring& fieldValue)
  {
    m_customFrameFields[fieldName] = fieldValue;
  }

  //----------------------------------------------------------------------------
  bool StreamBufferItem::HasValidTransformData()
  {
    return m_validTransformData;
  }

  //----------------------------------------------------------------------------
  bool StreamBufferItem::HasValidVideoData()
  {
    return m_frame->HasImage();
  }
}