#include <pybind11/pybind11.h>
#include "db.h"

namespace py = pybind11;

// "pwd_core" 就是你未来在 Python 里 import 的模块名字！
PYBIND11_MODULE(pwd_core, m) {
    m.doc() = "Password Manager Core C++ Plugin / 密码管理核心 C++ 底层插件";

    // 把 C++ 的 Database 类映射到 Python 中
    py::class_<Database>(m, "Database")
        .def(py::init<>()) // 暴露构造函数，让 Python 可以实例化它: db = Database()
        .def("isConnected", &Database::isConnected, "检查数据库连接状态")
        .def("checkLogin", &Database::checkLogin, 
             py::arg("username"), py::arg("password"), 
             "验证用户登录 (高强度防爆破)")
        .def("registerUser", &Database::registerUser, 
             py::arg("username"), py::arg("password"), 
             "注册新用户 (PBKDF2 动态盐值加密)")
        .def("changePassword", &Database::changePassword, 
             py::arg("username"), py::arg("oldpw"), py::arg("newpw"), 
             "修改用户密码");
}
