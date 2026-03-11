# ==========================================
# 阶段一：重装编译车间 (Builder Stage)
# ==========================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 1. 安装编译狂魔级依赖 (包含 venv 虚拟环境工具)
RUN apt-get update && apt-get install -y \
    g++ cmake make libmysqlclient-dev libssl-dev pybind11-dev python3-dev python3-venv

WORKDIR /app
COPY . .

# 2. 冶炼 C++ 底层引擎
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make

# 3. 核心魔法：在车间里构建 Python 虚拟环境，并安装 FastAPI
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"
RUN pip install --no-cache-dir fastapi uvicorn pydantic

# ==========================================
# 阶段二：超轻量生产舱 (Production Stage)
# 目标：拒绝任何编译工具，只保留运行时 (Runtime)
# ==========================================
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. 【极致瘦身】：拒绝 -dev 包！只安装纯净版运行库 libmysqlclient21
# 使用 --no-install-recommends 拒绝系统强塞的无关文档和辅助包
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 libmysqlclient21 libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 2. 从 Builder 偷取 C++ 引擎
COPY --from=builder /app/build/*.so ./build/
# 3. 从 Builder 偷取极其干净的 Python 虚拟环境！(彻底抛弃 pip)
COPY --from=builder /opt/venv /opt/venv
# 4. 拷贝业务代码
COPY server.py index.html ./

# 5. 激活“偷”来的虚拟环境
ENV PATH="/opt/venv/bin:$PATH"

EXPOSE 8000
CMD ["uvicorn", "server:app", "--host", "0.0.0.0", "--port", "8000"]
