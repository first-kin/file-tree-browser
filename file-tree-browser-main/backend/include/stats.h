#ifndef STATS_H
#define STATS_H

#include "tree_node.h"
#include <vector>

using namespace std;

// 功能：统计树中文件数和目录数
// 参数：root-根节点，fileCount-文件数输出，dirCount-目录数输出
// 返回值：无（通过引用参数返回）
void countNodes(TreeNode* root, int& fileCount, int& dirCount);

// 功能：后序遍历计算目录总大小（所有子孙文件大小之和）
// 参数：root-根节点
// 返回值：总大小（字节），空树返回 0
long long calcTotalSize(TreeNode* root);

// 功能：求树的最大深度（根为第 1 层）
// 参数：root-根节点
// 返回值：最大深度，空树返回 0
int maxDepth(TreeNode* root);

// 功能：找出最大的 N 个文件（最小堆实现）
// 参数：root-根节点，n-取前 N 大
// 返回值：最大的 N 个文件节点列表（按大小降序）
vector<TreeNode*> topNFiles(TreeNode* root, int n);

// 功能：检测所有空目录（isDir=true 且 firstChild=nullptr）
// 参数：root-根节点
// 返回值：空目录节点列表
vector<TreeNode*> findEmptyDirs(TreeNode* root);

// 功能：统计每层的节点数（宽度分布）
// 参数：root-根节点
// 返回值：vector，下标 i 表示第 i+1 层的节点数
vector<int> countByLevel(TreeNode* root);

#endif // STATS_H
