# ==============================================================================
# 1. 声明要求的最低 CMake 版本 (Define the minimum required CMake version)
# ==============================================================================
cmake_minimum_required(VERSION 3.10)

# ==============================================================================
# 2. 声明项目名称和使用的语言 (Declare project name and language: C++)
# ==============================================================================
project(PasswordManager CXX)

# ==============================================================================
# 3. 强制使用 C++17 标准 (Enforce C++17 standard)
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# 4. 如果没有指定构建模式，默认使用 Debug 模式 
#    (Default to Debug mode if no build type is specified)
# ==============================================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# 在终端打印当前使用的构建模式 (Print the current build type to the terminal)
message(STATUS "🚀 当前构建模式 (Current Build Type): ${CMAKE_BUILD_TYPE}")

# ==============================================================================
# 5. 配置三种模式的编译参数 (Configure compiler flags for three build modes)
# ==============================================================================

# Debug 模式: 包含调试信息，关闭优化 (Includes debug info, disables optimization)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")

# Release 模式: 开启 O2 优化，用于性能测试 (Enables O2 optimization for performance testing)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall")

# ASAN 模式: 开启 AddressSanitizer 进行内存越界检测 (Enables AddressSanitizer for memory error detection)
set(CMAKE_CXX_FLAGS_ASAN "-g -O1 -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS_ASAN "-fsanitize=address")

# ==============================================================================
# 6. 寻找外部依赖库 (Find external dependencies)
# ==============================================================================
find_package(OpenSSL REQUIRED)

# ==============================================================================
# 7. 生成可执行文件 (Generate the executable)
# ==============================================================================
add_executable(password_manager main.cpp db.cpp)

# ==============================================================================
# 8. 链接依赖库 (Link target libraries: OpenSSL Crypto & MySQL C API)
# ==============================================================================
target_link_libraries(password_manager 
    PRIVATE 
    OpenSSL::Crypto 
    mysqlclient
)
