#pragma once

#ifdef _WIN32
  #ifdef LIBGRAPHICS_EXPORTS
    #define LIBGRAPHICS_API __declspec(dllexport)
  #else
    #define LIBGRAPHICS_API __declspec(dllimport)
  #endif
#else
  #define LIBGRAPHICS_API
#endif
