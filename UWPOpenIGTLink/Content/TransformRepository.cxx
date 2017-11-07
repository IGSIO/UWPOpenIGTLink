/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

// Local includes
#include "pch.h"
#include "TrackedFrame.h"
#include "Transform.h"
#include "TransformRepository.h"

// STL includes
#include <sstream>
#include <iomanip>

using namespace Platform::Collections::Details;
using namespace Windows::Data::Xml::Dom;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  TransformRepository::TransformInfo::TransformInfo()
    : m_Transform(float4x4::identity())
    , m_IsValid(true)
    , m_IsComputed(false)
    , m_IsPersistent(false)
    , m_Error(-1.0)
  {

  }

  //----------------------------------------------------------------------------
  TransformRepository::TransformInfo::~TransformInfo()
  {
  }

  //----------------------------------------------------------------------------
  TransformRepository::TransformInfo::TransformInfo(const TransformInfo& obj)
  {
    m_Transform = obj.m_Transform;
    m_IsComputed = obj.m_IsComputed;
    m_IsValid = obj.m_IsValid;
    m_IsPersistent = obj.m_IsPersistent;
    m_Date = obj.m_Date;
    m_Error = obj.m_Error;
  }
  //----------------------------------------------------------------------------
  TransformRepository::TransformInfo& TransformRepository::TransformInfo::operator=(const TransformInfo& obj)
  {
    m_Transform = obj.m_Transform;
    m_IsComputed = obj.m_IsComputed;
    m_IsValid = obj.m_IsValid;
    m_IsPersistent = obj.m_IsPersistent;
    m_Date = obj.m_Date;
    m_Error = obj.m_Error;
    return *this;
  }

  //----------------------------------------------------------------------------
  TransformRepository::TransformRepository()
  {
  }

  //----------------------------------------------------------------------------
  TransformRepository::~TransformRepository()
  {
  }

  //----------------------------------------------------------------------------
  TransformRepository::TransformInfo* TransformRepository::GetOriginalTransform(TransformName^ aTransformName)
  {
    std::wstring fromStr = std::wstring(aTransformName->From()->Data());
    std::wstring toStr = std::wstring(aTransformName->To()->Data());
    CoordFrameToTransformMapType& fromCoordFrame = this->m_CoordinateFrames[fromStr];

    // Check if the transform already exist
    CoordFrameToTransformMapType::iterator fromToTransformInfoIt = fromCoordFrame.find(toStr);
    if (fromToTransformInfoIt != fromCoordFrame.end())
    {
      // transform is found
      return &(fromToTransformInfoIt->second);
    }
    // transform is not found
    return NULL;
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::SetTransforms(TrackedFrame^ trackedFrame)
  {
    auto transforms = trackedFrame->GetFrameTransformsInternal();
    bool result(true);
    for (auto& entry : transforms)
    {
      result &= SetTransform(entry->Name, entry->Matrix, entry->Valid);
    }

    return result;
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::SetTransforms(TransformListABI^ transforms)
  {
    bool result(true);
    for (auto entry : transforms)
    {
      result &= SetTransform(entry->Name, entry->Matrix, entry->Valid);
    }

    return result;
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::SetTransform(TransformName^ aTransformName, float4x4 matrix, bool isValid)
  {
    if (!aTransformName->IsValid())
    {
      return false;
    }

    if (aTransformName->From() == aTransformName->To())
    {
      return false;
    }

    std::lock_guard<std::mutex> guard(m_CriticalSection);

    // Check if the transform already exist
    TransformInfo* fromToTransformInfo = GetOriginalTransform(aTransformName);
    if (fromToTransformInfo != NULL)
    {
      // Transform already exists
      if (fromToTransformInfo->m_IsComputed)
      {
        // The transform already exists and it is computed (not original), so reject the transformation update
        return false;
      }

      // Update the matrix
      fromToTransformInfo->m_Transform = matrix;

      // Set the status of the original transform
      fromToTransformInfo->m_IsValid = isValid;

      // Set the same status for the computed inverse transform
      TransformName^ toFromTransformName = ref new TransformName(aTransformName->To(), aTransformName->From());
      TransformInfo* toFromTransformInfo = GetOriginalTransform(toFromTransformName);
      if (toFromTransformInfo == NULL)
      {
        return false;
      }
      toFromTransformInfo->m_IsValid = isValid;
      return true;
    }
    // The transform does not exist yet, add it now

    TransformInfoListType transformInfoList;
    if (FindPath(aTransformName, transformInfoList, NULL, true /*silent*/))
    {
      // a path already exist between the two coordinate frames
      // adding a new transform between these would result in a circle
      return false;
    }

    // Create the from->to transform
    std::wstring fromStr(aTransformName->From()->Data());
    std::wstring toStr(aTransformName->To()->Data());
    CoordFrameToTransformMapType& fromCoordFrame = this->m_CoordinateFrames[fromStr];
    fromCoordFrame[toStr].m_IsComputed = false;
    fromCoordFrame[toStr].m_Transform = matrix;
    fromCoordFrame[toStr].m_IsValid = isValid;

    // Create the to->from inverse transform
    CoordFrameToTransformMapType& toCoordFrame = this->m_CoordinateFrames[toStr];
    toCoordFrame[fromStr].m_IsComputed = true;
    invert(matrix, &matrix);
    toCoordFrame[fromStr].m_Transform = matrix;
    toCoordFrame[fromStr].m_IsValid = isValid;

    return true;
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::SetTransformValid(TransformName^ aTransformName, bool isValid)
  {
    if (!aTransformName->IsValid())
    {
      return false;
    }

    if (aTransformName->From() == aTransformName->To())
    {
      return false;
    }

    std::lock_guard<std::mutex> guard(m_CriticalSection);

    // Check if the transform already exist
    TransformInfo* fromToTransformInfo = GetOriginalTransform(aTransformName);
    if (fromToTransformInfo != NULL)
    {
      fromToTransformInfo->m_IsValid = isValid;
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------------------
  IKeyValuePair<bool, float4x4>^ TransformRepository::GetTransform(TransformName^ aTransformName)
  {
    KeyValuePair<bool, float4x4>^ result = ref new KeyValuePair<bool, float4x4>(false, float4x4::identity());

    if (aTransformName == nullptr || !aTransformName->IsValid() || aTransformName->From() == aTransformName->To())
    {
      return result;
    }

    std::lock_guard<std::mutex> guard(m_CriticalSection);

    // Check if we can find the transform by combining the input transforms
    // To improve performance the already found paths could be stored in a map of transform name -> transformInfoList
    TransformInfoListType transformInfoList;
    if (!FindPath(aTransformName, transformInfoList))
    {
      // the transform cannot be computed, error has been already logged by FindPath
      return result;
    }

    // Create transform chain and compute transform status
    float4x4 combinedTransform = float4x4::identity();
    bool combinedTransformValid(true);
    for (auto& transformInfo : transformInfoList)
    {
      combinedTransform = combinedTransform * transformInfo->m_Transform; // even though this operator shows m_transform on the right, this is actually premultiplication
      if (!transformInfo->m_IsValid)
      {
        combinedTransformValid = false;
      }
    }

    result = ref new KeyValuePair<bool, float4x4>(combinedTransformValid, combinedTransform);
    return result;
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::GetTransformValid(TransformName^ aTransformName)
  {
    IKeyValuePair<bool, float4x4>^ result = GetTransform(aTransformName);
    return result->Key;
  }

  //----------------------------------------------------------------------------
  void TransformRepository::SetTransformPersistent(TransformName^ aTransformName, bool isPersistent)
  {
    std::lock_guard<std::mutex> guard(m_CriticalSection);

    if (aTransformName->From() == aTransformName->To())
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Setting a transform to itself is not allowed: " + aTransformName->GetTransformName());
    }

    TransformInfo* fromToTransformInfo = GetOriginalTransform(aTransformName);
    if (fromToTransformInfo != NULL)
    {
      fromToTransformInfo->m_IsPersistent = isPersistent;
      return;
    }

    throw ref new Platform::Exception(E_INVALIDARG, L"The original " + aTransformName->From() + L"To" + aTransformName->To() +
                                      L" transform is missing. Cannot set its persistent status");
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::GetTransformPersistent(TransformName^ aTransformName)
  {
    std::lock_guard<std::mutex> guard(m_CriticalSection);

    if (aTransformName->From() == aTransformName->To())
    {
      return false;
    }

    TransformInfo* fromToTransformInfo = GetOriginalTransform(aTransformName);
    if (fromToTransformInfo != NULL)
    {
      return fromToTransformInfo->m_IsPersistent;
    }

    throw ref new Platform::Exception(E_INVALIDARG, L"The original " + aTransformName->From() + L"To" + aTransformName->To() +
                                      L" transform is missing. Cannot set its persistent status");
  }

  //----------------------------------------------------------------------------
  void TransformRepository::SetTransformError(TransformName^ aTransformName, double aError)
  {
    std::lock_guard<std::mutex> guard(m_CriticalSection);

    if (aTransformName->From() == aTransformName->To())
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Setting a transform to itself is not allowed: " + aTransformName->GetTransformName());
    }

    TransformInfo* fromToTransformInfo = GetOriginalTransform(aTransformName);
    if (fromToTransformInfo != NULL)
    {
      fromToTransformInfo->m_Error = aError;
      return;
    }

    throw ref new Platform::Exception(E_INVALIDARG, L"The original " + aTransformName->From() + L"To" + aTransformName->To()
                                      + L" transform is missing. Cannot set computation error value.");
  }

  //----------------------------------------------------------------------------
  double TransformRepository::GetTransformError(TransformName^ aTransformName)
  {
    if (aTransformName->From() == aTransformName->To())
    {
      return 0.0;
    }

    std::lock_guard<std::mutex> guard(m_CriticalSection);

    TransformInfo* fromToTransformInfo = GetOriginalTransform(aTransformName);
    if (fromToTransformInfo != NULL)
    {
      return fromToTransformInfo->m_Error;
    }

    throw ref new Platform::Exception(E_INVALIDARG, L"The original " + aTransformName->From() + L"To" + aTransformName->To()
                                      + L" transform is missing. Cannot get computation error value.");
  }

  //----------------------------------------------------------------------------
  void TransformRepository::SetTransformDate(TransformName^ aTransformName, Platform::String^ aDate)
  {
    if (aTransformName->From() == aTransformName->To())
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Setting a transform to itself is not allowed: " + aTransformName->GetTransformName());
    }

    std::lock_guard<std::mutex> guard(m_CriticalSection);

    TransformInfo* fromToTransformInfo = GetOriginalTransform(aTransformName);
    if (fromToTransformInfo != NULL)
    {
      fromToTransformInfo->m_Date = std::wstring(aDate->Data());
      return;
    }

    throw ref new Platform::Exception(E_INVALIDARG, L"The original " + aTransformName->From() + L"To" + aTransformName->To()
                                      + L" transform is missing. Cannot set computation date.");
  }

  //----------------------------------------------------------------------------
  Platform::String^ TransformRepository::GetTransformDate(TransformName^ aTransformName)
  {
    if (aTransformName->From() == aTransformName->To())
    {
      return L"";
    }

    std::lock_guard<std::mutex> guard(m_CriticalSection);

    TransformInfo* fromToTransformInfo = GetOriginalTransform(aTransformName);
    if (fromToTransformInfo != NULL)
    {
      return ref new Platform::String(fromToTransformInfo->m_Date.c_str());
    }

    throw ref new Platform::Exception(E_INVALIDARG, L"The original " + aTransformName->From() + L"To" + aTransformName->To()
                                      + L" transform is missing. Cannot get computation date.");
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::FindPath(TransformName^ aTransformName, TransformInfoListType& transformInfoList, const wchar_t* skipCoordFrameName /*=NULL*/, bool silent /*=false*/)
  {
    if (aTransformName->FromInternal() == aTransformName->ToInternal())
    {
      return false;
    }

    TransformInfo* fromToTransformInfo = GetOriginalTransform(aTransformName);
    if (fromToTransformInfo != NULL)
    {
      // found a transform
      transformInfoList.push_back(fromToTransformInfo);
      return true;
    }
    // not found, so try to find a path through all the connected coordinate frames
    CoordFrameToTransformMapType& fromCoordFrame = this->m_CoordinateFrames[aTransformName->FromInternal()];
    for (CoordFrameToTransformMapType::iterator transformInfoIt = fromCoordFrame.begin(); transformInfoIt != fromCoordFrame.end(); ++transformInfoIt)
    {
      if (skipCoordFrameName != NULL && transformInfoIt->first.compare(std::wstring(skipCoordFrameName)) == 0)
      {
        // coordinate frame shall be ignored
        // (probably it would just go back to the previous coordinate frame where we come from)
        continue;
      }
      TransformName^ newTransformName = ref new TransformName(transformInfoIt->first, aTransformName->ToInternal());
      if (FindPath(newTransformName, transformInfoList, aTransformName->FromInternal().c_str(), true /*silent*/))
      {
        transformInfoList.push_back(&(transformInfoIt->second));
        return true;
      }
    }
    if (!silent)
    {
      // Print available transforms into a string, for troubleshooting information
      std::wostringstream osAvailableTransforms;
      bool firstPrintedTransform = true;
      for (auto& coordFrame : m_CoordinateFrames)
      {
        for (auto& transformInfo : coordFrame.second)
        {
          if (transformInfo.second.m_IsComputed)
          {
            // only print original transforms
            continue;
          }
          // don't print separator before the first transform
          if (firstPrintedTransform)
          {
            firstPrintedTransform = false;
          }
          else
          {
            osAvailableTransforms << ", ";
          }
          osAvailableTransforms << coordFrame.first << L"To" << transformInfo.first << L" ("
                                << (transformInfo.second.m_IsValid ? L"valid" : L"invalid") << ", "
                                << (transformInfo.second.m_IsPersistent ? L"persistent" : L"non-persistent") << ")";
        }
      }
      OutputDebugStringW((L"Transform path not found from "
                          + aTransformName->From()
                          + L" to "
                          + aTransformName->To()
                          + L" coordinate system. Available transforms in the repository (including the inverse of these transforms): "
                          + ref new Platform::String(osAvailableTransforms.str().c_str())
                          + L"\n")->Data());
    }

    return false;
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::IsExistingTransform(TransformName^ aTransformName, bool aSilent/* = true*/)
  {
    if (aTransformName->From() == aTransformName->To())
    {
      return true;
    }

    std::lock_guard<std::mutex> guard(m_CriticalSection);

    TransformInfoListType transformInfoList;
    return FindPath(aTransformName, transformInfoList, NULL, aSilent);
  }

  //----------------------------------------------------------------------------
  void TransformRepository::DeleteTransform(TransformName^ aTransformName)
  {
    if (aTransformName->From() == aTransformName->To())
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"Setting a transform to itself is not allowed: " + aTransformName->GetTransformName());
    }

    std::lock_guard<std::mutex> guard(m_CriticalSection);

    std::wstring fromStr(aTransformName->From()->Data());
    std::wstring toStr(aTransformName->To()->Data());

    CoordFrameToTransformMapType& fromCoordFrame = this->m_CoordinateFrames[fromStr];
    CoordFrameToTransformMapType::iterator fromToTransformInfoIt = fromCoordFrame.find(toStr);

    if (fromToTransformInfoIt != fromCoordFrame.end())
    {
      // from->to transform is found
      if (fromToTransformInfoIt->second.m_IsComputed)
      {
        // this is not an original transform (has not been set by the user)
        throw ref new Platform::Exception(E_FAIL, L"The " + aTransformName->From() + L" to " + aTransformName->To()
                                          + L" transform cannot be deleted, only the inverse of the transform has been set in the repository ("
                                          + aTransformName->From() + L" to " + aTransformName->To() + L")");
      }
      fromCoordFrame.erase(fromToTransformInfoIt);
    }
    else
    {
      throw ref new Platform::Exception(E_FAIL, L"Delete transform failed: could not find the " + aTransformName->From() + L" to " + aTransformName->To() + L" transform");
    }

    CoordFrameToTransformMapType& toCoordFrame = this->m_CoordinateFrames[toStr];
    CoordFrameToTransformMapType::iterator toFromTransformInfoIt = toCoordFrame.find(fromStr);
    if (toFromTransformInfoIt != toCoordFrame.end())
    {
      // to->from transform is found
      toCoordFrame.erase(toFromTransformInfoIt);
    }
    else
    {
      throw ref new Platform::Exception(E_FAIL, L"Delete transform failed: could not find the " + aTransformName->To() + L" to " + aTransformName->From() + L" transform");
    }
  }

  //----------------------------------------------------------------------------
  void TransformRepository::Clear()
  {
    this->m_CoordinateFrames.clear();
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::ReadConfiguration(XmlDocument^ doc)
  {
    auto xpath = ref new Platform::String(L"/HoloIntervention/CoordinateDefinitions");
    if (doc->SelectNodes(xpath)->Length != 1)
    {
      throw ref new Platform::Exception(E_INVALIDARG, L"TransformRepository::ReadConfiguration: no CoordinateDefinitions element was found");
    }

    IXmlNode^ coordinateDefinitions = doc->SelectNodes(xpath)->Item(0);

    // Clear the transforms
    Clear();

    int numberOfErrors(0);
    for (IXmlNode^ nestedElement : coordinateDefinitions->ChildNodes)
    {
      if (nestedElement->NodeName != L"Transform")
      {
        continue;
      }

      Platform::String^ fromAttribute = dynamic_cast<Platform::String^>(nestedElement->Attributes->GetNamedItem(L"From")->NodeValue);
      Platform::String^ toAttribute = dynamic_cast<Platform::String^>(nestedElement->Attributes->GetNamedItem(L"To")->NodeValue);

      if (fromAttribute->IsEmpty() || toAttribute->IsEmpty())
      {
        numberOfErrors++;
        continue;
      }

      TransformName^ transformName = ref new TransformName(fromAttribute, toAttribute);
      if (!transformName->IsValid())
      {
        numberOfErrors++;
        continue;
      }

      float4x4 matrix;
      if (nestedElement->Attributes->GetNamedItem(L"Matrix") == nullptr)
      {
        numberOfErrors++;
        continue;
      }
      Platform::String^ matrixString = dynamic_cast<Platform::String^>(nestedElement->Attributes->GetNamedItem(L"Matrix")->NodeValue);
      if (matrixString->IsEmpty())
      {
        numberOfErrors++;
        continue;
      }
      else
      {
        std::wstring matrixStr(matrixString->Data());
        std::wstringstream wss;
        wss << matrixStr;
        float mat[16];
        try
        {
          for (int i = 0; i < 16; ++i)
          {
            wss >> mat[i];
          }
          matrix.m11 = mat[0];
          matrix.m12 = mat[1];
          matrix.m13 = mat[2];
          matrix.m14 = mat[3];
          matrix.m21 = mat[4];
          matrix.m22 = mat[5];
          matrix.m23 = mat[6];
          matrix.m24 = mat[7];
          matrix.m31 = mat[8];
          matrix.m32 = mat[9];
          matrix.m33 = mat[10];
          matrix.m34 = mat[11];
          matrix.m41 = mat[12];
          matrix.m42 = mat[13];
          matrix.m43 = mat[14];
          matrix.m44 = mat[15];
        }
        catch (...)
        {
          numberOfErrors++;
        }

        try
        {
          SetTransform(transformName, matrix, true);
          SetTransformPersistent(transformName, true);

          if (nestedElement->Attributes->GetNamedItem(L"Error") != nullptr)
          {
            Platform::String^ errorAttribute = dynamic_cast<Platform::String^>(nestedElement->Attributes->GetNamedItem(L"Error")->NodeValue);
            if (!errorAttribute->IsEmpty())
            {
              SetTransformError(transformName, stod(std::wstring(errorAttribute->Data())));
            }
          }

          if (nestedElement->Attributes->GetNamedItem(L"Date") != nullptr)
          {
            Platform::String^ dateAttribute = dynamic_cast<Platform::String^>(nestedElement->Attributes->GetNamedItem(L"Date")->NodeValue);
            if (!dateAttribute->IsEmpty())
            {
              SetTransformDate(transformName, dateAttribute);
            }
          }
        }
        catch (...)
        {
          numberOfErrors++;
          continue;
        }
      }
    }

    if (numberOfErrors > 0)
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------------------
  // copyAllTransforms: include non-persistent and invalid transforms
  // Attributes: Persistent="TRUE/FALSE" Valid="TRUE/FALSE" => add it to ReadConfiguration, too
  bool TransformRepository::WriteConfigurationGeneric(XmlDocument^ doc, bool copyAllTransforms)
  {
    auto xpath = ref new Platform::String(L"/HoloIntervention/CoordinateDefinitions");
    XmlElement^ coordinateDefinitionsElement(nullptr);
    if (doc->SelectNodes(xpath)->Length == 0)
    {
      auto rootXpath = ref new Platform::String(L"/HoloIntervention");
      if (doc->SelectNodes(rootXpath)->Length == 0)
      {
        XmlElement^ rootElem = doc->CreateElement(L"HoloIntervention");
        doc->AppendChild(rootElem);
      }
      coordinateDefinitionsElement = doc->CreateElement(L"CoordinateDefinitions");
      doc->SelectNodes(rootXpath)->Item(0)->AppendChild(coordinateDefinitionsElement);
    }
    else
    {
      coordinateDefinitionsElement = dynamic_cast<XmlElement^>(doc->SelectNodes(xpath)->Item(0));
    }

    if (coordinateDefinitionsElement == nullptr)
    {
      return false;
    }

    for (auto& coordFrame : m_CoordinateFrames)
    {
      for (auto& transformInfo : coordFrame.second)
      {
        // if copyAllTransforms is true => copy non persistent and persistent. if false => copy only persistent
        if ((transformInfo.second.m_IsPersistent || copyAllTransforms) && !transformInfo.second.m_IsComputed)
        {
          const std::wstring& fromCoordinateFrame = coordFrame.first;
          const std::wstring& toCoordinateFrame = transformInfo.first;
          const float4x4& transform = transformInfo.second.m_Transform;
          const std::wstring& persistent = transformInfo.second.m_IsPersistent ? L"true" : L"false";
          const std::wstring& valid = transformInfo.second.m_IsValid ? L"true" : L"false";

          XmlElement^ newTransformElement = doc->CreateElement(L"Transform");
          newTransformElement->SetAttribute(L"From", ref new Platform::String(fromCoordinateFrame.c_str()));
          newTransformElement->SetAttribute(L"To", ref new Platform::String(toCoordinateFrame.c_str()));
          if (persistent.compare(L"false") == 0)
          {
            newTransformElement->SetAttribute("Persistent", L"false");
          }
          if (valid.compare(L"false") == 0)
          {
            newTransformElement->SetAttribute("Valid", L"false");
          }
          std::wostringstream woss;
          woss << std::fixed << transform.m11 << " " << transform.m12 << " " << transform.m13 << " " << transform.m14
               << " " << transform.m21 << " " << transform.m22 << " " << transform.m23 << " " << transform.m24
               << " " << transform.m31 << " " << transform.m32 << " " << transform.m33 << " " << transform.m34
               << " " << transform.m41 << " " << transform.m42 << " " << transform.m43 << " " << transform.m44;

          newTransformElement->SetAttribute("Matrix", ref new Platform::String(woss.str().c_str()));

          if (transformInfo.second.m_Error > 0)
          {
            newTransformElement->SetAttribute("Error", transformInfo.second.m_Error.ToString());
          }

          if (!transformInfo.second.m_Date.empty())
          {
            newTransformElement->SetAttribute("Date", ref new Platform::String(transformInfo.second.m_Date.c_str()));
          }
          else
          {
            // Add current date if it was not explicitly specified
            SYSTEMTIME st;
            GetLocalTime(&st);
            std::wstringstream woss;
            woss << std::setfill(L'0');
            woss << st.wYear
                 << L".";
            woss << std::setw(2);
            woss << st.wMonth
                 << L"." << st.wDay
                 << L" " << st.wHour
                 << L":" << st.wMinute;
            newTransformElement->SetAttribute("Date", ref new Platform::String(woss.str().c_str()));
          }

          coordinateDefinitionsElement->AppendChild(newTransformElement);
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool TransformRepository::WriteConfiguration(XmlDocument^ doc)
  {
    return this->WriteConfigurationGeneric(doc, false);
  }

  //----------------------------------------------------------------------------
  void TransformRepository::DeepCopy(TransformRepository^ sourceRepository, bool copyAllTransforms)
  {
    std::lock_guard<std::mutex> guard(m_CriticalSection);

    XmlDocument^ doc = ref new XmlDocument();
    sourceRepository->WriteConfigurationGeneric(doc, copyAllTransforms);
    ReadConfiguration(doc);
  }
}