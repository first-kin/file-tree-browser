#include "../include/traversal.h"
#include "../../3rdparty/json.hpp"
#include <iostream>
#include <fstream>
#include <stack>
#include <queue>
#include <utility>

using namespace std;
using json = nlohmann::json;

// 内部辅助函数：递归打印带树形线条的缩进文本
// prefix: 当前层之前的缩进前缀（如 "│   " 或 "    "）
// isLast: 当前节点是否是父节点的最后一个孩子
static void printTree(ostream& os, TreeNode* node, const string& prefix, bool isLast)
{
    if (node == nullptr)
    {
        return;
    }

    os << prefix;
    if (isLast)
    {
        os << "└── ";
    }
    else
    {
        os << "├── ";
    }
    os << node->name;
    if (node->isDir)
    {
        os << "/";
    }
    os << endl;

    // 孩子的前缀：如果当前节点不是最后一个兄弟，孩子层需要画竖线 │
    string childPrefix = prefix + (isLast ? "    " : "│   ");

    TreeNode* child = node->firstChild;
    while (child != nullptr)
    {
        bool isLastChild = (child->nextSibling == nullptr);
        printTree(os, child, childPrefix, isLastChild);
        child = child->nextSibling;
    }
}

// 前序递归遍历：缩进式文本树打印
void preorderPrint(TreeNode* root, int depth)
{
    if (root == nullptr)
    {
        return;
    }
    // 根节点无前缀，视为最后一个（无兄弟）
    printTree(cout, root, "", true);
}

// 后序递归遍历：先孩子，再根，再兄弟
void postorderPrint(TreeNode* root, int depth)
{
    if (root == nullptr)
    {
        return;
    }

    postorderPrint(root->firstChild, depth + 1);

    // 打印当前节点，带缩进
    for (int i = 0; i < depth; i++)
    {
        cout << "    ";
    }
    cout << root->name;
    if (root->isDir)
    {
        cout << "/";
    }
    cout << endl;

    postorderPrint(root->nextSibling, depth);
}

// 层次遍历：队列 BFS，按层分组打印
void levelorderPrint(TreeNode* root)
{
    if (root == nullptr)
    {
        return;
    }

    queue<TreeNode*> q;
    q.push(root);
    int level = 0;

    while (!q.empty())
    {
        int size = q.size();
        cout << "Level " << level << ": ";

        for (int i = 0; i < size; i++)
        {
            TreeNode* node = q.front();
            q.pop();

            cout << node->name;
            if (node->isDir)
            {
                cout << "/";
            }
            if (i < size - 1)
            {
                cout << ", ";
            }

            // 将当前节点的所有孩子入队
            TreeNode* child = node->firstChild;
            while (child != nullptr)
            {
                q.push(child);
                child = child->nextSibling;
            }
        }

        cout << endl;
        level++;
    }
}

// 非递归前序遍历：栈模拟，栈中存 pair<节点, 深度>
void preorderPrintIterative(TreeNode* root)
{
    if (root == nullptr)
    {
        return;
    }

    stack<pair<TreeNode*, int>> s;
    s.push({root, 0});

    while (!s.empty())
    {
        auto current = s.top();
        s.pop();
        TreeNode* node = current.first;
        int depth = current.second;

        // 打印当前节点
        for (int i = 0; i < depth; i++)
        {
            cout << "    ";
        }
        cout << node->name;
        if (node->isDir)
        {
            cout << "/";
        }
        cout << endl;

        // 先压兄弟（后处理），再压孩子（先处理），保证前序顺序
        if (node->nextSibling != nullptr)
        {
            s.push({node->nextSibling, depth});
        }
        if (node->firstChild != nullptr)
        {
            s.push({node->firstChild, depth + 1});
        }
    }
}

// 非递归后序遍历：双栈法
void postorderPrintIterative(TreeNode* root)
{
    if (root == nullptr)
    {
        return;
    }

    stack<TreeNode*> s1, s2;
    s1.push(root);

    while (!s1.empty())
    {
        TreeNode* node = s1.top();
        s1.pop();
        s2.push(node);

        // 先压孩子，再压兄弟，保证 s2 弹出顺序为后序
        if (node->firstChild != nullptr)
        {
            s1.push(node->firstChild);
        }
        if (node->nextSibling != nullptr)
        {
            s1.push(node->nextSibling);
        }
    }

    while (!s2.empty())
    {
        TreeNode* node = s2.top();
        s2.pop();
        cout << node->name;
        if (node->isDir)
        {
            cout << "/";
        }
        cout << endl;
    }
}

// 导出为缩进文本文件
int exportToText(TreeNode* root, const string& filePath)
{
    if (root == nullptr)
    {
        return -1;
    }

    ofstream ofs(filePath);
    if (!ofs.is_open())
    {
        return -10;
    }

    printTree(ofs, root, "", true);
    ofs.close();
    return 0;
}

// 内部辅助函数：将单个节点递归转换为 JSON 对象
static json nodeToJson(TreeNode* node)
{
    json j;
    j["name"] = node->name;
    j["isDir"] = node->isDir;
    j["size"] = node->size;
    j["ext"] = node->ext;
    j["modifyTime"] = node->modifyTime;

    json children = json::array();
    TreeNode* child = node->firstChild;
    while (child != nullptr)
    {
        children.push_back(nodeToJson(child));
        child = child->nextSibling;
    }
    j["children"] = children;

    return j;
}

// 导出为 JSON 文件
int exportToJson(TreeNode* root, const string& filePath)
{
    if (root == nullptr)
    {
        return -1;
    }

    ofstream ofs(filePath);
    if (!ofs.is_open())
    {
        return -10;
    }

    json j = nodeToJson(root);
    ofs << j.dump(2) << endl;
    ofs.close();
    return 0;
}
