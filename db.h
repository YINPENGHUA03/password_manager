#ifndef DB_H
#define DB_H

#include <string>
#include <ctime>
#include <mysql/mysql.h>


class Database{
	private:
	MYSQL* conn;// 数据库连接句柄现在是类的私有财产
        
	bool userExists(const std::string& username);

	bool resetLockState(const std::string& username);
        bool recordFailure(const std::string& username, int newFailCount, bool lockNow);
	
	time_t mysqlTimeToEpoch(const MYSQL_TIME& mt);
 	bool isLockedNow(const MYSQL_TIME& t);

	public:
	// 构造函数：负责初始化和连接数据库
	Database();

	// 析构函数：负责关闭数据库连接
	~Database();

	// 禁用拷贝构造和赋值（因为数据库连接是独占资源，不能被复制）
	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;
    
	// 检查是否连接成功
	bool isConnected() const;

	//核心业务接口（注意：这里的参数已经没有 MYSQL* conn 了！）
	bool checkLogin(
	const std::string& username,
        const std::string& password
);

	bool registerUser(
        const std::string& username,
	const std::string& password);

	bool changePassword(
        const std::string& username,
	const std::string& oldPassword,
	const std::string& newPassword);
};
#endif

