# High-Security C++ Password Manager / 高安全 C++ 密码管理器

[English](#english) | [中文](#中文)

---

<h2 id="english" /h2>Us English</h2>

This is a lightweight, high-security command-line password management system developed in modern C++. The project integrates cryptographic hashing, secure database management, and multiple robust defense mechanisms against common attack vectors, aiming to provide a hardened local authentication solution.

### 🌟 Core Features

- **High-Strength Authentication:** Employs the `PBKDF2-SHA256` algorithm for password hashing, combined with **Dynamic Salts** to effectively thwart rainbow table attacks.
- **Safe Resource Management:** Fully embraces the **RAII (Resource Acquisition Is Initialization)** paradigm, utilizing `std::unique_ptr` with custom deleters to automatically manage MySQL C API resources, guaranteeing **zero memory leaks**.
- **Timing Attack Defense:** Implements **Dummy Hashing** logic to ensure constant-time response for both existing and non-existing user queries, preventing malicious user enumeration.
- **Sensitive Data Protection:**
  - Utilizes the Linux `termios` API to implement terminal-level **password masking** (hidden input).
  - Actively wipes sensitive memory (e.g., plaintext passwords) from RAM immediately after use via `OPENSSL_cleanse` to defend against memory dump exploits.
- **Persistent Storage:** Uses a MySQL database via the C API with Prepared Statements to store structured data and prevent SQL Injection.

### 🛠️ Tech Stack

- **Language:** C++17
- **Database:** MySQL (C API)
- **Cryptography:** OpenSSL (libcrypto)
- **OS:** Linux (Requires POSIX `termios` for terminal control)
- **Build System:** CMake

### 🚀 Quick Start

#### Prerequisites
Ensure the following dependencies are installed on your Linux system:
```bash
sudo apt-get install g++ cmake libmysqlclient-dev libssl-dev mysql-server
```

1. Database Setup
Log into your MySQL server and set up the database and table:

```SQL
CREATE DATABASE IF NOT EXISTS password_manager_db;
USE password_manager_db;

CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(64) NOT NULL,
    salt VARCHAR(32) NOT NULL,
    fail_count INT DEFAULT 0,
    lock_until DATETIME NULL
);
```
(Note: Please update the database credentials in db.cpp to match your local setup).

2. Build & Run
Follow the standard CMake out-of-source build process:

```Bash
git clone [https://github.com/YINPENGHUA03/password_manager.git](https://github.com/YINPENGHUA03/password_manager.git)
cd password_manager

# Create build directory and generate Makefiles using Release mode for performance
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Compile the project
make

# Run the executable
./password_manager
```
<h2 id="中文">🇨🇳 中文</h2>

这是一个基于现代 C++ 开发的轻量级、高安全性命令行密码管理系统。项目集成了密码学哈希算法、数据库安全管理以及多种防攻击底层机制，旨在提供一个具备防御深度的本地身份认证解决方案。

🌟 核心特性
高强度认证：采用 PBKDF2-SHA256 算法进行密码哈希，并结合动态盐值 (Dynamic Salts) 抵御彩虹表和离线暴力破解。

资源安全管理：全面践行 RAII (资源获取即初始化) 机制，利用 std::unique_ptr 封装 MySQL C API 资源，实现 0 内存泄漏。

抗计时攻击：引入 Dummy Hashing (虚假哈希) 技术，确保验证逻辑在处理“账号存在”或“不存在”时耗时恒定，彻底封堵基于时间差的账号探测。

敏感信息保护：

调用 Linux termios 库接管底层 I/O，实现终端级别的密码输入遮罩。

核心生命周期结束后，强制调用 OPENSSL_cleanse 对驻留在物理内存 (RAM) 中的明文密码进行安全擦除，防止内存倾印 (Memory Dump) 窃取。

持久化存储：采用 MySQL 预处理语句 (Prepared Statements) 进行结构化数据操作，从根源上杜绝 SQL 注入漏洞。

🛠️ 技术栈
语言: C++17

数据库: MySQL (C API)

安全库: OpenSSL (libcrypto)

操作系统: Linux (涉及 termios 终端控制)

构建工具: CMake

🚀 快速开始
前置条件
确保你的 Linux 系统中已安装以下环境：

```Bash
sudo apt-get install g++ cmake libmysqlclient-dev libssl-dev mysql-server
```

1. 初始化数据库
登录本地 MySQL，执行以下 SQL 语句建立必要的表结构：

```SQL
CREATE DATABASE IF NOT EXISTS password_manager_db;
USE password_manager_db;

CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(64) NOT NULL,
    salt VARCHAR(32) NOT NULL,
    fail_count INT DEFAULT 0,
    lock_until DATETIME NULL
);
```
（注意：请确保 db.cpp 中的数据库连接账号密码与你本地环境一致）

2. 编译与运行
本项目采用标准的 CMake 外部构建流程：

```Bash
git clone [https://github.com/YINPENGHUA03/password_manager.git](https://github.com/YINPENGHUA03/password_manager.git)
cd password_manager

# 创建构建目录，并指定为 Release 模式以开启性能优化
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# 开始编译
make

# 运行程序
./password_manager
```
---
