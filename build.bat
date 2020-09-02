
@echo off



set flags_debug=   -std=c99 -Wall -Wextra -pedantic-errors -fextended-identifiers -g -D DEBUG
set flags_release=   -std=c99 -Wall -Wextra -pedantic-errors -fextended-identifiers

set clang_flags_debug= /Z7 /DDEBUG /W4 /WX /MDd -Qunused-arguments
set clang_flags_debug_easy= /Z7 /DDEBUG /W4 /MDd -Qunused-arguments -Wno-unused-variable
set clang_flags_release= /O2 /W4 /MD -Qunused-arguments -Wno-unused-variable

pushd "%~dp0"
mkdir build
pushd build

REM gcc %flags_debug%   ..\src\main.c -o main_gcc_dbg
REM gcc %flags_release% ..\src\main.c -o main_gcc_release

REM clang-cl %clang_flags_debug%  ..\src\main.c -o main_clang_dbg
clang-cl %clang_flags_debug_easy% ..\src\main.c -o main_clang_dbg_easy.exe
REM clang-cl %clang_flags_debug_release% ..\src\main.c -o main_clang_release.exe

popd
popd


