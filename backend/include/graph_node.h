#ifndef GRAPH_NODE_H
#define GRAPH_NODE_H

#include "tree_node.h"
#include <unordered_map>
#include <vector>

// graph_node.h —— 目录链接图节点（三色标记法载体，接口冻结项）
struct GraphNode
{
    TreeNode* dirNode;                    // 对应的目录树节点
    std::vector<GraphNode*> neighbors;    // 出边列表（孩子目录 + 快捷方式指向）
    int color;                            // 三色标记：0 白（未访问）、1 灰（访问中）、2 黑（已完成）
};

// 邻接表：目录树节点 -> 图节点
using DirGraph = std::unordered_map<TreeNode*, GraphNode*>;

#endif // GRAPH_NODE_H
