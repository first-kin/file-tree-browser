#include "../include/auth.h"

#include "../../3rdparty/picosha2.h"

#include <cerrno>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <map>
#include <mutex>
#include <random>
#include <vector>

using namespace std;

// ===== 模块内部状态 =====

// 用户表：username -> User，std::map 保证元素地址稳定且写盘顺序确定
static map<string, User> g_users;

// 进程级当前登录用户，指向 g_users 中的元素；nullptr 表示未登录
static User* g_currentUser = nullptr;

// 单机版同时只允许一个账号登录；仍用互斥量保护静态用户表，避免同一账号并发请求读写竞争
static mutex g_mutex;

// 登录失败锁定策略：连续失败 3 次，锁定 5 分钟
static const int MAX_FAILED_LOGIN = 3;
static const int LOCK_SECONDS = 5 * 60;

// 内置管理员账号（五个文档要求：admin / 123456）
static const char* BUILTIN_ADMIN_NAME = "admin";
static const char* BUILTIN_ADMIN_PASSWORD = "123456";

// 用户表文件字段数：
// username|passwordHash|salt|role|enabled|failedLoginCount|lockedUntil|createdAt|lastLoginAt
static const size_t USER_FIELD_COUNT = 9;

// ===== 内部辅助函数 =====

// 生成 16 字节随机盐，以 32 位十六进制字符串返回
static string generateSalt()
{
    static const char* kHex = "0123456789abcdef";
    random_device rd;
    string salt;
    salt.reserve(32);
    for (int i = 0; i < 16; i++)
    {
        unsigned int byte = rd() & 0xFFu;
        salt.push_back(kHex[byte >> 4]);
        salt.push_back(kHex[byte & 0x0Fu]);
    }
    return salt;
}

// 计算 SHA256(password + salt) 的十六进制字符串
static string hashPassword(const string& password, const string& salt)
{
    return picosha2::hash256_hex_string(password + salt);
}

// 校验用户名：非空、长度不超过 32、不含分隔符与空白/控制字符
static bool isValidUsername(const string& username)
{
    if (username.empty() || username.size() > 32)
    {
        return false;
    }
    for (char c : username)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        if (c == '|' || uc < 0x20 || uc == 0x7F || c == ' ')
        {
            return false;
        }
    }
    return true;
}

// 校验明文密码：非空且长度不超过 128
static bool isValidPassword(const string& password)
{
    return !password.empty() && password.size() <= 128;
}

