#ifndef __ServerExport_h
#define __ServerExport_h

#if defined(WIN32) && !defined(Lib_STATIC)
 #if defined(Server_EXPORTS)
  #define ServerExport __declspec( dllexport )
 #else
  #define ServerExport __declspec( dllimport )
 #endif
#else
 #define ServerExport
#endif

#endif
