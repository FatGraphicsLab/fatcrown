# Developing Notes



## OBJECTIVES

* SSAO
  * https://ourmachinery.com/post/beta-2021-5/#screen-space-ambient-occlusion
* debug-build unittest works~
  * depends on `crown::error::abort()`



## 2021-06-06

core/string/string_view.h

* 对 const char* 的一个片段做 view
* 支持 ==, !=, < 等操作，方便放到容器中

finished

* `core/strings/string_view.h`
* `core/strings/string_view.inl`



## 2021-06-05

core/strings/string_id.h

* 表示 hashed string，方便做 ==, !=, <
* 假设 hash function 不会重复的情况下，对于大量的字符串，保存 hashed value 更节约空间
  * 比如："assets/monsters/3155/a.model" 保存为 StringId32 只需要 4-bytes

finished

* `core/strings/string_id.cpp`
* `core/strings/string_id.h`
* `core/strings/string_id.inl`



## 2021-06-04

ALLOCATOR_AWARE

* only used for `hash_map.inl` / `hash_set.inl`
* crown::construct of `memory.inl` is only used for `hash_map.inl` / `hash_set.inl`

finished

* `core/containers/pair.h`
* `core/containers/pair.inl`



## 2021-06-03

core/strings/string_stream.inl

* depends on `core/containers/array.inl`

finished

* `core/strings/string_stream.h`
* `core/strings/string_stream.inl`



## 2021-04-01

core/error/callstack.h

* depends on `core/strings/string_stream.h`
* depends on `core/memory/temp_allocator.inl`

finished

* `core/strings/string.inl`
* `core/strings/types.h`



## 2021-03-31

unittest

* full unittest for `containers/array.inl`

project rearrangement objective

* put unittest code to seperate project `[utils/unittest]`
* `fatcrown` and `unittest` both depends on `libcrown`
* so make all engine runtime code to a seperate .lib

project rearrangement

* libcrown, the engine runtime lib
* fatcrown, launcher of libcrown
* unittest, unittest for libengine's core functionality
* fateditor, game editor for fatcrown

finished

* `src/core/containers/array.inl`
* `src/core/containers/types.h`
* `src/core/functional/array.inl`
* `src/core/containers/types.h`
* `src/core/functional.h`
* `src/core/functional.inl`
* `src/core/murmur.cpp`
* `src/core/murmur.h`



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
* `src/core/unit_tests.cpp`
* `src/core/unit_tests.h`



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
* 模块之间的依赖，只需要 include 对应的 `types.h` 文件


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
