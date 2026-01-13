#ifndef RHI_DEF_H
#define RHI_DEF_H

#ifdef RHI_BUILD_SHARED
#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef RHI_BUILD
#define RHI_API __declspec(dllexport)
#else
#define RHI_API __declspec(dllimport)
#endif
#elif __GNUC__ >= 4
#define RHI_API __attribute__((visibility("default")))
#else
#define RHI_API
#endif
#else
#define RHI_API
#endif

#endif // RHI_DEF_H