// 按 '|' 拆分一行用户记录
static vector<string> splitByPipe(const string& line)
{
    vector<string> fields;
    size_t start = 0;
    while (true)
    {
        size_t pos = line.find('|', start);
        if (pos == string::npos)
        {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    return fields;
}

// 安全解析 int（整段必须是合法数字）
static bool parseInt(const string& text, int& value)
{
    try
    {
        size_t used = 0;
        int parsed = stoi(text, &used);
        if (used != text.size())
        {
            return false;
        }
        value = parsed;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// 安全解析 long long（用于时间戳）
static bool parseLongLong(const string& text, long long& value)
{
    try
    {
        size_t used = 0;
        long long parsed = stoll(text, &used);
        if (used != text.size())
        {
            return false;
        }
        value = parsed;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// AI生成，已手动审查修改：固定管理员只补种账号/角色，不覆盖用户已修改的密码
// 功能：确保内置管理员存在且可用（调用方必须已持有 g_mutex）
// 参数：无
// 返回值：无
static void ensureBuiltinAdminNoLock()
{
    auto existing = g_users.find(BUILTIN_ADMIN_NAME);
    if (existing != g_users.end())
    {
        // 内置管理员必须始终可用：角色被改回 ADMIN、被禁用时重新启用
        existing->second.role = Role::ADMIN;
        existing->second.enabled = true;
        return;
    }

    User admin;
    admin.username = BUILTIN_ADMIN_NAME;
    admin.salt = generateSalt();
    admin.passwordHash = hashPassword(BUILTIN_ADMIN_PASSWORD, admin.salt);
    admin.role = Role::ADMIN;
    admin.enabled = true;
    admin.createdAt = time(nullptr);
    g_users[admin.username] = admin;
}

// ===== 对外接口实现 =====

// 用户注册：默认角色 USER，密码仅保存加盐哈希
int registerUser(const string& username, const string& password)
{
    lock_guard<mutex> lock(g_mutex);
    ensureBuiltinAdminNoLock();

    if (!isValidUsername(username) || !isValidPassword(password))
    {
        return -12;
    }
    if (g_users.find(username) != g_users.end())
    {
        return -7;
    }

    User user;
    user.username = username;
    user.salt = generateSalt();
    user.passwordHash = hashPassword(password, user.salt);
    user.role = Role::USER;
    user.enabled = true;
    user.createdAt = time(nullptr);

    g_users[username] = user;
    return 0;
}

// 登录校验：禁用 -> 锁定 -> 密码比对，失败累计达阈值即锁定
int login(const string& username, const string& password)
{
    lock_guard<mutex> lock(g_mutex);
    ensureBuiltinAdminNoLock();

    if (username.empty() || password.empty())
    {
        return -12;
    }

    auto it = g_users.find(username);
    if (it == g_users.end())
    {
        return -4;
    }
    User& user = it->second;

    if (!user.enabled)
    {
        return -6;
    }

    time_t now = time(nullptr);
    if (user.lockedUntil != 0 && now < user.lockedUntil)
    {
        return -5;
    }
    if (user.lockedUntil != 0)
    {
        // 锁定期已结束，恢复为全新的失败计数
        user.lockedUntil = 0;
        user.failedLoginCount = 0;
    }

    if (hashPassword(password, user.salt) != user.passwordHash)
    {
        user.failedLoginCount++;
        if (user.failedLoginCount >= MAX_FAILED_LOGIN)
        {
            user.lockedUntil = now + LOCK_SECONDS;
            return -5;
        }
        return -4;
    }

    user.failedLoginCount = 0;
    user.lockedUntil = 0;
    user.lastLoginAt = now;
    g_currentUser = &user;
    return 0;
}

// 登出：清除进程级当前用户
void logout()
{
    lock_guard<mutex> lock(g_mutex);
    g_currentUser = nullptr;
}

// 获取当前登录用户
User* currentUser()
{
    lock_guard<mutex> lock(g_mutex);
    return g_currentUser;
}

// 获取全部用户快照：map 迭代本身按 username 升序
vector<User> getAllUsers()
{
    lock_guard<mutex> lock(g_mutex);
    ensureBuiltinAdminNoLock();

    vector<User> users;
    users.reserve(g_users.size());
    for (const auto& entry : g_users)
    {
        users.push_back(entry.second);
    }
    return users;
}

// 角色比较：Role 枚举值按 GUEST < USER < ADMIN 递增
bool hasPermission(Role required)
{
    lock_guard<mutex> lock(g_mutex);
    ensureBuiltinAdminNoLock();

    Role current = (g_currentUser != nullptr) ? g_currentUser->role : Role::GUEST;
    return static_cast<int>(current) >= static_cast<int>(required);
}

// 修改当前用户密码：旧密码校验通过后重新加盐
int changePassword(const string& oldPwd, const string& newPwd)
{
    lock_guard<mutex> lock(g_mutex);

    if (g_currentUser == nullptr)
    {
        return -11;
    }
    if (!isValidPassword(newPwd))
    {
        return -12;
    }
    if (hashPassword(oldPwd, g_currentUser->salt) != g_currentUser->passwordHash)
    {
        return -4;
    }

    g_currentUser->salt = generateSalt();
    g_currentUser->passwordHash = hashPassword(newPwd, g_currentUser->salt);
    return 0;
}

// 重置指定用户密码：同时清除锁定状态，便于管理员解锁
int resetPassword(const string& username, const string& newPwd)
{
    lock_guard<mutex> lock(g_mutex);
    ensureBuiltinAdminNoLock();

    if (!isValidPassword(newPwd))
    {
        return -12;
    }

    auto it = g_users.find(username);
    if (it == g_users.end())
    {
        return -1;
    }

    User& user = it->second;
    user.salt = generateSalt();
    user.passwordHash = hashPassword(newPwd, user.salt);
    user.failedLoginCount = 0;
    user.lockedUntil = 0;
    return 0;
}

// 启用/禁用用户：禁用当前登录用户时同步登出；重新启用时清除历史锁定
int setUserEnabled(const string& username, bool enabled)
{
    lock_guard<mutex> lock(g_mutex);
    ensureBuiltinAdminNoLock();

    auto it = g_users.find(username);
    if (it == g_users.end())
    {
        return -1;
    }
    if (!enabled && username == BUILTIN_ADMIN_NAME)
    {
        // 内置管理员按文档要求不可禁用
        return -1;
    }

    it->second.enabled = enabled;
    if (enabled)
    {
        // 重新启用视为解除历史锁定，避免刚启用就仍被锁 5 分钟
        it->second.failedLoginCount = 0;
        it->second.lockedUntil = 0;
    }
    else if (g_currentUser == &it->second)
    {
        g_currentUser = nullptr;
    }
    return 0;
}

// 修改指定用户角色（管理员端点调用，权限校验在控制器）
int setUserRole(const string& username, Role role)
{
    lock_guard<mutex> lock(g_mutex);
    ensureBuiltinAdminNoLock();

    int roleValue = static_cast<int>(role);
    if (roleValue < static_cast<int>(Role::GUEST) || roleValue > static_cast<int>(Role::ADMIN))
    {
        return -12;
    }

    auto it = g_users.find(username);
    if (it == g_users.end())
    {
        return -1;
    }
    if (username == BUILTIN_ADMIN_NAME && role != Role::ADMIN)
    {
        // 内置管理员按文档要求不可降级
        return -1;
    }

    it->second.role = role;
    return 0;
}

// 加载用户表：文件不存在视为空表，损坏行跳过，不阻断服务启动
int loadUsers(const string& filePath)
{
    lock_guard<mutex> lock(g_mutex);
    g_users.clear();
    g_currentUser = nullptr;

    ifstream fin(filePath.c_str());
    if (!fin.is_open())
    {
        // 主用户表不存在时尝试读取上一次保存留下的 .bak
        fin.open((filePath + ".bak").c_str());
    }
    if (!fin.is_open())
    {
        // 首次运行没有用户表属于正常情况；其余打开失败按 I/O 错误返回
        FILE* probe = fopen(filePath.c_str(), "rb");
        if (probe == nullptr)
        {
            if (errno == ENOENT)
            {
                ensureBuiltinAdminNoLock();
                return 0;
            }
            return -10;
        }
        fclose(probe);
        return -10;
    }

    string line;
    while (getline(fin, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        vector<string> fields = splitByPipe(line);
        if (fields.size() != USER_FIELD_COUNT)
        {
            continue;
        }

        int roleValue = 0;
        int enabledValue = 0;
        int failedCount = 0;
        long long lockedUntil = 0;
        long long createdAt = 0;
        long long lastLoginAt = 0;
        if (!isValidUsername(fields[0]) || fields[1].empty() || fields[2].empty()
            || !parseInt(fields[3], roleValue) || roleValue < 0 || roleValue > 2
            || !parseInt(fields[4], enabledValue) || (enabledValue != 0 && enabledValue != 1)
            || !parseInt(fields[5], failedCount)
            || !parseLongLong(fields[6], lockedUntil)
            || !parseLongLong(fields[7], createdAt)
            || !parseLongLong(fields[8], lastLoginAt))
        {
            continue;
        }

        User user;
        user.username = fields[0];
        user.passwordHash = fields[1];
        user.salt = fields[2];
        user.role = static_cast<Role>(roleValue);
        user.enabled = (enabledValue == 1);
        user.failedLoginCount = failedCount;
        user.lockedUntil = static_cast<time_t>(lockedUntil);
        user.createdAt = static_cast<time_t>(createdAt);
        user.lastLoginAt = static_cast<time_t>(lastLoginAt);

        g_users[user.username] = user;
    }

    ensureBuiltinAdminNoLock();
    return 0;
}

// 保存用户表：首行为字段说明注释，时间戳统一以十进制整数存储
int saveUsers(const string& filePath)
{
    lock_guard<mutex> lock(g_mutex);
    ensureBuiltinAdminNoLock();

    // 先写临时文件，成功后再替换，避免写一半留下损坏的用户表
    const string tempPath = filePath + ".tmp";
    ofstream fout(tempPath.c_str(), ios::out | ios::trunc);
    if (!fout.is_open())
    {
        return -10;
    }

    fout << "# username|passwordHash|salt|role|enabled|failedLoginCount|lockedUntil|createdAt|lastLoginAt\n";
    for (const auto& entry : g_users)
    {
        const User& user = entry.second;
        fout << user.username << '|'
             << user.passwordHash << '|'
             << user.salt << '|'
             << static_cast<int>(user.role) << '|'
             << (user.enabled ? 1 : 0) << '|'
             << user.failedLoginCount << '|'
             << static_cast<long long>(user.lockedUntil) << '|'
             << static_cast<long long>(user.createdAt) << '|'
             << static_cast<long long>(user.lastLoginAt) << '\n';
        if (!fout)
        {
            fout.close();
            std::remove(tempPath.c_str());
            return -10;
        }
    }

    fout.close();
    if (!fout)
    {
        std::remove(tempPath.c_str());
        return -10;
    }

    // 先备份旧文件，再替换；替换失败时回滚备份，避免旧用户表被直接删掉
    const string backupPath = filePath + ".bak";
    std::remove(backupPath.c_str());
    bool hadOldFile = (std::rename(filePath.c_str(), backupPath.c_str()) == 0);
    if (!hadOldFile)
    {
        // 区分“本来就没有旧文件”和“旧文件存在但改名备份失败”
        FILE* probe = fopen(filePath.c_str(), "rb");
        if (probe != nullptr)
        {
            fclose(probe);
            std::remove(tempPath.c_str());
            return -10;
        }
    }

    if (std::rename(tempPath.c_str(), filePath.c_str()) != 0)
    {
        if (hadOldFile)
        {
            std::rename(backupPath.c_str(), filePath.c_str());
        }
        std::remove(tempPath.c_str());
        return -10;
    }

    return 0;
}
