// Predefined macros in C++ compilers
// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc.html

#pragma once

#define CROWN_COMPILER_CLANG 0
#define CROWN_COMPILER_GCC   0
#define CROWN_COMPILER_MSVC  0

#define CROWN_PLATFORM_ANDROID 0
#define CROWN_PLATFORM_IOS     0
#define CROWN_PLATFORM_LINUX   0
#define CROWN_PLATFORM_OSX     0
#define CROWN_PLATFORM_WINDOWS 0

#define CROWN_CPU_ARM 0
#define CROWN_CPU_X86 0

#define CROWN_CPU_32BIT 0
#define CROWN_CPU_64BIT 0


// Detects Compiler
#if defined(_MSC_VER)
#  undef CROWN_COMPILER_MSVC
#  define CROWN_COMPILER_MSVC 1
#elif defined(__clang__)
#  undef CROWN_COMPILER_CLANG
#  define CROWN_COMPILER_CLANG 1
#elif defined(__GNUC__)
#  undef CROWN_COMPILER_GCC
#  define CROWN_COMPILER_GCC 1
#else
#  error "CROWN_COMPILER_* is not defined!"
#endif

// Detects Platform
#if defined(_WIN32) || defined(_WIN64)
#  undef CROWN_PLATFORM_WINDOWS
#  if !defined(WINVER) && !defined(_WIN32_WINNT)  // http://msdn.microsoft.com/en-us/library/6sehtctf.aspx
#    define WINVER 0x0601                         // win7 and above
#    define _WIN32_WINNT 0x0601
#  endif
#  define CROWN_PLATFORM_WINDOWS 1
#elif defined(__ANDROID__)
#  undef CROWN_PLATFORM_ANDROID
#  define CROWN_PLATFORM_ANDROID 1
#elif defined(__linux__)
#  undef CROWN_PLATFORM_LINUX
#  define CROWN_PLATFORM_LINUX 1
#elif defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
#  undef CROWN_PLATFORM_IOS
#  define CROWN_PLATFORM_IOS 1
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#  undef CROWN_PLATFORM_OSX
#  define CROWN_PLATFORM_OSX 1
#else
#  error "CROWN_PLATFORM_* is not defined!"
#endif

#define CROWN_PLATFORM_POSIX (CROWN_PLATFORM_ANDROID \
                        || CROWN_PLATFORM_IOS        \
                        || CROWN_PLATFORM_LINUX      \
                        || CROWN_PLATFORM_OSX)

// Detects CPU
#if defined(__arm__)
#  undef CROWN_CPU_ARM
#  define CROWN_CPU_ARM 1
#  define CROWN_CACHE_LINE_SIZE 64
#elif defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#  undef CROWN_CPU_X86
#  define CROWN_CPU_X86 1
#  define CROWN_CACHE_LINE_SIZE 64
#else
#  error "CROWN_CPU_* is not defined!"
#endif

// Detects Arch Bits
#if defined(__x86_64__) || defined(_M_X64) || defined(__64BIT__) || defined(__powerpc64__) || defined(__ppc64__)
#  undef CROWN_CPU_64BIT
#  define CROWN_CPU_64BIT 1
#else
#  undef CROWN_CPU_32BIT
#  define CROWN_CPU_32BIT 1
#endif


// Platform Name
#if CROWN_PLATFORM_ANDROID
#  define CROWN_PLATFORM_NAME "android"
#elif CROWN_PLATFORM_IOS
#  define CROWN_PLATFORM_NAME "ios"
#elif CROWN_PLATFORM_Linux
#  define CROWN_PLATFORM_NAME "linux"
#elif CROWN_PLATFORM_OSX
#  define CROWN_PLATFORM_NAME "osx"
#elif CROWN_PLATFORM_WINDOWS
#  define CROWN_PLATFORM_NAME "windows"
#endif

// Arch Name
#if CROWN_ARCH_32BIT
#  define CROWN_ARCH_NAME "32-bit"
#elif CROWN_ARCH_64BIT
#  define CROWN_ARCH_NAME "64-bit"
#endif
