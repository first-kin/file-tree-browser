#include "../include/auth_controller.h"
#include "../../backend/include/auth.h"
#include "../../backend/include/operations.h"
#include "../../backend/include/user.h"
#include "../include/api_result.h"
#include "../include/session_manager.h"

#include <mutex>
#include <string>
#include <vector>

using namespace drogon;
using namespace std;

// ===== 内部辅助函数 =====

// 读取 JSON 字符串字段：字段必须存在且为字符串
static bool readStringField(const shared_ptr<Json::Value>& body, const char* key, string& value)
{
    if (body == nullptr || !body->isMember(key))
    {
        return false;
    }
    const Json::Value& field = (*body)[key];
    if (!field.isString())
    {
        return false;
    }
    value = field.asString();
    return true;
}

// 读取 JSON 布尔字段：字段必须存在且为 bool
static bool readBoolField(const shared_ptr<Json::Value>& body, const char* key, bool& value)
{
    if (body == nullptr || !body->isMember(key))
    {
        return false;
    }
    const Json::Value& field = (*body)[key];
    if (!field.isBool())
    {
        return false;
    }
    value = field.asBool();
    return true;
}

// 从请求头提取 Bearer token
static string extractToken(const HttpRequestPtr& req)
{
    string auth = req->getHeader("Authorization");
    if (auth.size() > 7 && auth.substr(0, 7) == "Bearer ")
    {
        return auth.substr(7);
    }
    return "";
}

// 把组长 api_result 产生的 nlohmann JSON 序列化为 Drogon 响应
// 说明：api_result.h 由组长维护并返回 nlohmann::json，这里不改变其结构，
// 只负责按 Drogon 的要求设置 JSON Content-Type 与响应体。
static HttpResponsePtr makeJsonResponse(const json& body)
{
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(body.dump());
    return resp;
}

// 管理员校验：0 通过，-11 未登录或会话失效，-9 已登录但非管理员
static int checkAdmin(const HttpRequestPtr& req)
{
    string token = extractToken(req);
    Session* session = SessionManager::instance().getSession(token);
    if (session == nullptr)
    {
        return -11;
    }
    if (session->role != Role::ADMIN)
    {
        return -9;
    }
    return 0;
}

// 角色枚举转前端字符串
static string roleToString(Role role)
{
    switch (role)
    {
    case Role::ADMIN:
        return "ADMIN";
    case Role::USER:
        return "USER";
    default:
        return "GUEST";
    }
}

// 前端字符串转角色枚举
static bool roleFromString(const string& text, Role& role)
{
    if (text == "ADMIN")
    {
        role = Role::ADMIN;
        return true;
    }
    if (text == "USER")
    {
        role = Role::USER;
        return true;
    }
    if (text == "GUEST")
    {
        role = Role::GUEST;
        return true;
    }
    return false;
}

// 登录错误码转中文提示
static string loginErrorMessage(int code)
{
    switch (code)
    {
    case -4:
        return "用户名或密码错误";
    case -5:
        return "账号已被锁定，请 5 分钟后重试";
    case -6:
        return "账号已被禁用，请联系管理员";
    case -12:
        return "用户名或密码不能为空";
    default:
        return "登录失败";
    }
}

// 注册错误码转中文提示
static string registerErrorMessage(int code)
{
    switch (code)
    {
    case -7:
        return "用户名已存在";
    case -12:
        return "用户名或密码格式非法";
    default:
        return "注册失败";
    }
}

// ===== 用户表持久化 =====

// 单机版同一时间只保留一个登录会话：记录当前 token，换账号登录时踢掉旧 token
static mutex g_activeSessionMutex;
static string g_activeToken;

// 用户表路径来自 server/config.json 的 custom_config.data_path
static string userFilePath()
{
    const Json::Value& config = drogon::app().getCustomConfig();
    if (config.isMember("data_path") && config["data_path"].isString())
    {
        return config["data_path"].asString();
    }
    return "../data/users.dat";
}

// 首次请求时加载用户表；加载失败不阻塞服务（内存中仍会补种内置管理员）
static void ensureUserStore()
{
    static once_flag loadFlag;
    call_once(loadFlag, []() {
        (void)loadUsers(userFilePath());
    });
}

// 保存用户表；失败返回 -10，由调用方转成统一响应
static int persistUsers()
{
    return saveUsers(userFilePath());
}

// ===== 接口实现 =====

