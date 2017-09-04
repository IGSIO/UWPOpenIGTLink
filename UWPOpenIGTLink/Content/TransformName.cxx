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
#include "TransformName.h"

// std includes
#include <sstream>

namespace UWPOpenIGTLink
{
  //-------------------------------------------------------
  TransformName::TransformName()
  {
  }

  //----------------------------------------------------------------------------
  bool TransformName::operator==(const TransformName^ other)
  {
    if (other == nullptr)
    {
      return false;
    }
    return m_From == other->m_From && m_To == other->m_To;
  }

  //-------------------------------------------------------
  TransformName::~TransformName()
  {
  }

  //-------------------------------------------------------
  TransformName::TransformName(Platform::String^ aFrom, Platform::String^ aTo)
  {
    m_From = std::wstring(aFrom->Data());
    Capitalize(m_From);

    m_To = std::wstring(aTo->Data());
    Capitalize(m_To);
  }

  //-------------------------------------------------------
  TransformName::TransformName(Platform::String^ transformName)
  {
    SetTransformName(transformName);
  }

  //-------------------------------------------------------
  TransformName::TransformName(const std::wstring& aFrom, const std::wstring& aTo)
  {
    m_From = aFrom;
    Capitalize(m_From);

    m_To = aTo;
    Capitalize(m_To);
  }

  //-------------------------------------------------------
  TransformName::TransformName(const std::wstring& transformName)
  {
    SetTransformName(transformName);
  }

  //-------------------------------------------------------
  bool TransformName::IsValid()
  {
    if (m_From.empty())
    {
      return false;
    }

    if (m_To.empty())
    {
      return false;
    }

    return true;
  }

  //-------------------------------------------------------
  void TransformName::SetTransformName(Platform::String^ aTransformName)
  {
    m_From.clear();
    m_To.clear();

    size_t posTo = std::wstring::npos;

    // Check if the string has only one valid 'To' phrase
    int numOfMatch = 0;
    std::wstring transformNameStr = std::wstring(aTransformName->Data());
    std::wstring subString = transformNameStr;
    size_t posToTested = std::wstring::npos;
    size_t numberOfRemovedChars = 0;
    while (((posToTested = subString.find(L"To")) != std::wstring::npos) && (subString.length() > posToTested + 2))
    {
      if (toupper(subString[posToTested + 2]) == subString[posToTested + 2])
      {
        // there is a "To", and after that the next letter is uppercase, so it's really a match (e.g., the first To in TestToolToTracker would not be a real match)
        numOfMatch++;
        posTo = numberOfRemovedChars + posToTested;
      }
      // search in the rest of the string
      subString = subString.substr(posToTested + 2);
      numberOfRemovedChars += posToTested + 2;
    }

    if (numOfMatch != 1)
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Unable to parse transform name, there are " + numOfMatch + L" matching 'To' phrases in the transform name '" + aTransformName + L"', while exactly one allowed.");
    }

    // Find <FrameFrom>To<FrameTo> matches
    if (posTo == std::wstring::npos)
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Failed to set transform name - unable to find 'To' in '" + aTransformName + L"'!");
    }
    else if (posTo == 0)
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Failed to set transform name - no coordinate frame name before 'To' in '" + aTransformName + L"'!");
    }
    else if (posTo == transformNameStr.length() - 2)
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Failed to set transform name - no coordinate frame name after 'To' in '" + aTransformName + L"'!");
    }

    // Set From coordinate frame name
    m_From = transformNameStr.substr(0, posTo);

    // Allow handling of To coordinate frame containing "Transform"
    std::wstring postFrom(transformNameStr.substr(posTo + 2));
    if (postFrom.find(L"Transform") != std::wstring::npos)
    {
      postFrom = postFrom.substr(0, postFrom.find(L"Transform"));
    }

    m_To = postFrom;
    Capitalize(m_From);
    Capitalize(m_To);
  }

  //----------------------------------------------------------------------------
  void TransformName::SetTransformName(const std::wstring& aTransformName)
  {
    SetTransformName(ref new Platform::String(aTransformName.c_str()));
  }

  //----------------------------------------------------------------------------
  std::wstring TransformName::GetTransformNameInternal()
  {
    return m_From + L"To" + m_To;
  }

  //----------------------------------------------------------------------------
  std::wstring TransformName::ToInternal() const
  {
    return m_To;
  }

  //----------------------------------------------------------------------------
  std::wstring TransformName::FromInternal() const
  {
    return m_From;
  }

  //-------------------------------------------------------
  Platform::String^ TransformName::GetTransformName()
  {
    return ref new Platform::String((m_From + std::wstring(L"To") + m_To).c_str());
  }

  //-------------------------------------------------------
  Platform::String^ TransformName::From()
  {
    return ref new Platform::String(m_From.c_str());
  }

  //-------------------------------------------------------
  Platform::String^ TransformName::To()
  {
    return ref new Platform::String(m_To.c_str());
  }

  //-------------------------------------------------------
  void TransformName::Capitalize(std::wstring& aString)
  {
    // Change first character to uppercase
    if (aString.length() < 1)
    {
      return;
    }
    aString[0] = toupper(aString[0]);
  }

  //-------------------------------------------------------
  void TransformName::Clear()
  {
    m_From = L"";
    m_To = L"";
  }
}