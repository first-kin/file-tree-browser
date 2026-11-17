#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include <drogon/drogon.h>

// auth_controller.h —— 认证与用户管理 HTTP 接口
// 严格按《模块接口对接文档》第 13.1 节注册路由
// 复杂度说明：控制器只做参数解析、鉴权、调用 backend 与响应封装，
// 各 handler 自身为 O(1)（不含 backend 模块内部复杂度）。
class AuthController : public drogon::HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN
    METHOD_ADD(AuthController::handleLogin, "/api/auth/login", Post);
    METHOD_ADD(AuthController::handleRegister, "/api/auth/register", Post);
    METHOD_ADD(AuthController::handleLogout, "/api/auth/logout", Post);
    METHOD_ADD(AuthController::handleMe, "/api/auth/me", Get);
    METHOD_ADD(AuthController::handleChangePassword, "/api/auth/password", Put);
    METHOD_ADD(AuthController::handleListUsers, "/api/users", Get);
    METHOD_ADD(AuthController::handleSetUserEnabled, "/api/users/{1}/enabled", Put);
    METHOD_ADD(AuthController::handleResetPassword, "/api/users/{1}/password", Put);
    METHOD_ADD(AuthController::handleSetUserRole, "/api/users/{1}/role", Put);
    METHOD_LIST_END

    // 功能：登录校验，成功签发 token 并返回角色
    // 参数：req-HTTP 请求（JSON Body 含 username、password）
    // 返回值：无（通过 callback 返回 {code, message, data}）
    void handleLogin(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：注册新用户，默认 USER 角色
    // 参数：req-HTTP 请求（JSON Body 含 username、password）
    // 返回值：无（通过 callback 返回 {code, message, data}）
    void handleRegister(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：登出，清除当前登录态、撤销历史与会话
    // 参数：req-HTTP 请求（可选 Authorization 头）
    // 返回值：无（通过 callback 返回 {code, message, data}）
    void handleLogout(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：获取当前登录用户信息
    // 参数：req-HTTP 请求（Authorization 头）
    // 返回值：无（data 为 {username, role}）
    void handleMe(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：修改当前登录用户密码
    // 参数：req-JSON Body 含 oldPassword、newPassword
    // 返回值：无（成功 data 为 null）
    void handleChangePassword(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：管理员获取用户列表
    // 参数：req-HTTP 请求（Authorization 头）
    // 返回值：无（data 为 [{username, role, enabled, lastLoginAt}]）
    void handleListUsers(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：管理员启用或禁用指定用户
    // 参数：路径参数 1-用户名；JSON Body 含 enabled
    // 返回值：无（成功 data 为 null）
    void handleSetUserEnabled(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：管理员重置指定用户密码
    // 参数：路径参数 1-用户名；JSON Body 含 newPassword
    // 返回值：无（成功 data 为 null）
    void handleResetPassword(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：管理员修改指定用户角色
    // 参数：路径参数 1-用户名；JSON Body 含 role（GUEST/USER/ADMIN）
    // 返回值：无（成功 data 为 null）
    void handleSetUserRole(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

#endif // AUTH_CONTROLLER_H
