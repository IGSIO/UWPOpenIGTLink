/*=========================================================================

  Program:   OpenIGTLink Library
  Module:    $HeadURL: http://svn.na-mic.org/NAMICSandBox/trunk/OpenIGTLink/igtlConfigure.h.in $
  Language:  C
  Date:      $Date: 2010-06-09 16:16:36 -0400 (Wed, 09 Jun 2010) $
  Version:   $Revision: 6525 $

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __IGTL_CONFIGURE_H
#define __IGTL_CONFIGURE_H

/* #undef OpenIGTLink_PLATFORM_MACOSX */
/* #undef OpenIGTLink_PLATFORM_LINUX */
/* #undef OpenIGTLink_PLATFORM_SUNOS */
#define OpenIGTLink_PLATFORM_WIN32
#ifdef OpenIGTLink_PLATFORM_WIN32
  #ifndef _WIN32
    #define _WIN32
  #endif
  #ifndef WIN32
    #define WIN32
  #endif
  #define IGTLCommon_EXPORTS
#endif

/* #undef OpenIGTLink_USE_PTHREADS */
#define OpenIGTLink_USE_WIN32_THREADS
/* #undef OpenIGTLink_USE_SPROC */
/* #undef OpenIGTLink_HAVE_GETSOCKNAME_WITH_SOCKLEN_T */
#define OpenIGTLink_HAVE_STRNLEN

#define OpenIGTLink_PROTOCOL_VERSION_1 1
#define OpenIGTLink_PROTOCOL_VERSION_2 2
#define OpenIGTLink_PROTOCOL_VERSION_3 3
#define OpenIGTLink_PROTOCOL_VERSION 3
#define OpenIGTLink_HEADER_VERSION 2

#endif /*__IGTL_CONFIGURE_H*/



