#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "../../backend/include/user.h"
#include "../../backend/include/tree_node.h"
#include "../../3rdparty/picosha2.h"
#include <string>
#include <unordered_map>
#include <ctime>
#include <cstdlib>

using namespace std;

// session_manager.h —— 会话管理与权限校验
// 每个登录用户对应一个 Session，包含用户名、角色、过期时间、目录树根节点
struct Session
{
    string username;      // 用户名
    Role role;            // 角色
    time_t expireAt;      // 过期时间戳
    TreeNode* treeRoot;   // 该用户的目录树根节点

    Session()
        : role(Role::GUEST)
        , expireAt(0)
        , treeRoot(nullptr)
    {
    }
};

class SessionManager
{
public:
    // 功能：获取单例实例
    // 参数：无
    // 返回值：SessionManager 单例引用
    static SessionManager& instance()
    {
        static SessionManager inst;
        return inst;
    }

    // 功能：登录成功后创建会话，生成 token
    // 参数：username-用户名，role-角色
    // 返回值：token 字符串
    string createSession(const string& username, Role role)
    {
        string raw = username + to_string(time(nullptr)) + to_string(rand());
        string token = picosha2::hash256_hex_string(raw);

        Session s;
        s.username = username;
        s.role = role;
        s.expireAt = time(nullptr) + SESSION_TIMEOUT_SECONDS;
        s.treeRoot = nullptr;

        sessions[token] = s;
        return token;
    }

    // 功能：根据 token 获取会话，过期自动清理
    // 参数：token-会话令牌
    // 返回值：会话指针，未找到或过期返回 nullptr
    Session* getSession(const string& token)
    {
        auto it = sessions.find(token);
        if (it == sessions.end())
        {
            return nullptr;
        }
        if (time(nullptr) > it->second.expireAt)
        {
            sessions.erase(it);
            return nullptr;
        }
        return &it->second;
    }

    // 功能：移除会话（登出）
    // 参数：token-会话令牌
    // 返回值：无
    void removeSession(const string& token)
    {
        sessions.erase(token);
    }

    // 功能：校验当前用户是否拥有指定角色及以上权限
    // 参数：token-会话令牌，required-要求的最低角色
    // 返回值：true 有权限，false 无权限或未登录
    bool checkPermission(const string& token, Role required)
    {
        Session* s = getSession(token);
        if (s == nullptr)
        {
            return false;
        }
        return static_cast<int>(s->role) >= static_cast<int>(required);
    }

    // 功能：获取当前会话的目录树根节点
    // 参数：token-会话令牌
    // 返回值：TreeNode* 根节点指针，未登录返回 nullptr
    TreeNode* getTreeRoot(const string& token)
    {
        Session* s = getSession(token);
        if (s == nullptr)
        {
            return nullptr;
        }
        return s->treeRoot;
    }

    // 功能：设置当前会话的目录树根节点（建树后调用）
    // 参数：token-会话令牌，root-目录树根节点
    // 返回值：无
    void setTreeRoot(const string& token, TreeNode* root)
    {
        Session* s = getSession(token);
        if (s != nullptr)
        {
            s->treeRoot = root;
        }
    }

private:
    SessionManager() = default;
    unordered_map<string, Session> sessions;
    static const int SESSION_TIMEOUT_SECONDS = 30 * 60; // 30 分钟，与 config.json 一致
};

#endif // SESSION_MANAGER_H
