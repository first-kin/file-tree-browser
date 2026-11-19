#ifndef GRAPH_DETECT_H
#define GRAPH_DETECT_H

#include "tree_node.h"
#include "graph_node.h"
#include <string>
#include <vector>

// graph_detect.h —— 目录链接图环检测（纯内存，禁止磁盘 I/O 与 Drogon 依赖）
// 图边来源：目录的孩子目录 + 快捷方式文件（X.lnk 指向名为 X 的目录）
// 建图后由 hasCycle / getCyclePaths 复用同一份邻接表；单机版只维护一份全局图

// 功能：将目录树 + 快捷方式边建模为有向图（邻接表），重复调用会先释放旧图
// 参数：root-目录树根节点
// 返回值：0 成功，-1 root 为空或非目录
// 复杂度：时间 O(V + E)，空间 O(V + E)
int buildGraph(TreeNode* root);

// 功能：DFS 三色标记法检测是否存在环
// 参数：无（使用 buildGraph 建立的图）
// 返回值：true 存在环，false 无环或尚未建图
// 复杂度：时间 O(V + E)，空间 O(V)
bool hasCycle();

// 功能：返回所有检测到的环路径（节点名称序列，首尾闭合）
// 参数：无（使用 buildGraph 建立的图）
// 返回值：环路径列表；无环返回空数组
// 复杂度：时间 O(V + E + C)，空间 O(V + C)；C 为环路径总长度
std::vector<std::vector<std::string>> getCyclePaths();

// 功能：带访问标记的安全遍历打印（visited 集合去重，遇到重复指针不再深入）
// 参数：root-目录树根节点
// 返回值：无
// 复杂度：时间 O(V)，空间 O(V)
void printSafe(TreeNode* root);

// 功能：读取最近一次建图的节点数
// 参数：无
// 返回值：图节点数
// 复杂度：时间 O(1)，空间 O(1)
int getGraphNodeCount();

// 功能：读取最近一次建图的边数（孩子边 + 快捷方式边）
// 参数：无
// 返回值：边数
// 复杂度：时间 O(1)，空间 O(1)
int getGraphEdgeCount();

#endif // GRAPH_DETECT_H
