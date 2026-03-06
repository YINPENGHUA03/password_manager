#include "db.h"
#include <iostream>
#include <string>
#include <limits>
#include <cctype>
#include <openssl/crypto.h> // For OPENSSL_cleanse (Secure memory erasure / 安全内存擦除)

// ==============================================================================
// 1. Cross-Platform Headers Inclusion / 跨平台头文件引入
// ==============================================================================
#ifdef _WIN32
    // Windows-specific header for console I/O, providing _getch()
    // Windows 专属控制台输入输出头文件，提供无回显读取函数 _getch()
    #include <conio.h> 
#else
    // POSIX standard headers for terminal I/O interfaces and system calls
    // Linux/Unix 下用于终端底层控制 (termios) 和系统调用 (unistd) 的标准头文件
    #include <termios.h> 
    #include <unistd.h>
#endif

// ==============================================================================
// 2. Security Utilities / 安全辅助函数
// ==============================================================================

/**
 * Securely erases sensitive data from memory.
 * 安全擦除内存中的敏感数据。
 */
void secureErase(std::string& str) {
    if (!str.empty()) {
        // OPENSSL_cleanse guarantees that the memory is overwritten with zeros 
        // and prevents the compiler from optimizing it away (Dead Store Elimination).
        // OPENSSL_cleanse 包含内存屏障，强制用 0 覆盖内存，绝对不会被编译器因“死代码消除”优化掉。
        OPENSSL_cleanse(&str[0], str.capacity());
        str.clear();
    }
}

/**
 * Visually erases characters from the terminal.
 * 在终端视觉上擦除（退格）已输入的字符。
 */
static void eraseStars(size_t n) {
    for (size_t i = 0; i < n; i++) {
        // \b moves the cursor back, space overwrites the star, \b moves it back again
        // \b 将光标左移，空格覆盖掉原来的星号，\b 再次左移光标以等待下一次输入
        std::cout << "\b \b";
    }
    std::cout.flush(); // Force output to the screen immediately / 强制立刻刷新输出缓冲区
}

// ==============================================================================
// 3. Linux/Unix Specific Terminal Controls / Linux 平台专属底层终端控制
// ==============================================================================
#ifndef _WIN32

/**
 * RAII Wrapper for terminal settings. 
 * 基于 RAII 机制的终端设置保护器。确保异常退出时也能恢复终端原貌。
 */
struct TermiosGuard {
    termios oldt{};
    bool ok{false};

    // Save the current terminal settings upon initialization
    // 构造时获取并保存当前的终端底层设置
    TermiosGuard() { ok = (tcgetattr(STDIN_FILENO, &oldt) == 0); }

    void setNoEchoNoCanon() {
        if (!ok) return;
        termios newt = oldt;
        
        // Disable ECHO (typing won't show on screen) and ICANON (read byte-by-byte instantly)
        // 关闭回显 (ECHO)，关闭规范模式 (ICANON: 即不需要按回车就能逐个字节读取键盘输入)
        newt.c_lflag &= ~(ECHO | ICANON);  
        
        newt.c_cc[VMIN]  = 1; // Wait for at least 1 character / 至少读到 1 个字符才返回
        newt.c_cc[VTIME] = 0; // No timeout / 不设置读取超时
        
        // Apply the new settings immediately (TCSANOW)
        // 立即应用新设置
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }

