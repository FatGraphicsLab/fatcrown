# Developing Notes


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
