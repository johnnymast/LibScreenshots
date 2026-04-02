#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #ifdef LIBSCREENSHOTS_EXPORT
    #define LIBSCREENSHOTS_EXPORT __declspec(dllexport)
  #else
    #define LIBSCREENSHOTS_EXPORT __declspec(dllimport)
  #endif
#else
  #ifdef LIBSCREENSHOTS_EXPORT
    #define LIBSCREENSHOTS_EXPORT __attribute__((visibility("default")))
  #else
    #define LIBSCREENSHOTS_EXPORT
  #endif
#endif