    // Restore the original settings upon destruction (RAII)
    // 析构时雷打不动地恢复原本的终端设置
    ~TermiosGuard() {
        if (ok) tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
};

/**
 * Consumes escape sequences (e.g., arrow keys) to prevent garbage input.
 * 吞噬并丢弃转义序列（如方向键产生的乱码），防止它们被作为密码录入。
 */
static void swallowEscapeSequence() {
    unsigned char tmp = 0;
    // Arrow keys usually generate up to 4 bytes: ESC [ A/B/C/D
    // 方向键通常会产生连续的几个字节序列，我们最多向后读取并丢弃 4 个字节
    for (int i = 0; i < 4; i++) {
        ssize_t n = read(STDIN_FILENO, &tmp, 1);
        if (n <= 0) break;
        // Stop consuming when we hit a standard letter ending the sequence
        // 如果遇到字母，说明该转义序列已结束，停止吞噬
        if (tmp >= 'A' && tmp <= 'Z') break;
        if (tmp >= 'a' && tmp <= 'z') break;
    }
}
#endif // _WIN32 结束

// ==============================================================================
// 4. Cross-Platform Password Reader / 跨平台密码安全读取核心逻辑
// ==============================================================================

/**
 * Reads a password securely without echoing the plaintext.
 * 安全读取密码，屏蔽明文回显并支持复杂的终端快捷键交互。
 */
static std::string readPasswordStars(const std::string& prompt = "password: ",
                                     size_t maxLen = 128)
{
    std::cout << prompt;
    std::cout.flush();
    std::string pw;

#ifdef _WIN32
    // --------------------------------------------------------------------------
    // [Windows Implementation / Windows 平台实现]
    // --------------------------------------------------------------------------
    int ch;
    // _getch() reads a single character directly without echoing. 
    // Enter key maps to '\r' in Windows console.
    // _getch() 可直接无回显地读取单个按键。Windows 下回车键对应 '\r'。
    while ((ch = _getch()) != '\r') { 
        if (ch == '\b' || ch == 8) { 
            // Handle Backspace / 处理退格键
            if (!pw.empty()) {
                pw.pop_back();
                eraseStars(1);
            }
        } else if (ch == 3) { 
            // Handle Ctrl+C (Interrupt) / 处理 Ctrl+C 强制退出
            std::cout << "\n";
            pw.clear();
            break;
        } else if (ch == 0 || ch == 224) { 
            // Special keys (like arrows) in Windows generate two bytes, starting with 0 or 224.
            // 忽略 Windows 下的功能键（如方向键），它们会产生两个字节，第一个字节是 0 或 224
            _getch(); 
        } else if (ch >= 32) { 
            // Accept printable characters only / 过滤不可见控制字符，只接受可打印字符
            if (pw.size() < maxLen) {
                pw.push_back((char)ch);
                std::cout << '*';
                std::cout.flush();
            }
        }
    }
    std::cout << "\n";

#else
    // --------------------------------------------------------------------------
    // [Linux/Unix Implementation / Linux 平台实现]
    // --------------------------------------------------------------------------
    TermiosGuard tg;
    if (!tg.ok) {
        // Fallback to insecure cin if terminal control fails
        // 极小概率下：如果终端接管失败，退回到标准的不安全读取模式
        std::cin >> pw;
        return pw;
    }
    tg.setNoEchoNoCanon();

    while (true) {
        unsigned char ch = 0;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n <= 0) continue;

        // Enter key (Linux uses '\n') / 回车键
        if (ch == '\n' || ch == '\r') {
            std::cout << "\n";
            break;
        }
        
        // ESC (ASCII 27): Handle escape sequences like arrow keys
        // 处理 ESC 转义序列（通常是误触了方向键）
        if (ch == 27) {
            swallowEscapeSequence();
            continue;
        }
        
        // Ctrl+C (ASCII 3): Cancel input
        // Ctrl+C：取消当前输入
        if (ch == 3) {
            std::cout << "\n";
            pw.clear();
            break;
        }
        
        // Ctrl+U (ASCII 21): Clear entire line
        // Ctrl+U：一键清空整行输入的密码
        if (ch == 21) {
            eraseStars(pw.size());
            pw.clear();
            continue;
        }
        
        // Ctrl+W (ASCII 23): Delete the last "word"
        // Ctrl+W：删除最后一个连续的“单词”（包含先删尾部空格，再删非空格字符）
        if (ch == 23) {
            while (!pw.empty() && pw.back() == ' ') {
                pw.pop_back();
                eraseStars(1);
            }
            while (!pw.empty() && pw.back() != ' ') {
                pw.pop_back();
                eraseStars(1);
            }
            continue;
        }
        
        // Backspace (ASCII 127 or 8)
        // 退格键
        if (ch == 127 || ch == 8) {
            if (!pw.empty()) {
                pw.pop_back();
                eraseStars(1);
            }
            continue;
        }
        
        // Ignore other non-printable control characters
        // 过滤剩余的不可见控制字符
        if (ch < 32) continue;
        
        // Enforce maximum length
        // 强制密码长度限制
        if (pw.size() >= maxLen) continue;

        // Valid character: store it and echo a star
        // 录入合法字符，并在终端回显一颗星号
        pw.push_back((char)ch);
        std::cout << '*';
        std::cout.flush();
    }
#endif

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
