# 🛡️ Advanced Password Manager (C++ Core & Python Microservice)

[English](#english) | [中文](#中文)

---

<h2 id="english">🇬🇧 English</h2>

This is a high-security password management system that has evolved from a hardened C++ CLI tool into a modern web-based microservice. The project leverages **C++ for cryptographic heavy-lifting** and **Python (FastAPI) for high-concurrency API routing**, delivering a seamless full-stack experience without compromising low-level memory safety.

### 🌟 Core Architecture & Features

- **High-Strength Authentication (C++):** Employs the `PBKDF2-SHA256` algorithm combined with **Dynamic Salts** to effectively thwart rainbow table attacks.
- **Cross-Language Bridge (pybind11):** The C++ core is compiled into a dynamic shared object (`.so`), allowing the Python FastAPI backend to execute memory-safe cryptographic validations at native speeds.
- **Timing Attack Defense (C++):** Implements **Dummy Hashing** logic to ensure constant-time response for both existing and non-existing user queries, preventing malicious user enumeration.
- **Memory Dump Protection (C++):** Actively wipes sensitive memory (e.g., plaintext passwords) from RAM immediately after use via `OPENSSL_cleanse`.
- **Modern Full-Stack UI (Vanilla JS):** A sleek, dark/light mode adaptable frontend featuring state-machine form toggling, client-side consistency checks, and a custom `Math.atan2` driven eye-tracking SVG animation.
- **Persistent Storage:** Uses a MySQL database via the C API with Prepared Statements, managed entirely via the **RAII** paradigm (`std::unique_ptr`) to guarantee **zero memory leaks** and immunity to SQL Injections.

### 🛠️ Tech Stack

- **Core Engine:** C++17, OpenSSL (libcrypto), pybind11
- **Backend Service:** Python 3.12, FastAPI, Uvicorn
- **Frontend:** HTML5, CSS3, ES6 JavaScript (Fetch API)
- **Database:** MySQL 8.0 (C API)
- **Build System:** CMake

### 🚀 Quick Start

#### 1. Prerequisites
Ensure the following dependencies are installed on your Linux system:
```bash
sudo apt-get update
sudo apt-get install g++ cmake libmysqlclient-dev libssl-dev mysql-server python3-dev pybind11-dev python3-pip
```

2. Database Setup
Log into your MySQL server and execute:

```SQL
CREATE DATABASE IF NOT EXISTS login_db;
USE login_db;

CREATE TABLE user (
    id INT NOT NULL AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL,
    password VARCHAR(128) NOT NULL,
    salt VARCHAR(32) NOT NULL,
    iter INT NOT NULL DEFAULT 100000,
    fail_count INT NOT NULL DEFAULT 0,
    lock_until DATETIME DEFAULT NULL,
    PRIMARY KEY (id)
);
```
(Note: Update the database credentials in db.cpp to match your local setup).


3. Build C++ Core Engine
```Bash
git clone [https://github.com/YINPENGHUA03/password_manager.git](https://github.com/YINPENGHUA03/password_manager.git)
cd password_manager

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```


4. Start the Web Microservice
Return to the project root, install Python dependencies, and launch the server:

```Bash
cd ..
pip3 install fastapi uvicorn pydantic --break-system-packages
uvicorn server:app --host 0.0.0.0 --port 8000
Open your browser and navigate to http://localhost:8000 to experience the system.
```

<h2 id="中文">🇨🇳 中文</h2>

这是一个从底层 C++ 命令行工具进化而来的现代化全栈密码管理系统。本项目采用 C++ 作为密码学与内存安全的核心引擎，结合 Python (FastAPI) 构建高并发微服务网关，在保证极致防御深度的同时，提供了极佳的用户交互体验。

🌟 核心架构与特性
高强度底层认证 (C++)：采用 PBKDF2-SHA256 算法进行密码哈希，并结合动态盐值 (Dynamic Salts) 抵御彩虹表和离线暴力破解。

跨语言桥接重构 (pybind11)：将 C++ 核心业务逻辑编译为动态链接库 (.so)，使 Python 后端能够以原生级别的极速调用底层复杂的密码学验证。

抗计时攻击 (C++)：引入 Dummy Hashing (虚假哈希) 技术，确保验证逻辑在处理“账号存在”或“不存在”时耗时恒定，彻底封堵基于时间差的账号探测。

防御内存倾印 (C++)：核心生命周期结束后，强制调用 OPENSSL_cleanse 对驻留在物理内存 (RAM) 中的明文密码进行安全擦除。

现代化全栈交互 (Vanilla JS)：零外部框架依赖，实现了极简高级感 UI、客户端数据一致性拦截，以及基于三角函数 (Math.atan2) 的精准鼠标视线追踪 SVG 动画。

资源级数据库安全：采用 MySQL 预处理语句 (Prepared Statements) 杜绝 SQL 注入，并全面践行 RAII 机制，利用 std::unique_ptr 封装 C API 资源，实现 0 内存泄漏。

🛠️ 技术栈
底层引擎: C++17, OpenSSL (libcrypto), pybind11

后端微服务: Python 3.12, FastAPI, Uvicorn

前端交互: HTML5, CSS3, ES6 JavaScript (Fetch API)

数据库: MySQL 8.0 (C API)

构建工具: CMake

🚀 快速开始
1. 前置条件
确保系统中已安装必要的编译链与开发库：

```Bash
sudo apt-get update
sudo apt-get install g++ cmake libmysqlclient-dev libssl-dev mysql-server python3-dev pybind11-dev python3-pip
```


2. 初始化数据库
登录 MySQL 并执行建表语句：

```SQL
CREATE DATABASE IF NOT EXISTS login_db;
USE login_db;

CREATE TABLE user (
    id INT NOT NULL AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL,
    password VARCHAR(128) NOT NULL,
    salt VARCHAR(32) NOT NULL,
    iter INT NOT NULL DEFAULT 100000,
    fail_count INT NOT NULL DEFAULT 0,
    lock_until DATETIME DEFAULT NULL,
    PRIMARY KEY (id)
);
```
（注意：请确保 db.cpp 中的数据库连接账号密码与本地环境一致）


3. 编译 C++ 核心引擎
```Bash
git clone [https://github.com/YINPENGHUA03/password_manager.git](https://github.com/YINPENGHUA03/password_manager.git)
cd password_manager

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```


4. 启动 Web 微服务
回到项目根目录，安装 Python 依赖并启动后端服务器：

```Bash
cd ..
pip3 install fastapi uvicorn pydantic --break-system-packages
uvicorn server:app --host 0.0.0.0 --port 8000
```

在浏览器中访问 http://localhost:8000 即可体验系统。
