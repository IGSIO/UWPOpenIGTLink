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

// Local includes
#include "TransformName.h"

// STL includes
#include <list>
#include <map>
#include <mutex>

namespace UWPOpenIGTLink
{
  ref class TrackedFrame;

  public ref class TransformInfo sealed
  {
  public:
    TransformInfo();
    virtual ~TransformInfo();

    property Windows::Foundation::Numerics::float4x4 Matrix {Windows::Foundation::Numerics::float4x4 get(); void set(Windows::Foundation::Numerics::float4x4); }
    property bool Valid {bool get(); void set(bool); }
    property bool Computed {bool get(); void set(bool); }
    property bool Persistent {bool get(); void set(bool); }
    property Platform::String^ Date {Platform::String ^ get(); void set(Platform::String^); }
    property float Error {float get(); void set(float); }

  protected private:
    Windows::Foundation::Numerics::float4x4   m_matrix;
    bool                                      m_isValid;
    bool                                      m_isComputed;
    bool                                      m_isPersistent;
    Platform::String^                         m_date = nullptr;
    float                                     m_error;
  };

  /*!
  TransformRepository
  Combine multiple transforms to get a transform between arbitrary coordinate frames

  The TransformRepository stores a number of transforms between coordinate frames and
  it can multiply these transforms (or the inverse of these transforms) to
  compute the transform between any two coordinate frames.

  Example usage:
    transformRepository->SetTransform("Probe", "Tracker", mxProbeToTracker);
    transformRepository->SetTransform("Image", "Probe", mxImageToProbe);
    ...
    Windows::Numerics::float4x4 mxImageToTracker;
    TransformRepository::TransformStatus status = TransformRepository::TRANSFORM_INVALID;
    transformRepository->GetTransform("Image", "Tracker", mxImageToTracker, &status);

  The following coordinate frames are used commonly:
    Image: image frame coordinate system, origin is the bottom-left corner, unit is pixel
    Tool: coordinate system of the DRB attached to the probe, unit is mm
    Reference: coordinate system of the DRB attached to the reference body, unit is mm
    Tracker: coordinate system of the tracker, unit is mm
    World: world coordinate system, orientation is usually patient RAS, unit is mm
  */
  public ref class TransformRepository sealed
  {
  protected private:
    typedef std::map<std::wstring, TransformInfo^>                CoordFrameToTransformMapType;
    typedef std::map<std::wstring, CoordFrameToTransformMapType>  CoordFrameToCoordFrameToTransformMapType;
    typedef std::list<TransformInfo^>                             TransformInfoListType;

  public:
    TransformRepository();
    virtual ~TransformRepository();

    /*!
      Set a transform matrix between two coordinate frames. The method fails if the transform
      can be already constructed by concatenating/inverting already stored transforms. Changing an already
      set transform is allowed. The transform is computed even if one or more of the used transforms
      have non valid status.
      Throws exception if not valid
    */
    bool SetTransform(TransformName^ aTransformName, Windows::Foundation::Numerics::float4x4 matrix, bool isValid);

    /*!
      Set all transform matrices between two coordinate frames stored in TrackedFrame. The method fails if any of the transforms
      can be already constructed by concatenating/inverting already stored transforms. Changing an already
      set transform is allowed. The transform is computed even if one or more of the used transforms
      have non valid statuses.
    */
    [Windows::Foundation::Metadata::DefaultOverloadAttribute]
    bool SetTransforms(TrackedFrame^ trackedFrame);
    bool SetTransforms(TransformListABI^ transforms);

    /*!
      Set the valid status of a transform matrix between two coordinate frames. A transform is normally valid,
      but temporarily it can be set to non valid (e.g., when a tracked tool gets out of view).
    */
    bool SetTransformValid(TransformName^ aTransformName, bool isValid);

    /*!
      Set the persistent status of a transform matrix between two coordinate frames. A transform is non persistent by default.
      Transforms with status persistent will be written into config file on WriteConfiguration call.
    */
    void SetTransformPersistent(TransformName^ aTransformName, bool isPersistent);

    /*! Set the computation error of the transform matrix between two coordinate frames. */
    void SetTransformError(TransformName^ aTransformName, float aError);

    /*! Get the computation error of the transform matrix between two coordinate frames. */
    double GetTransformError(TransformName^ aTransformName);

    /*! Set the computation date of the transform matrix between two coordinate frames. */
    void SetTransformDate(TransformName^ aTransformName, Platform::String^ aDate);

    /*! Get the computation date of the transform matrix between two coordinate frames. */
    Platform::String^ GetTransformDate(TransformName^ aTransformName);

    /*!
      Read all transformations from XML data CoordinateDefinitions element and add them to the transforms with
      persistent and valid status. The method fails if any of the transforms
      can be already constructed by concatenating/inverting already stored transforms. Changing an already
      set transform is allowed.
    */
    bool ReadConfiguration(Windows::Data::Xml::Dom::XmlDocument^ doc);

    /*!
      Delete all transforms from XML data CoordinateDefinitions element then write all transform matrices with persistent status
      into the xml data CoordinateDefinitions element. The function will give a warning message in case of any non valid persistent transform.
    */
    bool WriteConfiguration(Windows::Data::Xml::Dom::XmlDocument^ doc);

    /*!
      Delete all transforms from XML data CoordinateDefinitions element then write all transform matrices that are persistent and non-persistent if boolean
      is true, only persistent if the boolean is false into the xml data CoordinateDefinitions element. The function will give a warning message
      in case of any non valid persistent transform.
    */
    bool WriteConfigurationGeneric(Windows::Data::Xml::Dom::XmlDocument^ doc, bool copyAllTransforms);

    /*!
      Get a transform matrix between two coordinate frames. The method fails if the transform
      cannot be already constructed by combining/inverting already stored transforms.
      \param aTransformName name of the transform to retrieve from the repository
      \param isValid whether or not the computed or original transform is valid
    */
    Windows::Foundation::Collections::IKeyValuePair<bool, Windows::Foundation::Numerics::float4x4>^ GetTransform(TransformName^ aTransformName);

    /*!
      Get the valid status of a transform matrix between two coordinate frames.
      The status is typically invalid when a tracked tool is out of view.
    */
    bool GetTransformValid(TransformName^ aTransformName);

    /// Get the persistent status of a transform matrix between two coordinate frames.
    bool GetTransformPersistent(TransformName^ aTransformName);

    /// Removes a transform from the repository
    void DeleteTransform(TransformName^ aTransformName);

    /// Removes all the transforms from the repository
    void Clear();

    /// Checks if a transform exist
    bool IsExistingTransform(TransformName^ aTransformName, bool aSilent);

    /// Copies the persistent and non-persistent contents if boolean is true, only persistent contents if false
    void DeepCopy(TransformRepository^ sourceRepositoryName, bool copyAllTransforms);

  protected private:
    /*! Get a user-defined original input transform (or its inverse). Does not combine user-defined input transforms. */
    TransformInfo^ GetOriginalTransform(TransformName^ aTransformName);

    /*!
    Find a transform path between the specified coordinate frames.
    \param aTransformName name of the transform to find
    \param transformInfoList Stores the list of transforms to get from the fromCoordFrameName to toCoordFrameName
    \param skipCoordFrameName This is the name of a coordinate system that should be ignored (e.g., because it was checked previously already)
    \param silent Don't log an error if path cannot be found (it's normal while searching in branches of the graph)
    \return returns PLUS_SUCCESS if a path can be found, PLUS_FAIL otherwise
    */
    bool FindPath(TransformName^ aTransformName, TransformInfoListType& transformInfoList, const wchar_t* skipCoordFrameName = NULL, bool silent = false);

    CoordFrameToCoordFrameToTransformMapType  m_CoordinateFrames;
    std::mutex                                m_CriticalSection;
    TransformInfo                             m_TransformToSelf;
  };
}