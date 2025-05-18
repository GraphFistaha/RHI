#pragma once

#if defined(_WIN32)
#if defined(RHI_BUILD)
#define RHI_API __declspec(dllexport)
#else
#define RHI_API __declspec(dllimport)
#endif
#else
#define RHI_API
#endif