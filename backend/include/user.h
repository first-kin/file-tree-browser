#ifndef USER_H
#define USER_H

#include <string>
#include <ctime>

using namespace std;

// user.h —— 用户信息与角色定义（接口冻结项）
enum class Role { GUEST, USER, ADMIN };

struct User
{
    string username;       // 用户名（主键，唯一）
    string passwordHash;   // SHA256(password + salt)，非明文
    string salt;           // 随机盐值
    Role role;             // 角色
    bool enabled;          // 是否启用
    int failedLoginCount;  // 连续登录失败次数
    time_t lockedUntil;    // 锁定截止时间（0=未锁定）
    time_t createdAt;      // 创建时间
    time_t lastLoginAt;    // 最后登录时间

    User()
        : role(Role::GUEST)
        , enabled(true)
        , failedLoginCount(0)
        , lockedUntil(0)
        , createdAt(0)
        , lastLoginAt(0)
    {
    }
};

#endif // USER_H
