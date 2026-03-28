#pragma once

#ifdef HAVE_WINDOWS

  #ifdef LIBSCREENSHOTS_EXPORT
    #define LIBSCREENSHOTS_EXPORT __declspec(dllexport)
  #else
    #define LIBSCREENSHOTS_EXPORT __declspec(dllimport)
  #endif

#else
  #define LIBSCREENSHOTS_EXPORT
#endif
