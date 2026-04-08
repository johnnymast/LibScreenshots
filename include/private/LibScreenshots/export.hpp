#pragma once

#ifdef _WIN32
    #ifdef LIBSCREENSHOTS_EXPORTS
        #define LIBSCREENSHOTS_API __declspec(dllexport)
    #else
        #define LIBSCREENSHOTS_API __declspec(dllimport)
    #endif
#else
    #define LIBSCREENSHOTS_API
#endif
