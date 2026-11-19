#ifndef AUTH_H
#define AUTH_H

#include "user.h"
#include <string>
#include <vector>

// auth.h —— 权限与用户管理（纯业务层，禁止包含 Drogon 等框架头文件）
// 用户表以 username 为主键常驻内存，loadUsers / saveUsers 负责 data/users.dat 持久化
// 系统内置固定管理员：用户名 admin，密码 123456，首次使用时自动创建

// 功能：注册新用户，角色固定为 USER，密码按 SHA256(password + salt) 存储
// 参数：username-用户名（唯一，不含 '|' 和空白），password-明文密码
// 返回值：0 成功，-7 用户名已存在，-12 参数非法
// 复杂度：时间 O(log n)，空间 O(1)
int registerUser(const std::string& username, const std::string& password);

// 功能：登录校验，连续失败 3 次锁定 5 分钟，成功后重置失败计数
// 参数：username-用户名，password-明文密码
// 返回值：0 成功，-4 用户名或密码错误，-5 账号锁定，-6 账号禁用，-12 参数为空
// 复杂度：时间 O(log n + |pwd|)，空间 O(1)
int login(const std::string& username, const std::string& password);

// 功能：清除当前登录用户（登出）
// 参数：无
// 返回值：无
// 复杂度：时间 O(1)，空间 O(1)
void logout();

// 功能：获取当前登录用户
// 参数：无
// 返回值：当前用户指针；未登录返回 nullptr
// 复杂度：时间 O(1)，空间 O(1)
User* currentUser();

// 功能：校验当前用户是否拥有指定角色及以上权限（未登录视为 GUEST）
// 参数：required-要求的最低角色
// 返回值：true 有权限，false 无权限或未登录
// 复杂度：时间 O(1)，空间 O(1)
bool hasPermission(Role required);

// 功能：修改当前登录用户的密码（校验旧密码，重新生成盐）
// 参数：oldPwd-旧明文密码，newPwd-新明文密码
// 返回值：0 成功，-4 旧密码错误，-11 未登录，-12 新密码非法
// 复杂度：时间 O(|pwd|)，空间 O(1)
int changePassword(const std::string& oldPwd, const std::string& newPwd);

// 功能：重置指定用户密码（管理员操作，权限校验由控制器完成）
// 参数：username-目标用户名，newPwd-新明文密码
// 返回值：0 成功，-1 用户不存在，-12 新密码非法
// 复杂度：时间 O(log n + |pwd|)，空间 O(1)
int resetPassword(const std::string& username, const std::string& newPwd);

// 功能：启用或禁用指定用户（管理员操作，权限校验由控制器完成）
// 参数：username-目标用户名，enabled-true 启用，false 禁用
// 返回值：0 成功，-1 用户不存在
// 复杂度：时间 O(log n)，空间 O(1)
int setUserEnabled(const std::string& username, bool enabled);

// 功能：修改指定用户的角色（管理员操作，权限校验由控制器完成）
// 参数：username-目标用户名，role-新角色
// 返回值：0 成功，-1 用户不存在，-12 角色非法
// 复杂度：时间 O(log n)，空间 O(1)
int setUserRole(const std::string& username, Role role);

// 功能：获取全部用户快照（按用户名升序）
// 参数：无
// 返回值：用户列表副本（含密码哈希与盐值，仅供业务层使用，禁止直接输出）
// 复杂度：时间 O(n)，空间 O(n)
std::vector<User> getAllUsers();

// 功能：从文件加载用户表（服务端启动时调用，文件不存在视为空表）
// 参数：filePath-用户表文件路径
// 返回值：0 成功，-10 文件读取失败
// 复杂度：时间 O(n)，空间 O(n)
int loadUsers(const std::string& filePath);

// 功能：将用户表写入文件（每条记录一行，字段以 '|' 分隔；先写临时文件再替换）
// 参数：filePath-用户表文件路径
// 返回值：0 成功，-10 文件写入失败
// 复杂度：时间 O(n)，空间 O(1)
int saveUsers(const std::string& filePath);

#endif // AUTH_H
