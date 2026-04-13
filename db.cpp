#include "db.h"
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <random>
#include <openssl/evp.h>
#include <ctime>
#include <memory> //For smart pointers
#include <mysql/mysql.h>

//Internal tool: RAII automatically manages MYSQL Statement resources
struct StmtDeleter {
    void operator()(MYSQL_STMT* stmt) const {
        if (stmt) {
            mysql_stmt_close(stmt);
        }
    }
};

//Aliases(别名）of smart pointers
using ScopedStmt = std::unique_ptr<MYSQL_STMT, StmtDeleter>;
  

//  构造函数：当 Database 对象诞生时，自动连接数据库

Database::Database(){
	// 注意：这里直接使用类的成员变量 conn
	conn = mysql_init(nullptr);
	if(!conn){
		std::cerr << "mysql_init failed\n";
		return;
	}

	if(!mysql_real_connect(conn,"host.docker.internal","root","060410","login_db",0,nullptr,0)){
		std::cerr << "connect failed:" << mysql_error(conn) << "\n";
		mysql_close(conn);
                conn = nullptr; // 连接失败时，把指针设为空
    }
}

//  析构函数：当 Database 对象生命周期结束时，自动断开连接

Database::~Database(){
	if(conn){
		mysql_close(conn);
		std::cout << "[Debug] Database The base has been destroyed,  the database connection has been automatically and securely closed.\n";
	}
}

//  辅助函数：用来检查一开始有没有连上数据库

bool Database::isConnected() const {
    return conn != nullptr;
}

std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.size(),
           hash);

    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

    return ss.str();
}

std::string pbkdf2_sha256_hex(const std::string& password,
                              const std::string& salt,
                              int iter,
                              int dkLenBytes = 32)
{
    unsigned char out[64]; // 足够大，dkLenBytes <= 64
    if (dkLenBytes > 64) dkLenBytes = 64;

    int ok = PKCS5_PBKDF2_HMAC(
        password.c_str(), (int)password.size(),
        (const unsigned char*)salt.c_str(), (int)salt.size(),
        iter,
        EVP_sha256(),
        dkLenBytes,
        out
    );

    if (ok != 1) return "";

    std::ostringstream ss;
    for (int i = 0; i < dkLenBytes; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)out[i];
    return ss.str();
}

bool Database::isLockedNow(const MYSQL_TIME& t) {
    // 如果 lock_until 是 NULL，MySQL 通常不会写入这个结构（需要 is_null 标记）
    // 我们用 is_null 来判断更可靠（见 outbind[4].is_null）
    // 这里留空，实际判断在主逻辑里用 is_null 指针
    (void)t;
    return false;
}

time_t Database::mysqlTimeToEpoch(const MYSQL_TIME& mt) {
    std::tm tmv{};
    tmv.tm_year = mt.year - 1900;
    tmv.tm_mon  = mt.month - 1;
    tmv.tm_mday = mt.day;
    tmv.tm_hour = mt.hour;
    tmv.tm_min  = mt.minute;
    tmv.tm_sec  = mt.second;
    tmv.tm_isdst = -1;
    return std::mktime(&tmv); // 本地时区；对“还剩几秒”足够用
}

bool Database::resetLockState(const std::string& username) {
    const char* sql = "UPDATE user SET fail_count=0, lock_until=NULL WHERE username=?";
    ScopedStmt stmt(mysql_stmt_init(conn));
    if (!stmt) return false;
    if (mysql_stmt_prepare(stmt.get(), sql, (unsigned long)strlen(sql)) != 0) {
        return false;
    }

    MYSQL_BIND b[1];
    memset(b, 0, sizeof(b));
    unsigned long ulen = (unsigned long)username.size();
    b[0].buffer_type = MYSQL_TYPE_STRING;
    b[0].buffer = (void*)username.c_str();
    b[0].buffer_length = ulen;
    b[0].length = &ulen;

    bool ok = (mysql_stmt_bind_param(stmt.get(), b) == 0) && (mysql_stmt_execute(stmt.get()) == 0);
    return ok;
}

