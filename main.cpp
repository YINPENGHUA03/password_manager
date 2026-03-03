#include "db.h"
#include <iostream>
#include <string>
#include <limits>
#include <termios.h>
#include <unistd.h>
#include <cctype>
#include <openssl/crypto.h>//安全内存擦除内存

void secureErase(std::string& str) {
    if (!str.empty()) {
        // OPENSSL_cleanse 强制用 0 覆盖内存，且绝对不会被编译器优化掉！
        // 我们不仅清理 size()，而是清理 capacity() 分配的所有底层空间
        OPENSSL_cleanse(&str[0], str.capacity());
        str.clear();
    }
}

struct TermiosGuard {
    termios oldt{};
    bool ok{false};

    TermiosGuard() { ok = (tcgetattr(STDIN_FILENO, &oldt) == 0); }

    void setNoEchoNoCanon() {
        if (!ok) return;
        termios newt = oldt;
        newt.c_lflag &= ~(ECHO | ICANON);   // 关闭回显 + 关闭规范模式（逐字符）
        newt.c_cc[VMIN]  = 1;              // 至少读到 1 个字符才返回
        newt.c_cc[VTIME] = 0;              // 不超时
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }

    ~TermiosGuard() {
        if (ok) tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
};

static void eraseStars(size_t n) {
    for (size_t i = 0; i < n; i++) {
        std::cout << "\b \b";
    }
    std::cout.flush();
}

static void swallowEscapeSequence() {
    // 方向键通常是：ESC [ A/B/C/D  或 ESC O A/B/C/D
    // 我们简单吞掉接下来最多 4 个字节，防止乱码进入密码
    unsigned char tmp = 0;
    for (int i = 0; i < 4; i++) {
        ssize_t n = read(STDIN_FILENO, &tmp, 1);
        if (n <= 0) break;
        // 如果不是 '['/'O' 或不是字母，我们也就差不多吞完了
        if (tmp >= 'A' && tmp <= 'Z') break;
        if (tmp >= 'a' && tmp <= 'z') break;
    }
}

static std::string readPasswordStars(const std::string& prompt = "password: ",
                                     size_t maxLen = 128)
{
    std::cout << prompt;
    std::cout.flush();

    TermiosGuard tg;
    if (!tg.ok) {
        std::string pw;
        std::cin >> pw;
        return pw;
    }
    tg.setNoEchoNoCanon();

    std::string pw;
    while (true) {
        unsigned char ch = 0;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n <= 0) continue;

        // Enter
        if (ch == '\n' || ch == '\r') {
            std::cout << "\n";
            break;
        }

        // ESC：吞掉方向键等转义序列
        if (ch == 27) {
            swallowEscapeSequence();
            continue;
        }

        // Ctrl+C：取消（这里选择直接返回空串）
        if (ch == 3) {
            std::cout << "\n";
            pw.clear();
            break;
        }

        // Ctrl+U：清空整行
        if (ch == 21) {
            eraseStars(pw.size());
            pw.clear();
            continue;
        }

        // Ctrl+W：删除一个“单词”（先删尾部空格，再删连续非空格）
        if (ch == 23) {
            // 删尾部空格
            while (!pw.empty() && pw.back() == ' ') {
                pw.pop_back();
                eraseStars(1);
            }
            // 删一个单词
            while (!pw.empty() && pw.back() != ' ') {
                pw.pop_back();
                eraseStars(1);
            }
            continue;
        }

        // Backspace
        if (ch == 127 || ch == 8) {
            if (!pw.empty()) {
                pw.pop_back();
                eraseStars(1);
            }
            continue;
        }

        // 过滤不可见控制字符
        if (ch < 32) continue;

        // 长度限制
        if (pw.size() >= maxLen) {
            // 可选：提示一下（这里不提示，避免泄露长度交互）
            continue;
        }

        pw.push_back((char)ch);
        std::cout << '*';
        std::cout.flush();
    }

    return pw;
}


struct PwStrength {
    int score;               // 0-100
    std::string level;       // Weak/Medium/Strong
    std::string feedback;    // 建议
};

