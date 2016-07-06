/*=========================================================================

  Program:   The OpenIGTLink Library
  Language:  C++
  Web page:  http://openigtlink.org/

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


/*=========================================================================

  Part of the code is copied from itk::RealTimeClock class in
  Insight Segmentation & Registration Toolkit:


  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRealTimeClock.cxx,v $
  Language:  C++
  Date:      $Date: 2011-03-24 00:08:23 -0400 (Thu, 24 Mar 2011) $
  Version:   $Revision: 7354 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "igtlTimeStamp.h"

#include <iostream>
#include <math.h>

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif  // defined(WIN32) || defined(_WIN32)

#include <string.h>

#include "igtl_util.h"

namespace igtl
{

TimeStamp::TimeStamp()
  : Object()
#if defined(WIN32) || defined(_WIN32)
  , PCFreq(0.0)
  , CounterStart(0)
#endif
{
#if defined(WIN32) || defined(_WIN32)
  LARGE_INTEGER li;
  if (!QueryPerformanceFrequency(&li))
    std::cout << "QueryPerformanceFrequency failed!\n";

  this->PCFreq = double(li.QuadPart) / 1000000.0;

  QueryPerformanceCounter(&li);
  CounterStart = li.QuadPart;
#else
  this->m_Frequency = 1000000;
#endif  // defined(WIN32) || defined(_WIN32)
}


TimeStamp::~TimeStamp()
{
}


void TimeStamp::GetTime()
{
#if defined(WIN32) || defined(_WIN32)

  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  double value = double(li.QuadPart - CounterStart) / PCFreq;
  this->m_Nanosecond = value * 1000;
  this->m_Second = value / 1000000;
#else

  struct timeval tval;

  ::gettimeofday( &tval, 0 );

  this->m_Second     = tval.tv_sec;
  this->m_Nanosecond = tval.tv_usec * 1000; /* convert from micro to nano */

#endif  // defined(WIN32) || defined(_WIN32)

}

void TimeStamp::SetTime( double tm )
{
  double second = floor( tm );
  this->m_Second = static_cast<igtlInt32>( second );
  this->m_Nanosecond = static_cast<igtlInt32>( ( tm - second ) * 1e9 );
}

void TimeStamp::SetTime( igtlUint32 second, igtlUint32 nanosecond )
{
  if ( nanosecond < 1e9 )
  {
    this->m_Second = second;
    this->m_Nanosecond = nanosecond;
  }
}


void TimeStamp::SetTime( igtlUint64 tm )
{
  // Export from 64-bit fixed-point expression used in OpenIGTLink
  igtlInt32 sec      = static_cast<igtlInt32>( ( tm >> 32 ) & 0xFFFFFFFF );
  igtlInt32 fraction = static_cast<igtlInt32>( tm & 0xFFFFFFFF );
  this->m_Second     = sec;
  this->m_Nanosecond = igtl_frac_to_nanosec( static_cast<igtlUint32>( fraction ) );
}


//-----------------------------------------------------------------------------
void TimeStamp::SetTimeInNanoseconds( igtlUint64 tm )
{
  igtlUint64 sec = tm / 1e9; // integer rounding
  igtlUint64 nano = sec * 1e9; // round it back up to get whole number of seconds expressed in nanoseconds.
  this->m_Second = static_cast<igtlInt32>( sec );
  this->m_Nanosecond = static_cast<igtlInt32>( tm - nano );
}


double TimeStamp::GetTimeStamp()
{
  double tm;
  tm = static_cast<double>( this->m_Second ) +
       static_cast<double>( this->m_Nanosecond ) / 1e9;

  return tm;
}

void TimeStamp::GetTimeStamp( igtlUint32* second, igtlUint32* nanosecond )
{
  *second     = this->m_Second;
  *nanosecond = this->m_Nanosecond;
}


igtlUint64 TimeStamp::GetTimeStampUint64()
{
  // Export as 64-bit fixed-point expression used in OpenIGTLink
  igtlInt32 sec      = this->m_Second;
  igtlInt32 fraction = igtl_nanosec_to_frac( this->m_Nanosecond );

  igtlUint64 ts  =  sec & 0xFFFFFFFF;
  ts = ( ts << 32 ) | ( fraction & 0xFFFFFFFF );

  return ts;
}


//-----------------------------------------------------------------------------
igtlUint64 TimeStamp::GetTimeStampInNanoseconds() const
{
  igtlUint64 tmp = this->m_Second * 1e9;
  tmp += this->m_Nanosecond;
  return tmp;
}

void TimeStamp::PrintSelf( std::ostream& os ) const
{
  Superclass::PrintSelf( os );

  std::string indent = "    ";

  os << indent << "Frequency of the clock: "
     << this->m_Frequency << std::endl;
  os << indent << "Second : "
     << this->m_Second << std::endl;
  os << indent << "Nanosecond : "
     << this->m_Nanosecond << std::endl;
}

}