bool Database::recordFailure(const std::string& username,
                          int newFailCount, bool lockNow)
{
    // lockNow=true → lock_until = NOW() + INTERVAL 10 MINUTE
    const char* sql_lock =
        "UPDATE user SET fail_count=?, lock_until=DATE_ADD(NOW(), INTERVAL 10 MINUTE) WHERE username=?";
    const char* sql_nolock =
        "UPDATE user SET fail_count=? WHERE username=?";

    const char* sql = lockNow ? sql_lock : sql_nolock;

    ScopedStmt stmt(mysql_stmt_init(conn));
    if (!stmt) return false;
    if (mysql_stmt_prepare(stmt.get(), sql, (unsigned long)strlen(sql)) != 0) {
        return false;
    }

    MYSQL_BIND b[2];
    memset(b, 0, sizeof(b));

    b[0].buffer_type = MYSQL_TYPE_LONG;
    b[0].buffer = &newFailCount;

    unsigned long ulen = (unsigned long)username.size();
    b[1].buffer_type = MYSQL_TYPE_STRING;
    b[1].buffer = (void*)username.c_str();
    b[1].buffer_length = ulen;
    b[1].length = &ulen;

    bool ok = (mysql_stmt_bind_param(stmt.get(), b) == 0) && (mysql_stmt_execute(stmt.get()) == 0);
    return ok;
}