static PwStrength evaluatePasswordStrength(const std::string& pw) {
    PwStrength r{0, "Weak", ""};

    if (pw.empty()) {
        r.feedback = "Password is empty.";
        return r;
    }

    bool hasLower = false, hasUpper = false, hasDigit = false, hasSymbol = false;
    for (unsigned char c : pw) {
        if (std::islower(c)) hasLower = true;
        else if (std::isupper(c)) hasUpper = true;
        else if (std::isdigit(c)) hasDigit = true;
        else hasSymbol = true;
    }

    int len = (int)pw.size();
    int types = (hasLower?1:0) + (hasUpper?1:0) + (hasDigit?1:0) + (hasSymbol?1:0);

    // 基础分：长度
    int score = 0;
    if (len >= 8)  score += 20;
    if (len >= 12) score += 20;
    if (len >= 16) score += 10;

    // 类型分：字符种类
    score += types * 10; // 最多 +40

    // 常见弱点惩罚
    auto hasRepeat = [&]() {
        // 连续3个相同字符：aaa / 111
        for (int i = 2; i < len; i++) {
            if (pw[i] == pw[i-1] && pw[i-1] == pw[i-2]) return true;
        }
        return false;
    };

    auto isVeryCommon = [&]() {
        // 简单演示：常见密码/模式（你也可以扩展）
        static const char* common[] = {
            "123456", "12345678", "password", "qwerty", "111111", "000000",
            "abc123", "iloveyou", "admin", "letmein"
        };
        for (auto s : common) {
            if (pw == s) return true;
        }
        return false;
    };

    if (isVeryCommon()) score -= 50;
    if (len <= 6) score -= 20;
    if (types <= 1) score -= 20;
    if (hasRepeat()) score -= 10;

    // 夹紧到 0-100
    if (score < 0) score = 0;
    if (score > 100) score = 100;

    r.score = score;

    if (score >= 75) r.level = "Strong";
    else if (score >= 45) r.level = "Medium";
    else r.level = "Weak";

    // 反馈
    std::string fb;
    if (len < 12) fb += "- Use 12+ characters.\n";
    if (!hasUpper) fb += "- Add uppercase letters.\n";
    if (!hasLower) fb += "- Add lowercase letters.\n";
    if (!hasDigit) fb += "- Add digits.\n";
    if (!hasSymbol) fb += "- Add symbols (e.g. !@#$).\n";
    if (isVeryCommon()) fb += "- Avoid common passwords.\n";
    if (hasRepeat()) fb += "- Avoid repeated characters (e.g. aaa/111).\n";
    if (fb.empty()) fb = "Good password habits.";

    r.feedback = fb;
    return r;
}

int main() {
    
    Database db;

    if (!db.isConnected()) {
    return 1;
}
    int choice;

    // 1) 先把 choice 读对（只接受 1 或 2）
    while (true) {
        std::cout << "1. Login\n2. Register\n3. Change Password\nChoose: ";
        if (std::cin >> choice && (choice == 1 || choice == 2 || choice == 3)) break;

        std::cin.clear(); // 清掉 fail 状态
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 丢掉本行剩余输入
        std::cout << "Invalid input. Please enter 1 or 2.\n";
    }

    if (choice == 1) {  // Login
    std::string username, password;
    std::cout << "username:";
    std::cin  >> username;
    password = readPasswordStars("password:");

    if (db.checkLogin(username, password))
        std::cout << "login success\n";
    else
        std::cout << "login failed\n";

    secureErase(password);
}

    else if (choice == 2) {  // Register
    std::string username, password;
    std::cout << "username: ";
    std::cin >> username;
    password = readPasswordStars("password: ");

    auto s = evaluatePasswordStrength(password);
    std::cout << "Password strength: " << s.level
          << " (" << s.score << "/100)\n";
    std::cout << s.feedback;

    if (s.score < 45) {
    std::cout << "Password too weak. Register cancelled.\n";
    return 0; // 或 continue（如果你后面有循环）
}

    if (db.registerUser(username, password)){
        std::cout << "register success\n";
	} else{
        std::cout << "register failed\n";
	}
	secureErase(password);
}

    else if (choice == 3) {  // Change Password
    std::string username, oldpw, newpw, confirm;

    std::cout << "username: ";
    std::cin >> username;
    oldpw   = readPasswordStars("old password: ");
    newpw   = readPasswordStars("new password: ");
    confirm = readPasswordStars("confirm new password: ");

       if (newpw != confirm) {
        std::cout << "change password failed: new passwords do not match\n";
    } else {
        auto s = evaluatePasswordStrength(newpw);
        std::cout << "New password strength: " << s.level
              << " (" << s.score << "/100)\n";
        std::cout << s.feedback;

    if (s.score < 45) {
        std::cout << "Password too weak. Change cancelled.\n";
        return 0;
    }
        if (db.changePassword(username, oldpw, newpw))
            std::cout << "change password success\n";
        else
            std::cout << "change password failed\n";
	secureErase(oldpw);
        secureErase(newpw);
        secureErase(confirm);
    }
}

return 0;
}
