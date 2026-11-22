#ifndef SERVER_PATH_UTIL_H
#define SERVER_PATH_UTIL_H

#include "../../backend/include/tree_node.h"
#include <string>

// server/path_util.h —— 服务层路径工具
// 负责：扫描路径白名单校验、根相对路径 <-> TreeNode* 互转、路径穿越拦截

// 功能：校验请求路径是否位于扫描白名单根目录内，拦截 ../ 路径穿越
// 参数：path-前端传入的待扫描路径
// 返回值：true 合法；false 越界、非法或不在白名单内
// 复杂度：时间 O(n * r)，空间 O(n)
bool isValidScanPath(const std::string& path);

// 功能：把根相对路径解析为树节点
// 参数：root-目录树根节点，path-根相对路径（"/" 表示根）
// 返回值：成功返回节点指针；路径不存在或越界返回 nullptr
// 复杂度：时间 O(d * k)，空间 O(d)；d 为路径深度，k 为同级节点数
TreeNode* pathToNode(TreeNode* root, const std::string& path);

// 功能：把树节点转换为根相对路径
// 参数：node-待转换的树节点
// 返回值：根相对路径（根节点为 "/"）；node 为空返回空串
// 复杂度：时间 O(d)，空间 O(d)
std::string nodeToPath(TreeNode* node);

// 功能：带错误码的路径解析，供控制器区分“不存在(-2)”与“路径穿越(-3)”
// 参数：root-目录树根节点，path-根相对路径，out-输出节点指针
// 返回值：0 成功，-1 参数为空，-2 路径不存在/不是目录，-3 路径穿越或绝对磁盘路径
// 复杂度：时间 O(d * k)，空间 O(d)
int resolveNodePath(TreeNode* root, const std::string& path, TreeNode** out);

#endif // SERVER_PATH_UTIL_H