// 登录：校验成功返回 token、用户名、角色与有效期
void AuthController::handleLogin(const HttpRequestPtr& req,
                                 function<void(const HttpResponsePtr&)>&& callback)
{
    ensureUserStore();

    auto body = req->getJsonObject();
    string username;
    string password;
    if (!readStringField(body, "username", username)
        || !readStringField(body, "password", password)
        || username.empty() || password.empty())
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 username 或 password 参数")));
        return;
    }

    int code = ::login(username, password);
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, loginErrorMessage(code))));
        return;
    }

    User* user = ::currentUser();
    if (user == nullptr)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "登录状态异常，请重试")));
        return;
    }

    // 单机版同时只有一个账号登录：新登录清空上一个账号遗留的撤销历史与 workspace.trash 内容，
    // 但保留磁盘根绑定，避免其他扫描根的树在重新登录后失去绑定
    ::clearHistory();

    Role role = user->role;
    string token = SessionManager::instance().createSession(username, role);

    // 单机版同一时间只允许一个账号登录，换账号时让旧 token 立即失效
    {
        lock_guard<mutex> lock(g_activeSessionMutex);
        if (!g_activeToken.empty() && g_activeToken != token)
        {
            SessionManager::instance().removeSession(g_activeToken);
        }
        g_activeToken = token;
    }

    // 顺带把内置管理员和最后登录时间写回用户表；失败不影响本次登录
    (void)persistUsers();

    json data;
    data["token"] = token;
    data["username"] = username;
    data["role"] = roleToString(role);
    data["expiresIn"] = 30 * 60;

    callback(makeJsonResponse(ApiResult::ok(data)));
}

// 注册：成功后不自动登录，由前端跳转登录页
void AuthController::handleRegister(const HttpRequestPtr& req,
                                    function<void(const HttpResponsePtr&)>&& callback)
{
    ensureUserStore();

    auto body = req->getJsonObject();
    string username;
    string password;
    if (!readStringField(body, "username", username)
        || !readStringField(body, "password", password)
        || username.empty() || password.empty())
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 username 或 password 参数")));
        return;
    }

    int code = ::registerUser(username, password);
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, registerErrorMessage(code))));
        return;
    }

    if (persistUsers() != 0)
    {
        callback(makeJsonResponse(ApiResult::error(-10, "用户已创建，但保存用户表失败")));
        return;
    }

    callback(makeJsonResponse(ApiResult::ok()));
}

// 登出：清除当前登录态、撤销历史，并移除 token 对应的会话
void AuthController::handleLogout(const HttpRequestPtr& req,
                                  function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    Session* session = SessionManager::instance().getSession(token);
    if (session == nullptr)
    {
        callback(makeJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    ::logout();
    ::clearHistory();
    SessionManager::instance().removeSession(token);

    {
        lock_guard<mutex> lock(g_activeSessionMutex);
        if (g_activeToken == token)
        {
            g_activeToken.clear();
        }
    }

    callback(makeJsonResponse(ApiResult::ok()));
}

// 当前登录用户信息
void AuthController::handleMe(const HttpRequestPtr& req,
                              function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    Session* session = SessionManager::instance().getSession(token);
    if (session == nullptr)
    {
        callback(makeJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    json data;
    data["username"] = session->username;
    data["role"] = roleToString(session->role);
    callback(makeJsonResponse(ApiResult::ok(data)));
}

// 修改当前登录用户密码
void AuthController::handleChangePassword(const HttpRequestPtr& req,
                                          function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    Session* session = SessionManager::instance().getSession(token);
    if (session == nullptr)
    {
        callback(makeJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    User* user = ::currentUser();
    if (user == nullptr || user->username != session->username)
    {
        callback(makeJsonResponse(ApiResult::error(-11, "登录状态已失效，请重新登录")));
        return;
    }

    auto body = req->getJsonObject();
    string oldPassword;
    string newPassword;
    if (!readStringField(body, "oldPassword", oldPassword)
        || !readStringField(body, "newPassword", newPassword))
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 oldPassword 或 newPassword 参数")));
        return;
    }

    int code = ::changePassword(oldPassword, newPassword);
    if (code != 0)
    {
        string message = "修改密码失败";
        if (code == -4)
        {
            message = "旧密码错误";
        }
        else if (code == -12)
        {
            message = "新密码不能为空且长度不能超过 128";
        }
        callback(makeJsonResponse(ApiResult::error(code, message)));
        return;
    }

    if (persistUsers() != 0)
    {
        callback(makeJsonResponse(ApiResult::error(-10, "密码已修改，但保存用户表失败")));
        return;
    }

    callback(makeJsonResponse(ApiResult::ok()));
}

// 用户列表：输出契约要求的 username、role、enabled、lastLoginAt
void AuthController::handleListUsers(const HttpRequestPtr& req,
                                     function<void(const HttpResponsePtr&)>&& callback)
{
    int authCode = checkAdmin(req);
    if (authCode != 0)
    {
        callback(makeJsonResponse(ApiResult::error(
            authCode, authCode == -11 ? "未登录或会话已失效" : "权限不足，仅管理员可操作")));
        return;
    }

    ensureUserStore();

    vector<User> users = ::getAllUsers();
    json userList = json::array();
    for (const User& user : users)
    {
        json item;
        item["username"] = user.username;
        item["role"] = roleToString(user.role);
        item["enabled"] = user.enabled;
        item["lastLoginAt"] = static_cast<long long>(user.lastLoginAt);
        userList.push_back(item);
    }

    callback(makeJsonResponse(ApiResult::ok(userList)));
}

// 启用/禁用用户：禁止管理员禁用当前登录账号，避免把自己锁在系统外
void AuthController::handleSetUserEnabled(const HttpRequestPtr& req,
                                          function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    Session* session = SessionManager::instance().getSession(token);
    int authCode = checkAdmin(req);
    if (authCode != 0)
    {
        callback(makeJsonResponse(ApiResult::error(
            authCode, authCode == -11 ? "未登录或会话已失效" : "权限不足，仅管理员可操作")));
        return;
    }

    string username = req->getParameter("1");
    auto body = req->getJsonObject();
    bool enabled = false;
    if (username.empty() || !readBoolField(body, "enabled", enabled))
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少用户名或 enabled 参数")));
        return;
    }
    if (!enabled && session != nullptr && username == session->username)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "不能禁用当前登录账号")));
        return;
    }
    if (!enabled && username == "admin")
    {
        callback(makeJsonResponse(ApiResult::error(-1, "内置管理员不可禁用")));
        return;
    }

    int code = ::setUserEnabled(username, enabled);
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, code == -1 ? "用户不存在" : "操作失败")));
        return;
    }

    if (persistUsers() != 0)
    {
        callback(makeJsonResponse(ApiResult::error(-10, "状态已修改，但保存用户表失败")));
        return;
    }

    callback(makeJsonResponse(ApiResult::ok()));
}

