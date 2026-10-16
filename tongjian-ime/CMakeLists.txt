cmake_minimum_required(VERSION 3.15)
project(TongjianIME CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 64 位 DLL（Windows 输入法需与宿主进程同位数，主流为 x64）
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "请使用 64 位工具链生成（例如 Visual Studio 的 x64 生成器）")
endif()

add_library(TongjianIME SHARED
    src/dllmain.cpp
    src/TextService.cpp
    src/CandidateWindow.cpp
    src/DictEngine.cpp
    src/exports.def
)

target_include_directories(TongjianIME PRIVATE src)

target_compile_definitions(TongjianIME PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0601
)

# 源码为 UTF-8，需显式告知 MSVC，否则中文注释/字符串会被按 ANSI 误读
if(MSVC)
    target_compile_options(TongjianIME PRIVATE /utf-8)
    # 静态链接运行库，使 DLL 不依赖 vcruntime140.dll，可在任意 Windows 机器使用
    set_property(TARGET TongjianIME PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

target_link_libraries(TongjianIME PRIVATE
    advapi32
    ole32
    oleaut32
    uuid
    user32
    gdi32
    imm32
)

set_target_properties(TongjianIME PROPERTIES OUTPUT_NAME "TongjianIME")
