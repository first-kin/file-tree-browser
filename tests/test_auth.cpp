// test_auth.cpp —— 认证模块独立单元测试（纯 C++，不依赖 Drogon）
// 注意：本文件含独立 main，需单独编译运行；若把 tests/*.cpp 一次性 GLOB 进同一个
// 可执行目标，会与 test_operations.cpp 的 main 冲突，建议每个测试文件建独立目标。

#include "../backend/include/auth.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace std;

// 断言失败时打印模块名、原因与行号，并在 NDEBUG 下也能终止进程
#define CHECK(condition, module, reason)                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            cerr << "[" << module << "] " << reason                             \
                 << " (line " << __LINE__ << ")" << endl;                       \
            assert(condition);                                                  \
            abort();                                                            \
        }                                                                       \
    } while (0)

int main()
{
    // 1. 创建 admin 账号（auth.h 未提供 initAdmin，用注册接口创建测试账号）
    CHECK(registerUser("admin", "123456") == 0,
          "test_auth", "admin 账号创建失败");

    // 2. 正确密码登录，并校验盐值与密码哈希
    CHECK(login("admin", "123456") == 0,
          "test_auth", "admin 使用正确密码登录应返回 0");

    User* user = currentUser();
    CHECK(user != nullptr,
          "test_auth", "登录成功后 currentUser 不应为空");
    CHECK(user->username == "admin",
          "test_auth", "当前登录用户名应为 admin");
    CHECK(user->passwordHash != "123456",
          "test_auth", "密码不应以明文形式保存");
    CHECK(user->salt.size() == 32,
          "test_auth", "盐值应为 16 字节随机数的 32 位十六进制");
    logout();

    // 3. 连续 3 次错误密码触发锁定；锁定期间即使密码正确也应拒绝
    CHECK(login("admin", "wrong1") == -4,
          "test_auth", "第 1 次错误密码应返回 -4");
    CHECK(login("admin", "wrong2") == -4,
          "test_auth", "第 2 次错误密码应返回 -4");
    CHECK(login("admin", "wrong3") == -5,
          "test_auth", "第 3 次错误密码应触发锁定并返回 -5");
    CHECK(login("admin", "123456") == -5,
          "test_auth", "锁定期间即使密码正确也应返回 -5");

    // 4. 注册新用户；重名注册应返回 -7
    CHECK(registerUser("alice", "abcdef") == 0,
          "test_auth", "新用户 alice 注册应成功");
    CHECK(registerUser("alice", "abcdef") == -7,
          "test_auth", "重名注册应返回 -7");

    cout << "test_auth passed!" << endl;
    return 0;
}