// 重置密码：管理员操作，重置后清除该账号锁定状态
void AuthController::handleResetPassword(const HttpRequestPtr& req,
                                         function<void(const HttpResponsePtr&)>&& callback)
{
    int authCode = checkAdmin(req);
    if (authCode != 0)
    {
        callback(makeJsonResponse(ApiResult::error(
            authCode, authCode == -11 ? "未登录或会话已失效" : "权限不足，仅管理员可操作")));
        return;
    }

    string username = req->getParameter("1");
    auto body = req->getJsonObject();
    string newPassword;
    if (username.empty() || !readStringField(body, "newPassword", newPassword) || newPassword.empty())
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少用户名或 newPassword 参数")));
        return;
    }

    int code = ::resetPassword(username, newPassword);
    if (code != 0)
    {
        string message = "重置密码失败";
        if (code == -1)
        {
            message = "用户不存在";
        }
        else if (code == -12)
        {
            message = "新密码格式非法";
        }
        callback(makeJsonResponse(ApiResult::error(code, message)));
        return;
    }

    if (persistUsers() != 0)
    {
        callback(makeJsonResponse(ApiResult::error(-10, "密码已重置，但保存用户表失败")));
        return;
    }

    callback(makeJsonResponse(ApiResult::ok()));
}

// 角色分配：管理员操作，禁止把当前登录管理员降级，避免失去唯一管理员入口
void AuthController::handleSetUserRole(const HttpRequestPtr& req,
                                       function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    Session* session = SessionManager::instance().getSession(token);
    int authCode = checkAdmin(req);
    if (authCode != 0)
    {
        callback(makeJsonResponse(ApiResult::error(
            authCode, authCode == -11 ? "未登录或会话已失效" : "权限不足，仅管理员可操作")));
        return;
    }

    string username = req->getParameter("1");
    auto body = req->getJsonObject();
    string roleText;
    Role role = Role::USER;
    if (username.empty() || !readStringField(body, "role", roleText) || !roleFromString(roleText, role))
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少用户名或 role 参数（GUEST/USER/ADMIN）")));
        return;
    }
    if (session != nullptr && username == session->username && role != Role::ADMIN)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "不能修改当前登录账号的管理员角色")));
        return;
    }
    if (username == "admin" && role != Role::ADMIN)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "内置管理员不可降级")));
        return;
    }

    int code = ::setUserRole(username, role);
    if (code != 0)
    {
        string message = "角色修改失败";
        if (code == -1)
        {
            message = "用户不存在";
        }
        else if (code == -12)
        {
            message = "角色非法";
        }
        callback(makeJsonResponse(ApiResult::error(code, message)));
        return;
    }

    if (persistUsers() != 0)
    {
        callback(makeJsonResponse(ApiResult::error(-10, "角色已修改，但保存用户表失败")));
        return;
    }

    callback(makeJsonResponse(ApiResult::ok()));
}