bool Database::checkLogin(
                const std::string& username,
                const std::string& password)
{
    const int MAX_FAIL = 5;

    const char* sql =
    "SELECT password, salt, iter, fail_count, lock_until "
    "FROM user WHERE username=? LIMIT 1";

    ScopedStmt stmt(mysql_stmt_init(conn));
    if (!stmt) {
        std::cerr << "stmt_init failed\n";
        return false;
    }

    if (mysql_stmt_prepare(stmt.get(), sql, (unsigned long)strlen(sql)) != 0) {
        std::cerr << "stmt_prepare failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    //---- bind input: username ----
    MYSQL_BIND inbind[1];
    memset(inbind, 0, sizeof(inbind));

    unsigned long ulen = (unsigned long)username.size();

    inbind[0].buffer_type = MYSQL_TYPE_STRING;
    inbind[0].buffer = (void*)username.c_str();
    inbind[0].buffer_length = ulen;
    inbind[0].length = &ulen;

    if (mysql_stmt_bind_param(stmt.get(), inbind) != 0) {
        std::cerr << "bind_param failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    if (mysql_stmt_execute(stmt.get()) != 0) {
        std::cerr << "execute failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

      // 绑定输出结果：password_hash + salt
    char db_hash[129] = {0};  // 128 + '\0'
    char db_salt[33]  = {0};  // 32 + '\0'
    int  db_iter      = 0;
    int  db_fail      = 0;

    unsigned long hash_len = 0;
    unsigned long salt_len = 0;

    MYSQL_TIME db_lock{};
    bool lock_is_null = false;
    unsigned long lock_len = 0;


    MYSQL_BIND outbind[5];
    memset(outbind, 0, sizeof(outbind));

    outbind[0].buffer_type = MYSQL_TYPE_STRING;
    outbind[0].buffer = db_hash;
    outbind[0].buffer_length = sizeof(db_hash);
    outbind[0].length = &hash_len;

    outbind[1].buffer_type = MYSQL_TYPE_STRING;
    outbind[1].buffer = db_salt;
    outbind[1].buffer_length = sizeof(db_salt);
    outbind[1].length = &salt_len;

    outbind[2].buffer_type   = MYSQL_TYPE_LONG;
    outbind[2].buffer        = &db_iter;

    outbind[3].buffer_type = MYSQL_TYPE_LONG;
    outbind[3].buffer = &db_fail;

    outbind[4].buffer_type = MYSQL_TYPE_DATETIME;
    outbind[4].buffer = &db_lock;
    outbind[4].buffer_length = sizeof(db_lock);
    outbind[4].is_null = &lock_is_null;
    outbind[4].length = &lock_len;

    // ---- 绑定输出列：password_hash + salt ----
    if (mysql_stmt_bind_result(stmt.get(), outbind) != 0) {
    std::cerr << "bind_result failed: " << mysql_stmt_error(stmt.get()) << "\n";
    return false;
    }

    // 把服务器结果拷贝到客户端缓冲区
    if (mysql_stmt_store_result(stmt.get()) != 0) {
    std::cerr << "store_result failed: " << mysql_stmt_error(stmt.get()) << "\n";
    return false;
    }

    // 取一行    
    int fetch_ret = mysql_stmt_fetch(stmt.get());
    
    // 如果是严重的数据库底层错误，直接退出（黑客很难利用数据库崩溃来计时）
    if (fetch_ret != 0 && fetch_ret != MYSQL_NO_DATA && fetch_ret != MYSQL_DATA_TRUNCATED) {
        return false;
    }
    // 1. 标记用户到底存不存在
    bool user_exists = (fetch_ret != MYSQL_NO_DATA);

    // 2. 准备哈希计算材料（真实材料 or 虚假材料）
    std::string target_salt;
    int target_iter = 100000; // 你系统默认的迭代次数
    
    if (user_exists) {
        target_salt = std::string(db_salt, salt_len);
        target_iter = db_iter;
    } else {
        // 如果用户不存在，我们掏出一个 32 字节的“假盐值”来演戏
        target_salt = "00000000000000000000000000000000";
    }

    // 3. 🔥 防御核心：无论用户存不存在，都给我死死地算够这 10 万次！
    // 这一步是整个系统的性能瓶颈，也是我们掩盖真相的“时间烟雾弹”
    std::string calc = pbkdf2_sha256_hex(password, target_salt, target_iter, 32);

    // 4. 算完了。如果刚才发现用户其实不存在，现在就可以安全地（慢吞吞地）返回失败了
    if (!user_exists) {
        return false;
    }
    
    // ---- lock check ----
    if (!lock_is_null) {
        time_t until = mysqlTimeToEpoch(db_lock);
        time_t now = std::time(nullptr);
        if (until > now) {
            int left = (int)(until - now);
            std::cerr << "Account locked. Try again in " << left << " seconds.\n";
            return false;
        }
    }

    // ---- verify password ----
    std::string hash_in_db(db_hash, hash_len);
    bool ok = (!calc.empty() && calc == hash_in_db);

    if (ok) {
        // success: reset fail_count/lock_until
        resetLockState(username);
        return true;
    } else {
        // failure: increment fail_count; lock if threshold reached
        int newFail = db_fail + 1;
        bool lockNow = (newFail >= MAX_FAIL);
        recordFailure(username, newFail, lockNow);
        return false;
    }
}


std::string randomSaltHex16() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dist(0, 255);

    unsigned char bytes[16];
    for (int i = 0; i < 16; i++) bytes[i] = (unsigned char)dist(gen);

    std::ostringstream ss;
    for (int i = 0; i < 16; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i];

    return ss.str(); // 32 hex chars
}


bool Database:: changePassword(
                    const std::string& username,
                    const std::string& oldPassword,
                    const std::string& newPassword)
{
    // 1) 先验证旧密码（复用你现有 checkLogin：它包含锁定判断+失败计数）
    if (!checkLogin(username, oldPassword)) {
        std::cerr << "change password failed: old password incorrect (or account locked)\n";
        return false;
    }

    // 2) 生成新 salt + 新 hash
    int iter = 100000; // 你也可以选择沿用数据库里的 iter，这里先固定
    std::string salt = randomSaltHex16();
    std::string hashed = pbkdf2_sha256_hex(newPassword, salt, iter, 32);
    if (hashed.empty()) {
        std::cerr << "change password failed: pbkdf2 error\n";
        return false;
    }

    // 3) UPDATE：写入新 hash/salt/iter，并清空 fail_count/lock_until
    const char* sql =
        "UPDATE user "
        "SET password=?, salt=?, iter=?, fail_count=0, lock_until=NULL "
        "WHERE username=?";

    ScopedStmt stmt(mysql_stmt_init(conn));
    if (!stmt) {
        std::cerr << "stmt_init failed\n";
        return false;
    }

    if (mysql_stmt_prepare(stmt.get(), sql, (unsigned long)strlen(sql)) != 0) {
        std::cerr << "prepare failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    MYSQL_BIND b[4];
    memset(b, 0, sizeof(b));

    unsigned long hlen = (unsigned long)hashed.size();
    unsigned long slen = (unsigned long)salt.size();
    unsigned long ulen = (unsigned long)username.size();

    b[0].buffer_type = MYSQL_TYPE_STRING;
    b[0].buffer = (void*)hashed.c_str();
    b[0].buffer_length = hlen;
    b[0].length = &hlen;

    b[1].buffer_type = MYSQL_TYPE_STRING;
    b[1].buffer = (void*)salt.c_str();
    b[1].buffer_length = slen;
    b[1].length = &slen;

    b[2].buffer_type = MYSQL_TYPE_LONG;
    b[2].buffer = &iter;

    b[3].buffer_type = MYSQL_TYPE_STRING;
    b[3].buffer = (void*)username.c_str();
    b[3].buffer_length = ulen;
    b[3].length = &ulen;

    if (mysql_stmt_bind_param(stmt.get(), b) != 0) {
        std::cerr << "bind_param failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    if (mysql_stmt_execute(stmt.get()) != 0) {
        std::cerr << "execute failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    // 可选：确认确实更新到 1 行
    my_ulonglong changed = mysql_stmt_affected_rows(stmt.get());

    return changed == 1;
}


bool Database::userExists(const std::string& username) {
    const char* sql = "SELECT 1 FROM user WHERE username=? LIMIT 1";

    //Using smart pointers to manage resources
    ScopedStmt stmt(mysql_stmt_init(conn));
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt.get(), sql, (unsigned long)strlen(sql)) != 0) {
        std::cerr << "exists prepare failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    unsigned long ulen = (unsigned long)username.size();
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)username.c_str();
    bind[0].buffer_length = ulen;
    bind[0].length = &ulen;

    if (mysql_stmt_bind_param(stmt.get(), bind) != 0) {
        std::cerr << "exists bind failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    if (mysql_stmt_execute(stmt.get()) != 0) {
        std::cerr << "exists exec failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    if (mysql_stmt_store_result(stmt.get()) != 0) {
        std::cerr << "exists store_result failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    bool exists = mysql_stmt_num_rows(stmt.get()) > 0;
    mysql_stmt_free_result(stmt.get());

    return exists;
}


bool Database::registerUser(
                  const std::string& username,
                  const std::string& password)
{
    if (userExists(username)) {
        std::cerr << "register failed: username already exists\n";
        return false;
    }

    int iter = 100000;
    std::string salt = randomSaltHex16();
    std::string hashed = pbkdf2_sha256_hex(password, salt, iter, 32);


    const char* sql = "INSERT INTO user (username, password, salt, iter) VALUES (?, ?, ?, ?)";

    ScopedStmt stmt(mysql_stmt_init(conn));
    if (!stmt.get()) {
        std::cerr << "stmt_init failed\n";
        return false;
    }

    if (mysql_stmt_prepare(stmt.get(), sql, (unsigned long)strlen(sql)) != 0) {
        std::cerr << "insert prepare failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));

    unsigned long ulen = (unsigned long)username.size();
    unsigned long plen = (unsigned long)hashed.size();
    unsigned long slen = (unsigned long)salt.size();

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)username.c_str();
    bind[0].buffer_length = ulen;
    bind[0].length = &ulen;

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void*)hashed.c_str();
    bind[1].buffer_length = plen;
    bind[1].length = &plen;

    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (void*)salt.c_str();
    bind[2].buffer_length = slen;
    bind[2].length = &slen;

    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = (void*)&iter;

    if (mysql_stmt_bind_param(stmt.get(), bind) != 0) {
        std::cerr << "insert bind failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    if (mysql_stmt_execute(stmt.get()) != 0) {
        std::cerr << "insert exec failed: " << mysql_stmt_error(stmt.get()) << "\n";
        return false;
    }

    return true;
}


