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

// std includes
#include <string>

namespace UWPOpenIGTLink
{
  /*
  The TransformName stores and generates the from and to coordinate frame names for transforms.
  To enable robust serialization to/from a simple string (...To...Transform), the coordinate frame names must
  start with an uppercase character and it shall not contain "To" followed by an uppercase character. E.g., valid
  coordinate frame names are Tracker, TrackerBase, Tool; invalid names are tracker, trackerBase, ToImage.

  Setting a transform name:
    TransformName tnImageToProbe("Image", "Probe");
  or
    TransformName tnImageToProbe;
    if ( tnImageToProbe->SetTransformName("ImageToProbe") != PLUS_SUCCESS )
    {
      LOG_ERROR("Failed to set transform name!");
      return PLUS_FAIL;
    }

  Getting coordinate frame or transform names:
    std::string fromFrame = tnImageToProbe->From();
    std::string toFrame = tnImageToProbe->To();
  or
    std::string strImageToProbe;
    if ( tnImageToProbe->GetTransformName(strImageToProbe) != PLUS_SUCCESS )
    {
      LOG_ERROR("Failed to get transform name!");
      return PLUS_FAIL;
    }
  */

  [Windows::Foundation::Metadata::WebHostHiddenAttribute]
  public ref class TransformName sealed
  {
  public:
    TransformName();
    virtual ~TransformName();
    TransformName(Platform::String^ aFrom, Platform::String^ aTo);
    TransformName(Platform::String^ transformName);

    /*!
    Set 'From' and 'To' coordinate frame names from a combined transform name with the following format [FrameFrom]To[FrameTo].
    The combined transform name might contain only one 'To' phrase followed by a capital letter (e.g. ImageToToProbe is not allowed)
    and the coordinate frame names should be in camel case format starting with capitalized letters.
    */
    void SetTransformName(Platform::String^ aTransformName);

    /// Return combined transform name between 'From' and 'To' coordinate frames: [From]To[To]
    Platform::String^ GetTransformName();

    /// Return 'From' coordinate frame name, give a warning if it's not capitalized and capitalize it
    Platform::String^ From();

    /// Return 'To' coordinate frame name, give a warning if it's not capitalized and capitalize it
    Platform::String^ To();

    /// Clear the 'From' and 'To' fields
    void Clear();

    // Check if the current transform name is valid
    bool IsValid();

  protected private:
    /// Check if the input string is capitalized, if not capitalize it
    void Capitalize(std::wstring& aString);

  protected private:
    /// From coordinate frame name
    std::wstring m_From;
    /// To coordinate frame name
    std::wstring m_To;
  };
}