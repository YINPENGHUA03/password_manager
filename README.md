# C++ 密码管理器 (Password Manager)

这是一个基于 C++ 开发的轻量级、高安全性命令行密码管理系统。项目集成了加密哈希算法、数据库管理以及多种防攻击安全机制，旨在提供一个安全的本地密码存储解决方案。

## 🌟 核心特性

- **高强度认证**：采用 `PBKDF2-SHA256` 算法进行密码哈希，并结合**动态盐值 (Dynamic Salts)** 抵御彩虹表攻击。
- **资源安全管理**：全面应用 **RAII (Resource Acquisition Is Initialization)** 机制，利用 `std::unique_ptr` 自动管理 MySQL C API 资源，防止内存泄漏。
- **抗时间攻击**：引入 **Dummy Hashing (伪哈希)** 技术，确保验证逻辑在处理存在或不存在的用户时耗时接近，防御时间攻击 (Timing Attacks)。
- **敏感信息保护**：
    - 使用 `termios` 库实现终端级别的**密码掩码输入**（输入时不显示明文）。
    - 关键内存区域使用 `OPENSSL_cleanse` 及时擦除，防止敏感数据残留在 RAM 中。
- **持久化存储**：使用 MySQL 数据库进行结构化数据存储，确保密码记录的高效存取。

## 🛠️ 技术栈

- **语言**: C++11/14+
- **数据库**: MySQL (C API)
- **安全库**: OpenSSL (libcrypto)
- **操作系统**: Linux (涉及 termios 终端控制)
- **构建工具**: GNU Makefile

## 🚀 快速开始

### 前置条件

确保你的系统中已安装以下环境：
- `g++` 编译器
- `libmysqlclient-dev` (MySQL 开发库)
- `libssl-dev` (OpenSSL 开发库)
- MySQL Server

### 安装与运行

1. **克隆仓库**
   ```bash
   git clone [https://github.com/YINPENGHUA03/password_manager.git](https://github.com/YINPENGHUA03/password_manager.git)
   cd password_manager
