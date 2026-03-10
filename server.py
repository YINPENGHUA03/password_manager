import sys
sys.path.append("./build")
import pwd_core
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from fastapi.responses import HTMLResponse
import re # 引入正则表达式库，用于校验密码强度

app = FastAPI(title="高安全密码管理器 API")
db = pwd_core.Database()

class UserRequest(BaseModel):
    username: str
    password: str

class ChangePwdRequest(BaseModel):
    username: str
    oldpw: str
    newpw: str

# ---------------------------------------------------------
# 核心校验函数：复刻你原本 C++ main.cpp 里的强度逻辑
# ---------------------------------------------------------
def check_password_strength(password: str):
    if len(password) < 8:
        return False, "密码太弱：长度至少需要 8 位"
    if not re.search(r"[a-z]", password) or not re.search(r"[A-Z]", password):
        return False, "密码太弱：必须包含大小写字母"
    if not re.search(r"\d", password):
        return False, "密码太弱：必须包含数字"
    return True, "OK"

# ---------------------------------------------------------
# API 路由
# ---------------------------------------------------------
@app.post("/api/login")
def login(user: UserRequest):
    if db.checkLogin(user.username, user.password):
        return {"status": "success", "message": "登录成功！底层 PBKDF2 验证通过。"}
    else:
        raise HTTPException(status_code=401, detail="账号或密码错误！")

@app.post("/api/register")
def register(user: UserRequest):
    # 1. 在调用 C++ 之前，先拦截弱密码！
    is_strong, msg = check_password_strength(user.password)
    if not is_strong:
        raise HTTPException(status_code=400, detail=msg)
        
    # 2. 密码合格，才放行给 C++ 引擎
    if db.registerUser(user.username, user.password):
        return {"status": "success", "message": f"用户 {user.username} 注册成功！"}
    else:
        raise HTTPException(status_code=400, detail="注册失败，用户可能已存在")

@app.post("/api/change_password")
def change_password(req: ChangePwdRequest):
    # 修改密码时同样要校验新密码的强度
    is_strong, msg = check_password_strength(req.newpw)
    if not is_strong:
        raise HTTPException(status_code=400, detail="新" + msg)

    if db.changePassword(req.username, req.oldpw, req.newpw):
        return {"status": "success", "message": "密码修改成功！底层数据已更新。"}
    else:
        raise HTTPException(status_code=400, detail="修改失败：原密码错误或用户不存在")

@app.get("/")
def serve_frontend():
    with open("index.html", "r", encoding="utf-8") as f:
        return HTMLResponse(content=f.read())
