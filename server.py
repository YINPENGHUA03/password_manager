from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import sys
sys.path.append("./build") # 告诉 Python 去 build 文件夹里找 C++ 引擎
import pwd_core
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

# 1. 初始化 Web 服务器
app = FastAPI(title="高安全密码管理器 API", description="基于 C++ 核心引擎的 Python 后端")

# 2. 实例化 C++ 数据库对象
db = pwd_core.Database()

# 3. 定义前端发来的数据格式 (JSON)
class UserRequest(BaseModel):
    username: str
    password: str

# 4. 编写登录接口 (POST 请求)
@app.post("/api/login")
def login(user: UserRequest):
    # 瞬间调用 C++ 底层的高强度防爆破验证！
    if db.checkLogin(user.username, user.password):
        return {"status": "success", "message": "登录成功！底层 PBKDF2 验证通过。"}
    else:
        # 如果验证失败，返回标准的 401 未授权 HTTP 状态码
        raise HTTPException(status_code=401, detail="账号或密码错误")

# 5. 编写注册接口
@app.post("/api/register")
def register(user: UserRequest):
    if db.registerUser(user.username, user.password):
        return {"status": "success", "message": f"用户 {user.username} 注册成功！"}
    else:
        raise HTTPException(status_code=400, detail="注册失败，用户可能已存在")
