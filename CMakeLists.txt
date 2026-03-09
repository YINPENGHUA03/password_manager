cmake_minimum_required(VERSION 3.15)
project(password_manager)

# 告诉 CMake 我们用的是 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 寻找 Python 和 pybind11 的开发库
find_package(pybind11 REQUIRED)

# 2. 核心指令：不要再生成可执行程序了！
# 我们要生成一个 Python 模块，名字叫 pwd_core (必须和 bindings.cpp 里的名字一模一样)
# 把 bindings.cpp (胶水) 和 db.cpp (核心逻辑) 一起扔进去炼丹！
pybind11_add_module(pwd_core bindings.cpp db.cpp)

# 3. 链接底层的 MySQL 和 OpenSSL 加密库
target_link_libraries(pwd_core PRIVATE mysqlclient crypto)
