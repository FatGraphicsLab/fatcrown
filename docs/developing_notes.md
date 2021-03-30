# Developing Notes


## 2021-03-30

short objective

* see unittest runs~ (DONE)

core/memory

* HeapAllocator, ScratchAllocator in `memory/globals.cpp` are basic allocator.
* ScratchAllocator is mainly used by TempAllocator.

core/thread

* 多线程基础设施
* ConditionVariable, Mutex, ScopedMutex, Semaphore, Thread

finished

* `src/core/memory/globals.cpp`
* `src/core/memory/globals.h`
* `src/core/memory/memory.inl`
* `src/core/memory/temp_allocator.inl`
* `src/core/thread/mutex.cpp`
* `src/core/thread/mutex.h`
* `src/core/thread/scoped_mutex.inl`
* `src/core/unit_test.cpp`
* `src/core/unit_test.h`


## 2021-03-29

solutions

* re-arrange `[vs2017]` into `[solutions]`
* we will support more platform in the future

short objective

* see unittest runs~

finished

* `src/core/error/error.h`
* `src/core/error/error.inl`
* `src/memory/allocator.h`
* `src/memory/types.h`


## 2021-03-27

finished

* `src/core/types.h`, s8/u8/s32/u32等等，以及clamp/max/min等模板函数

types.h

* 每个模块目录下都会有一个 `types.h` 文件，定义了模块相关的数据
* 模块之间的依赖，尽量只需要 include 对应的 `types.h` 文件


## 2021-03-20

CommandLine

* `src/core/command_line.h & .cpp`
  * depends on `src/core/strings/string.inl`
    * depends on `src/error/error.inl`

build config macro

* CROWN_DEBUG, msvc Debug
* CROWN_RELEASE, msvc Release
* don't support CROWN_DEVELOPMENT

compiling macros

* `src/core/platform.h` 定义所有 macro 的默认值
* `src/config.h` 根据 platform info，修正 `platform.h` 中的所有 macro
