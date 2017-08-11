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

#include "pch.h"
#include "TimestampedCircularBuffer.h"

// STL includes
#include <stdexcept>

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  TimestampedCircularBuffer::TimestampedCircularBuffer()
    : m_writePointer(0)
    , m_currentTimeStamp(0.f)
    , m_localTimeOffsetSec(0.f)
    , m_latestItemUid(0)
    , m_averagedItemsForFiltering(20)
    , m_maxAllowedFilteringTimeDifference(0.5f)
    , m_startTime(0.f)
    , m_negligibleTimeDifferenceSec(0.00001f)
  {
    m_bufferItemContainer.resize(0);
    m_filterContainerIndexVector.resize(0);
    m_filterContainerTimestampVector.resize(0);
    m_filterContainersOldestIndex = 0;
    m_filterContainersNumberOfValidElements = 0;
  }

  //----------------------------------------------------------------------------
  TimestampedCircularBuffer::~TimestampedCircularBuffer()
  {
  }

  //----------------------------------------------------------------------------
  bool TimestampedCircularBuffer::PrepareForNewItem(float timestamp, BufferItemUidType* newFrameUid, BufferItemList::size_type* bufferIndex)
  {
    if(newFrameUid == nullptr || bufferIndex == nullptr)
    {
      return false;
    }

    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);

    if(timestamp <= m_currentTimeStamp)
    {
      OutputDebugStringA((std::string("Need to skip newly added frame - new timestamp (") + std::to_string(timestamp) + ") is not newer than the last one (" + std::to_string(m_currentTimeStamp) + ")!").c_str());
      return false;
    }

    // Increase frame unique ID
    *newFrameUid = ++m_latestItemUid;
    *bufferIndex = m_writePointer;
    m_currentTimeStamp = timestamp;

    m_numberOfItems++;
    if(m_numberOfItems > GetBufferSize())
    {
      m_numberOfItems = GetBufferSize();
    }

    // Increase the write pointer
    if(++m_writePointer >= GetBufferSize())
    {
      m_writePointer = 0;
    }

    return true;
  }

  //----------------------------------------------------------------------------
  // Sets the buffer size, and copies the maximum number of the most current old
  // frames and timestamps
  bool TimestampedCircularBuffer::SetBufferSize(BufferItemList::size_type newBufferSize)
  {
    if(newBufferSize < 0)
    {
      OutputDebugStringA("SetBufferSize: invalid buffer size");
      return false;
    }

    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    if(newBufferSize == GetBufferSize() && newBufferSize != 0)
    {
      return true;
    }

    if(GetBufferSize() == 0)
    {
      for(uint32 i = 0; i < newBufferSize; i++)
      {
        StreamBufferItem^ emptyBufferItem = ref new StreamBufferItem();
        m_bufferItemContainer.push_back(emptyBufferItem);
      }
      m_writePointer = 0;
      m_numberOfItems = 0;
      m_currentTimeStamp = 0.0;
    }
    // if the new buffer is bigger than the old buffer
    else if(GetBufferSize() < newBufferSize)
    {
      std::deque<StreamBufferItem^>::iterator it = m_bufferItemContainer.begin() + m_writePointer;
      const BufferItemList::size_type numberOfNewBufferObjects = newBufferSize - GetBufferSize();
      for(BufferItemList::size_type i = 0; i < numberOfNewBufferObjects; ++i)
      {
        StreamBufferItem^ emptyBufferItem = ref new StreamBufferItem();
        it = m_bufferItemContainer.insert(it, emptyBufferItem);
      }
    }
    // if the new buffer is smaller than the old buffer
    else if(GetBufferSize() > newBufferSize)
    {
      // delete the oldest buffer objects
      BufferItemList::size_type oldBufferSize = GetBufferSize();
      for(BufferItemList::size_type i = 0; i < oldBufferSize - newBufferSize; ++i)
      {
        std::deque<StreamBufferItem^>::iterator it = m_bufferItemContainer.begin() + m_writePointer;
        m_bufferItemContainer.erase(it);
        if(m_writePointer >= GetBufferSize())
        {
          m_writePointer = 0;
        }
      }
    }

    // update the number of items
    if(m_numberOfItems > GetBufferSize())
    {
      m_numberOfItems = GetBufferSize();
    }

    return true;
  }

  //----------------------------------------------------------------------------
  StreamBufferItem^ TimestampedCircularBuffer::GetBufferItemFromUid(BufferItemUidType uid)
  {
    // the caller must have locked the buffer
    BufferItemUidType oldestUid = m_latestItemUid - (m_numberOfItems - 1);
    if(uid < oldestUid)
    {
      throw UWPOpenIGTLink::ItemNotAvailableAnymoreException();
    }
    else if(uid > m_latestItemUid)
    {
      throw UWPOpenIGTLink::ItemNotAvailableYetException();
    }
    BufferItemList::size_type bufferIndex = (m_writePointer - 1) - (m_latestItemUid - uid);
    if(bufferIndex > ((m_latestItemUid - uid) - (m_writePointer - 1)))
    {
      // Underflow
      bufferIndex = m_bufferItemContainer.size() + (m_writePointer - 1) - (m_latestItemUid - uid);
    }
    return m_bufferItemContainer[bufferIndex];
  }

  //----------------------------------------------------------------------------
  StreamBufferItem^ TimestampedCircularBuffer::GetBufferItemFromBufferIndex(BufferItemList::size_type bufferIndex)
  {
    // the caller must have locked the buffer
    if(GetBufferSize() <= 0 || bufferIndex >= GetBufferSize() || bufferIndex < 0)
    {
      throw std::out_of_range("Invalid index.");
    }
    return m_bufferItemContainer[bufferIndex];
  }

  //----------------------------------------------------------------------------
  float TimestampedCircularBuffer::GetFilteredTimeStamp(BufferItemUidType uid)
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    StreamBufferItem^ itemPtr = GetBufferItemFromUid(uid);
    return itemPtr->GetFilteredTimestamp(m_localTimeOffsetSec);
  }

  //----------------------------------------------------------------------------
  float TimestampedCircularBuffer::GetUnfilteredTimeStamp(BufferItemUidType uid)
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    StreamBufferItem^ itemPtr = GetBufferItemFromUid(uid);
    return itemPtr->GetUnfilteredTimestamp(m_localTimeOffsetSec);
  }

  //----------------------------------------------------------------------------
  bool TimestampedCircularBuffer::GetLatestItemHasValidVideoData()
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    if(m_numberOfItems < 1)
    {
      return false;
    }
    BufferItemList::size_type latestItemBufferIndex = (m_writePointer > 0) ? (m_writePointer - 1) : (m_bufferItemContainer.size() - 1);
    return m_bufferItemContainer[latestItemBufferIndex]->HasValidVideoData();
  }

  //----------------------------------------------------------------------------
  bool TimestampedCircularBuffer::GetLatestItemHasValidTransformData()
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    if(m_numberOfItems < 1)
    {
      return false;
    }
    BufferItemList::size_type latestItemBufferIndex = (m_writePointer > 0) ? (m_writePointer - 1) : (m_bufferItemContainer.size() - 1);
    return m_bufferItemContainer[latestItemBufferIndex]->HasValidTransformData();
  }

  //----------------------------------------------------------------------------
  bool TimestampedCircularBuffer::GetLatestItemHasValidFieldData()
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    if(m_numberOfItems < 1)
    {
      return false;
    }
    BufferItemList::size_type latestItemBufferIndex = (m_writePointer > 0) ? (m_writePointer - 1) : (m_bufferItemContainer.size() - 1);
    return m_bufferItemContainer[latestItemBufferIndex]->HasValidFieldData();
  }

  //----------------------------------------------------------------------------
  BufferItemList::size_type TimestampedCircularBuffer::GetIndex(BufferItemUidType uid)
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    StreamBufferItem^ itemPtr = GetBufferItemFromUid(uid);
    if(itemPtr == nullptr)
    {
      return 0;
    }
    return itemPtr->GetIndex();
  }

  //----------------------------------------------------------------------------
  BufferItemList::size_type TimestampedCircularBuffer::GetBufferIndexFromTime(float time)
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);

    BufferItemUidType itemUid = GetItemUidFromTime(time);

    BufferItemList::size_type bufferIndex = (m_writePointer - 1) - (m_latestItemUid - itemUid);
    if(bufferIndex > ((m_latestItemUid - itemUid) - (m_writePointer - 1)))
    {
      // Underflow
      bufferIndex = m_bufferItemContainer.size() + (m_writePointer - 1) - (m_latestItemUid - itemUid);
    }
    return bufferIndex;
  }

  //----------------------------------------------------------------------------
  // do a simple divide-and-conquer search for the transform
  // that best matches the given timestamp
  BufferItemUidType TimestampedCircularBuffer::GetItemUidFromTime(float time)
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);

    if(m_numberOfItems == 1)
    {
      // There is only one item, it's the closest one to any timestamp
      return m_latestItemUid;
    }

    BufferItemUidType lo = m_latestItemUid - (m_numberOfItems - 1);   // oldest item UID
    BufferItemUidType hi = m_latestItemUid; // latest item UID

    // minimum time
    // This method is called often, therefore instead of calling GetTimeStamp(lo, tlo) we perform low-level operations to get the timestamp
    BufferItemList::size_type loBufferIndex = (m_writePointer - 1) - (m_latestItemUid - lo);
    if(loBufferIndex > ((m_latestItemUid - lo) - (m_writePointer - 1)))
    {
      loBufferIndex = m_bufferItemContainer.size() + (m_writePointer - 1) - (m_latestItemUid - lo);
    }
    float tlo = m_bufferItemContainer[loBufferIndex]->GetFilteredTimestamp(m_localTimeOffsetSec);

    // This method is called often, therefore instead of calling GetTimeStamp(hi, thi) we perform low-level operations to get the timestamp
    BufferItemList::size_type hiBufferIndex = (m_writePointer - 1) - (m_latestItemUid - hi);
    if(hiBufferIndex > ((m_latestItemUid - hi) - (m_writePointer - 1)))
    {
      hiBufferIndex = m_bufferItemContainer.size() + (m_writePointer - 1) - (m_latestItemUid - hi);
    }
    float thi = m_bufferItemContainer[hiBufferIndex]->GetFilteredTimestamp(m_localTimeOffsetSec);

    // If the timestamp is slightly out of range then still accept it
    // (due to errors in conversions there could be slight differences)
    if(time < tlo - m_negligibleTimeDifferenceSec)
    {
      throw UWPOpenIGTLink::ItemNotAvailableAnymoreException();
    }
    else if(time > thi + m_negligibleTimeDifferenceSec)
    {
      throw UWPOpenIGTLink::ItemNotAvailableYetException();
    }

    for(;;)
    {
      if(hi - lo <= 1)
      {
        if(time - tlo > thi - time)
        {
          return hi;
        }
        else
        {
          return lo;
        }
      }

      BufferItemList::size_type mid = (lo + hi) / 2;

      // This is a hot loop, therefore instead of calling GetTimeStamp(mid, tmid) we perform low-level operations to get the timestamp
      BufferItemList::size_type midBufferIndex = (m_writePointer - 1) - (m_latestItemUid - mid);
      if(midBufferIndex > ((m_latestItemUid - mid) - (m_writePointer - 1)))
      {
        midBufferIndex = m_bufferItemContainer.size() + (m_writePointer - 1) - (m_latestItemUid - mid);
      }
      float tmid = m_bufferItemContainer[midBufferIndex]->GetFilteredTimestamp(m_localTimeOffsetSec);

      if(time < tmid)
      {
        hi = mid;
        thi = tmid;
      }
      else
      {
        lo = mid;
        tlo = tmid;
      }
    }
    assert(false);
  }

  //----------------------------------------------------------------------------
  BufferItemUidType TimestampedCircularBuffer::GetLatestItemUidInBuffer()
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    return m_latestItemUid;
  }

  //----------------------------------------------------------------------------
  BufferItemUidType TimestampedCircularBuffer::GetOldestItemUidInBuffer()
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    // LatestItemUid - ( NumberOfItems - 1 ) is the oldest element in the buffer
    return m_latestItemUid - (m_numberOfItems - 1);
  }

  //----------------------------------------------------------------------------
  float TimestampedCircularBuffer::GetLatestTimeStamp()
  {
    return GetTimeStamp(GetLatestItemUidInBuffer());
  }

  //----------------------------------------------------------------------------
  float TimestampedCircularBuffer::GetOldestTimeStamp()
  {
    // The oldest item may be removed from the buffer at any moment
    // therefore we need to retrieve its UID and timestamp within a single lock
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    // LatestItemUid - ( NumberOfItems - 1 ) is the oldest element in the buffer
    BufferItemUidType oldestUid = (m_latestItemUid - (m_numberOfItems - 1));
    return GetTimeStamp(oldestUid);
  }

  //----------------------------------------------------------------------------
  void TimestampedCircularBuffer::DeepCopy(TimestampedCircularBuffer^ buffer)
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    m_writePointer = buffer->m_writePointer;
    m_numberOfItems = buffer->m_numberOfItems;
    m_currentTimeStamp = buffer->m_currentTimeStamp;
    m_localTimeOffsetSec = buffer->m_localTimeOffsetSec;
    m_latestItemUid = buffer->m_latestItemUid;
    m_startTime = buffer->m_startTime;
    m_averagedItemsForFiltering = buffer->m_averagedItemsForFiltering;
    m_filterContainersNumberOfValidElements = buffer->m_filterContainersNumberOfValidElements;
    m_filterContainersOldestIndex = buffer->m_filterContainersOldestIndex;
    m_filterContainerTimestampVector = buffer->m_filterContainerTimestampVector;
    m_filterContainerIndexVector = buffer->m_filterContainerIndexVector;

    m_bufferItemContainer = buffer->m_bufferItemContainer;
  }

  //----------------------------------------------------------------------------
  void TimestampedCircularBuffer::SetLocalTimeOffsetSec(float offset)
  {
    m_localTimeOffsetSec = offset;
  }

  //----------------------------------------------------------------------------
  float TimestampedCircularBuffer::GetLocalTimeOffsetSec()
  {
    return m_localTimeOffsetSec;
  }

  //----------------------------------------------------------------------------
  void TimestampedCircularBuffer::Clear()
  {
    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    m_writePointer = 0;
    m_numberOfItems = 0;
    m_currentTimeStamp = 0;
    m_latestItemUid = 0;
  }

  //----------------------------------------------------------------------------
  float TimestampedCircularBuffer::GetFrameRate(bool ideal, float* framePeriodStdevSec)
  {
    // TODO: Start the frame rate computation from the latest frame UID with using a few seconds of items in the buffer
    bool cannotComputeIdealFrameRateDueToInvalidFrameNumbers = false;

    std::vector<float> framePeriods;
    for(BufferItemUidType frame = GetLatestItemUidInBuffer(); frame > GetOldestItemUidInBuffer(); --frame)
    {
      float time(0);
      try
      {
        time = GetTimeStamp(frame);
      }
      catch(const std::exception&)
      {
        continue;
      }

      BufferItemList::size_type framenum(0);
      try
      {
        framenum = GetIndex(frame);
      }
      catch(const std::exception&)
      {
        continue;
      }

      float prevtime(0);
      try
      {
        prevtime = GetTimeStamp(frame - 1);
      }
      catch(const std::exception&)
      {
        continue;
      }

      BufferItemList::size_type prevframenum(0);
      try
      {
        prevframenum = GetIndex(frame - 1);
      }
      catch(const std::exception&)
      {
        continue;
      }

      float frameperiod = (time - prevtime);
      BufferItemList::size_type frameDiff = framenum - prevframenum;

      if(ideal)
      {
        if(frameDiff > 0)
        {
          frameperiod /= (1.f * frameDiff);
        }
        else
        {
          // the same frame number was set for different frame indexes; this should not happen (probably no frame number is available)
          cannotComputeIdealFrameRateDueToInvalidFrameNumbers = true;
        }
      }

      if(frameperiod > 0)
      {
        framePeriods.push_back(frameperiod);
      }
    }

    if(cannotComputeIdealFrameRateDueToInvalidFrameNumbers)
    {
      OutputDebugStringA("Cannot compute ideal frame rate accurately, as frame numbers are invalid or missing");
    }

    const BufferItemList::size_type numberOfFramePeriods = framePeriods.size();
    if(numberOfFramePeriods < 1)
    {
      OutputDebugStringA("Failed to compute frame rate. Not enough samples.");
      return 0;
    }

    float samplingPeriod(0);
    for(BufferItemList::size_type i = 0; i < numberOfFramePeriods; i++)
    {
      samplingPeriod += framePeriods[i];
    }
    samplingPeriod /= 1.f * numberOfFramePeriods;

    float frameRate(0);
    if(samplingPeriod != 0)
    {
      frameRate = 1.f / samplingPeriod;
    }

    if(framePeriodStdevSec != nullptr)
    {
      // Standard deviation of sampling period
      // stdev = sqrt ( 1/N * sum[ (xi-mean)^2 ] ) = sqrt ( 1/N * sumOfXiMeanDiffSquare )
      float sumOfXiMeanDiffSquare = 0;
      for(BufferItemList::size_type i = 0; i < numberOfFramePeriods; i++)
      {
        sumOfXiMeanDiffSquare += (framePeriods[i] - samplingPeriod) * (framePeriods[i] - samplingPeriod);
      }
      float framePeriodStdev = sqrt(sumOfXiMeanDiffSquare / numberOfFramePeriods);
      *framePeriodStdevSec = framePeriodStdev;
    }

    return frameRate;
  }

  //----------------------------------------------------------------------------
  // for accurate timing of the frame: an exponential moving average
  // is computed to smooth out the jitter in the times that are returned by the system clock:
  bool TimestampedCircularBuffer::CreateFilteredTimeStampForItem(uint32 itemIndex, float inUnfilteredTimestamp, float* outFilteredTimestamp, bool* filteredTimestampProbablyValid)
  {
    if(outFilteredTimestamp == nullptr || filteredTimestampProbablyValid == nullptr)
    {
      return false;
    }

    std::lock_guard<std::recursive_mutex> bufferGuardedLock(m_mutex);
    *filteredTimestampProbablyValid = true;

    if(m_filterContainerIndexVector.size() != m_averagedItemsForFiltering || m_filterContainerTimestampVector.size() != m_averagedItemsForFiltering)
    {
      // this call set elements to null
      m_filterContainerIndexVector.reserve(m_averagedItemsForFiltering);
      m_filterContainerTimestampVector.reserve(m_averagedItemsForFiltering);
      m_filterContainersOldestIndex = 0;
      m_filterContainersNumberOfValidElements = 0;
    }

    // We store the last AveragedItemsForFiltering unfiltered timestamp and item indexes, because these are used for computing the filtered timestamp.
    if(m_averagedItemsForFiltering > 1)
    {
      m_filterContainerIndexVector[m_filterContainersOldestIndex] = itemIndex;
      m_filterContainerTimestampVector[m_filterContainersOldestIndex] = inUnfilteredTimestamp;
      m_filterContainersNumberOfValidElements++;
      m_filterContainersOldestIndex++;

      if(m_filterContainersNumberOfValidElements > m_averagedItemsForFiltering)
      {
        m_filterContainersNumberOfValidElements = m_averagedItemsForFiltering;
      }

      if(m_filterContainersOldestIndex >= m_averagedItemsForFiltering)
      {
        m_filterContainersOldestIndex = 0;
      }
    }

    // If we don't have enough unfiltered timestamps or we don't want to use filtering then just use the unfiltered timestamps
    if(m_averagedItemsForFiltering < 2 || m_filterContainersNumberOfValidElements < m_averagedItemsForFiltering)
    {
      *outFilteredTimestamp = inUnfilteredTimestamp;
      return true;
    }

    // The items are acquired periodically, with quite accurate frame periods. The data is not timestamped
    // by the source, only Plus attaches a timestamp when it receives the data. The timestamp that Plus attaches
    // (the unfiltered timestamp) may be inaccurate, due to random delays in transferring the data.
    // Without the random delays (and if the acquisition frame rate is constant) the itemIndex vs. timestamp function would be a straight line.
    // With the random delays small spikes appear on this line, causing inaccuracies.
    // Get rid of the small spikes and get a smooth straight line by fitting a line (timestamp = itemIndex * framePeriod + timeOffset) to the
    // itemIndex vs. unfiltered timestamp function and compute the current filtered timestamp
    // by extrapolation of this line to the current item index.
    // The line parameters computed by linear regression.
    //
    // timestamp = framePeriod * itemIndex+ timeOffset
    //   x = itemIndex
    //   y = timestamp
    //   a = framePeriod
    //   b = timeOffset
    //
    // Ordinary least squares estimation:
    //   y(i) = a * x(i) + b;
    //   a = sum( (x(i)-xMean) * (y(i)-yMean) ) / sum( (x(i)-xMean) * (x(i)-xMean) )
    //   b = yMean - a*xMean
    //

    float xMean = 1.f * VectorMean<uint32>(m_filterContainerIndexVector, 0);
    float yMean = VectorMean<float>(m_filterContainerTimestampVector, 0.f);
    float covarianceXY = 0;
    float varianceX = 0;
    for(BufferItemList::size_type i = m_filterContainerTimestampVector.size() - 1; i >= 0; i--)
    {
      float xiMinusXmean = m_filterContainerIndexVector[i] - xMean;
      covarianceXY += xiMinusXmean * (m_filterContainerTimestampVector[i] - yMean);
      varianceX += xiMinusXmean * xiMinusXmean;
    }
    float a = covarianceXY / varianceX;
    float b = yMean - a * xMean;

    *outFilteredTimestamp = a * itemIndex + b;

    if(fabs(*outFilteredTimestamp - inUnfilteredTimestamp) > m_maxAllowedFilteringTimeDifference)
    {
      // Write current timestamps and frame indexes to the log to allow investigation of the problem
      filteredTimestampProbablyValid = false;
      std::stringstream ss;
      ss << "Difference between unfiltered timestamp is larger than the threshold. The unfiltered timestamp may be incorrect."
         << " Unfiltered timestamp: " << inUnfilteredTimestamp << ", filtered timestamp: " << *outFilteredTimestamp << ", difference: " << fabs(*outFilteredTimestamp - inUnfilteredTimestamp) << ", threshold: " << m_maxAllowedFilteringTimeDifference << "."
         << " timestamps = [" << std::fixed << m_filterContainerTimestampVector << "];"
         << " frame indexes = [" << std::fixed << m_filterContainerIndexVector << "];";
      OutputDebugStringA(ss.str().c_str());
    }

    return true;
  }

  //----------------------------------------------------------------------------
  BufferItemList::size_type TimestampedCircularBuffer::GetBufferSize()
  {
    return m_bufferItemContainer.size();
  }

  //----------------------------------------------------------------------------
  BufferItemList::size_type TimestampedCircularBuffer::GetNumberOfItems()
  {
    return m_numberOfItems;
  }

  //----------------------------------------------------------------------------
  float TimestampedCircularBuffer::GetTimeStamp(BufferItemUidType uid)
  {
    return GetFilteredTimeStamp(uid);
  }

  //----------------------------------------------------------------------------
  void TimestampedCircularBuffer::SetAveragedItemsForFiltering(uint32 items)
  {
    m_averagedItemsForFiltering = items;
  }

  //----------------------------------------------------------------------------
  uint32 TimestampedCircularBuffer::GetAveragedItemsForFiltering()
  {
    return m_averagedItemsForFiltering;
  }

  //----------------------------------------------------------------------------
  void TimestampedCircularBuffer::SetStartTime(float startTime)
  {
    m_startTime = startTime;
  }

  //----------------------------------------------------------------------------
  float TimestampedCircularBuffer::GetStartTime()
  {
    return m_startTime;
  }
}