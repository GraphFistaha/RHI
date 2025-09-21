#ifndef TEST_UTILS_DEF_H
#define TEST_UTILS_DEF_H

#if defined(_WIN32)
#if defined(TEST_UTILS_BUILD)
#define TEST_UTILS_API __declspec(dllexport)
#else
#define TEST_UTILS_API __declspec(dllimport)
#endif
#else
#define TEST_UTILS_API
#endif

#endif // TEST_UTILS_DEF_H
