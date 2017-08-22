#ifndef _SHARED_LIB_H_
#define _SHARED_LIB_H_

#ifdef __GNUC__
#define DLL_IMPORT __attribute__((dllimport))
#define DLL_EXPORT __attribute__((dllexport))
#elif defined(_MSC_VER)
#define DLL_IMPORT __declspec(dllimport)
#define DLL_EXPORT __declspec(dllexport)
#endif

#if defined(WIN32)||defined(_WIN32)
#include <Windows.h>
#define dll_t          HMODULE
#define dlopen(x,opt)  LoadLibraryA(x)
#define dlsym(h,x)     GetProcAddress(h,x)
#define dlclose(h)     FreeLibrary(h)
#else
#include <dlfcn.h>
#define dll_t          void*
#endif

#endif/*_SHARED_LIB_H_*/