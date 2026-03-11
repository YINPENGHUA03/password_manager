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
- **🐳 Cloud-Native Deployment:** Utilizes Docker multi-stage builds to decouple the C++ compilation chain from the production runtime, resulting in an ultra-lean microservice image.

### 🛠️ Tech Stack

- **Core Engine:** C++17, OpenSSL (libcrypto), pybind11
- **Backend Service:** Python 3.12, FastAPI, Uvicorn
- **Frontend:** HTML5, CSS3, ES6 JavaScript (Fetch API)
- **Database:** MySQL 8.0 (C API)
- **Build System:** Docker,CMake

### 🚀 Quick Start

#### 1. Prerequisites
Ensure you have a local MySQL server running and create the necessary database and tables using the SQL schema below:
<details>
<summary><b>Click to expand: MySQL Database Setup SQL</b></summary>

```sql
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

</details>
Option A: Docker Deployment (Recommended)
Build and run the highly optimized containerized microservice:

```Bash
git clone [https://github.com/YINPENGHUA03/password_manager.git](https://github.com/YINPENGHUA03/password_manager.git)
cd password_manager

# Build the multi-stage Docker image
docker build -t cpp-auth-gateway:latest .

# Run the container (bridging to host's MySQL via host.docker.internal)
docker run -d --name auth_server \
  --add-host=host.docker.internal:host-gateway \
  -p 8000:8000 \
  cpp-auth-gateway:latest
```
Visit http://localhost:8000 to experience the system.


Option B: Manual Local Build
<details>
<summary><b>Click to expand: Manual Build Instructions</b></summary>
1.Install dependencies:
    
```Bash
    sudo apt-get update
sudo apt-get install g++ cmake libmysqlclient-dev libssl-dev python3-dev pybind11-dev python3-pip python3-venv
```
2.Build C++ Engine:

```Bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
3.Run FastAPI Server:
```Bash
cd ..
python3 -m venv venv
source venv/bin/activate
pip install fastapi uvicorn pydantic
uvicorn server:app --host 0.0.0.0 --port 8000
```
</details>


<h2 id="中文">🇨🇳 中文</h2>

这是一个从底层 C++ 命令行工具进化而来的现代化全栈密码管理系统。本项目采用 C++ 作为密码学与内存安全的核心引擎，结合 Python (FastAPI) 构建高并发微服务网关，在保证极致防御深度的同时，提供了极佳的用户交互体验。

🌟 核心架构与特性
高强度底层认证 (C++)：采用 PBKDF2-SHA256 算法进行密码哈希，并结合动态盐值 (Dynamic Salts) 抵御彩虹表和离线暴力破解。

跨语言桥接重构 (pybind11)：将 C++ 核心业务逻辑编译为动态链接库 (.so)，使 Python 后端能够以原生级别的极速调用底层复杂的密码学验证。

抗计时攻击 (C++)：引入 Dummy Hashing (虚假哈希) 技术，确保验证逻辑在处理“账号存在”或“不存在”时耗时恒定，彻底封堵基于时间差的账号探测。

防御内存倾印 (C++)：核心生命周期结束后，强制调用 OPENSSL_cleanse 对驻留在物理内存 (RAM) 中的明文密码进行安全擦除。

现代化全栈交互 (Vanilla JS)：零外部框架依赖，实现了极简高级感 UI、客户端数据一致性拦截，以及基于三角函数 (Math.atan2) 的精准鼠标视线追踪 SVG 动画。

资源级数据库安全：采用 MySQL 预处理语句 (Prepared Statements) 杜绝 SQL 注入，并全面践行 RAII 机制，利用 std::unique_ptr 封装 C API 资源，实现 0 内存泄漏。

🐳 云原生集装箱化：采用 Docker 多阶段构建 (Multi-stage Build) 技术，彻底剥离 C++ 编译链与运行期环境，打造超轻量级、开箱即用的工业级微服务镜像。

🛠️ 技术栈
底层引擎: C++17, OpenSSL (libcrypto), pybind11

后端微服务: Python 3.12, FastAPI, Uvicorn

前端交互: HTML5, CSS3, ES6 JavaScript (Fetch API)

数据库: MySQL 8.0 (C API)

构建工具: Docker,CMake

🚀 快速开始
首先，请确保本地 MySQL 服务已启动，并执行以下 SQL 初始化数据库与表结构：

<details>
<summary><b>点击展开：MySQL 数据库建表语句</b></summary>

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
（注意：请确保 db.cpp 中的数据库连接账号密码与本地环境一致，Docker 部署需使用 host.docker.internal）。

</details>

方案 A：Docker 容器化部署（推荐）
一键构建并运行经过极致瘦身的微服务镜像：
```Bash
git clone [https://github.com/YINPENGHUA03/password_manager.git](https://github.com/YINPENGHUA03/password_manager.git)
cd password_manager

# 构建多阶段 Docker 镜像
docker build -t cpp-auth-gateway:latest .

# 启动容器（利用 host.docker.internal 穿透访问宿主机数据库）
docker run -d --name auth_server \
  --add-host=host.docker.internal:host-gateway \
  -p 8000:8000 \
  cpp-auth-gateway:latest
```
在浏览器中访问 http://localhost:8000 即可体验系统。

方案 B：本地源码编译运行
<details>
<summary><b>点击展开：传统本地编译指南</b></summary>
1.安装基础编译环境：
    
```Bash
sudo apt-get update
sudo apt-get install g++ cmake libmysqlclient-dev libssl-dev python3-dev pybind11-dev python3-pip python3-venv
```

2.编译 C++ 核心引擎：

```Bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

3.启动 FastAPI 服务：
```Bash
cd ..
python3 -m venv venv
source venv/bin/activate
pip install fastapi uvicorn pydantic
uvicorn server:app --host 0.0.0.0 --port 8000
```
</details>
