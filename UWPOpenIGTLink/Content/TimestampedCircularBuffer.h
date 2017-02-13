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

#include "StreamBufferItem.h"

// STL includes
#include <mutex>
#include <deque>

namespace UWPOpenIGTLink
{
  enum ItemStatus
  {
    ITEM_OK,
    ITEM_NOT_AVAILABLE_YET,
    ITEM_NOT_AVAILABLE_ANYMORE,
    ITEM_UNKNOWN_ERROR
  };

  public ref class TimestampedCircularBuffer sealed
  {
  public:
    TimestampedCircularBuffer();
    virtual ~TimestampedCircularBuffer();

    bool SetBufferSize(uint32 n);
    uint32 GetBufferSize();

    /*!
      Get the number of items in the list (this is not the same as
      the buffer size, but is rather the number of transforms that
      have been added to the list).  This will never be greater than
      the BufferSize.
    */
    uint32 GetNumberOfItems();

    /*!
      Given a timestamp, compute the nearest frame UID
      This assumes that the times moronically increase
    */
    BufferItemUidType GetItemUidFromTime(double time);

    /*! Get the most recent frame UID that is already in the buffer */
    BufferItemUidType GetLatestItemUidInBuffer();

    /*! Get the oldest frame UID in the buffer  */
    BufferItemUidType GetOldestItemUidInBuffer();

    double GetLatestTimeStamp();
    double GetOldestTimeStamp();

    double GetTimeStamp(BufferItemUidType uid);
    double GetFilteredTimeStamp(BufferItemUidType uid);
    double GetUnfilteredTimeStamp(BufferItemUidType uid);

    bool GetLatestItemHasValidVideoData();
    bool GetLatestItemHasValidTransformData();
    bool GetLatestItemHasValidFieldData();


    /*! Get the index assigned by the data reacquisition system (usually a counter) from the buffer by frame UID. */
    uint32 GetIndex(BufferItemUidType uid);
    /*!
      Given a timestamp, compute the nearest buffer index
      This assumes that the times moronically increase
    */
    uint32 GetBufferIndexFromTime(double time);

    /*!
      Make this buffer into a copy of another buffer.  You should
      Lock both of the buffers before doing this.
    */
    void DeepCopy(TimestampedCircularBuffer^ buffer);

    /*!  Set the local time offset in seconds (global = local + offset) */
    void SetLocalTimeOffsetSec(double offset);
    /*!  Get the local time offset in seconds (global = local + offset) */
    double GetLocalTimeOffsetSec();

    /*!
      Get the frame rate from the buffer based on the number of frames in the buffer
      and the elapsed time.
      Ideal frame rate shows the mean of the frame periods in the buffer based on the frame
      number difference (a.k.a. the device frame rate, a.k.a. the frame rate that would have been achieved
      if frames were not dropped).
      If framePeriodStdevSecPtr is not null, then the standard deviation of the frame period is computed as well (in seconds) and
      stored at the specified address.
    */
    double GetFrameRate(bool ideal, double* framePeriodStdevSecPtr);

    /*! Clear buffer (set the buffer pointer to the first element) */
    void Clear();

    /*!
      Get next writable buffer object
      INTERNAL USE ONLY! Need to lock buffer until we use the buffer index
    */
    StreamBufferItem^ GetBufferItemFromBufferIndex(uint32 bufferIndex);

    /*!
      Get next writable buffer object
      INTERNAL USE ONLY! Need to lock buffer until we use the buffer index
    */
    StreamBufferItem^ GetBufferItemFromUid(BufferItemUidType uid);

    bool PrepareForNewItem(double timestamp, BufferItemUidType* newFrameUid, int* bufferIndex);

    /*!
      Create filtered and unfiltered timestamp for accurate timing of the buffer item.
      The timing may be inaccurate because the timestamp is attached to the item when Plus receives it
      and so the timestamp is affected by data transfer speed (which may slightly vary).
      A line is fitted to the index and timestamp of the last (AveragedItemsForFiltering) items.
      The filtered timestamp is the time value that corresponds to the frame index according to the fitted line.
      If the filtered timestamp is very different from the non-filtered timestamp then
      filteredTimestampProbablyValid will be false and it is recommended not to use that item,
      because its timestamp is probably incorrect.
    */
    bool CreateFilteredTimeStampForItem(uint32 itemIndex, double inUnfilteredTimestamp, double* outFilteredTimestamp, bool* filteredTimestampProbablyValid);

    /*! Set number of items used for timestamp filtering (with LSQR minimizer) */
    void SetAveragedItemsForFiltering(uint32 items);
    /*! Get number of items used for timestamp filtering (with LSQR minimizer) */
    uint32 GetAveragedItemsForFiltering();

    /*! Set recording start time */
    void SetStartTime(double startTime);
    /*! Get recording start time */
    double GetStartTime();

  protected private:
    std::recursive_mutex          m_mutex;
    uint32                        m_numberOfItems;
    uint32                        m_writePointer;
    double                        m_currentTimeStamp;
    double                        m_localTimeOffsetSec;
    BufferItemUidType             m_latestItemUid;
    std::deque<StreamBufferItem^> m_bufferItemContainer;
    std::vector<double>           m_filterContainerIndexVector;
    std::vector<double>           m_filterContainerTimestampVector;
    uint32                        m_filterContainersOldestIndex;
    uint32                        m_filterContainersNumberOfValidElements;
    uint32                        m_averagedItemsForFiltering;
    double                        m_maxAllowedFilteringTimeDifference;
    double                        m_startTime;
    double                        m_negligibleTimeDifferenceSec;
  };
}