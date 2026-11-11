#ifndef TRAVERSAL_H
#define TRAVERSAL_H

#include "tree_node.h"
#include <string>

using namespace std;

// ===== 递归版遍历 =====

// 功能：前序递归遍历并打印（缩进式文本树，带 ├── └── 线条）
// 参数：root-根节点，depth-当前深度（调用方传 0）
// 返回值：无（void）
void preorderPrint(TreeNode* root, int depth);

// 功能：后序递归遍历并打印
// 参数：root-根节点，depth-当前深度（调用方传 0）
// 返回值：无（void）
void postorderPrint(TreeNode* root, int depth);

// 功能：层次遍历并打印（按层分组，每层一行）
// 参数：root-根节点
// 返回值：无（void）
void levelorderPrint(TreeNode* root);

// ===== 非递归版遍历（栈模拟，用于对比演示） =====

// 功能：非递归前序遍历（栈模拟，防止超深目录栈溢出）
// 参数：root-根节点
// 返回值：无（void）
void preorderPrintIterative(TreeNode* root);

// 功能：非递归后序遍历（双栈法）
// 参数：root-根节点
// 返回值：无（void）
void postorderPrintIterative(TreeNode* root);

// ===== 导出 =====

// 功能：将树序列化为缩进文本并写入文件
// 参数：root-根节点，filePath-输出文件路径
// 返回值：0 成功，-10 文件写入失败
int exportToText(TreeNode* root, const string& filePath);

// 功能：将树序列化为 JSON 并写入文件（依赖 nlohmann/json）
// 参数：root-根节点，filePath-输出文件路径
// 返回值：0 成功，-10 文件写入失败
int exportToJson(TreeNode* root, const string& filePath);

#endif // TRAVERSAL_H
