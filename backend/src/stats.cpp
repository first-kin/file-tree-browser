#include "../include/stats.h"
#include <queue>
#include <algorithm>

using namespace std;

// 统计文件数和目录数（后序遍历）
void countNodes(TreeNode* root, int& fileCount, int& dirCount)
{
    if (root == nullptr)
    {
        return;
    }

    if (root->isDir)
    {
        dirCount++;
    }
    else
    {
        fileCount++;
    }

    countNodes(root->firstChild, fileCount, dirCount);
    countNodes(root->nextSibling, fileCount, dirCount);
}

// 后序遍历计算总大小：目录累加所有孩子的大小
long long calcTotalSize(TreeNode* root)
{
    if (root == nullptr)
    {
        return 0;
    }

    // 文件直接返回自身大小
    if (!root->isDir)
    {
        return root->size;
    }

    // 目录累加所有子树大小
    long long total = 0;
    TreeNode* child = root->firstChild;
    while (child != nullptr)
    {
        total += calcTotalSize(child);
        child = child->nextSibling;
    }
    return total;
}

// 最大深度：根为第 1 层
int maxDepth(TreeNode* root)
{
    if (root == nullptr)
    {
        return 0;
    }

    int childDepth = 0;
    TreeNode* child = root->firstChild;
    while (child != nullptr)
    {
        int d = maxDepth(child);
        if (d > childDepth)
        {
            childDepth = d;
        }
        child = child->nextSibling;
    }
    return childDepth + 1;
}

// 最大的 N 个文件（最小堆实现）
vector<TreeNode*> topNFiles(TreeNode* root, int n)
{
    vector<TreeNode*> result;
    if (root == nullptr || n <= 0)
    {
        return result;
    }

    // 最小堆：按文件大小升序，堆顶是当前最小的
    priority_queue<pair<long long, TreeNode*>, vector<pair<long long, TreeNode*>>, greater<pair<long long, TreeNode*>>> minHeap;

    // 前序遍历所有文件节点
    TreeNode* node = root;
    while (node != nullptr)
    {
        if (!node->isDir)
        {
            if (static_cast<int>(minHeap.size()) < n)
            {
                minHeap.push({node->size, node});
            }
            else if (node->size > minHeap.top().first)
            {
                minHeap.pop();
                minHeap.push({node->size, node});
            }
        }

        // 手动前序遍历（栈模拟，避免深递归栈溢出）
        // 这里用递归的话简单，但考虑目录可能很深，用栈
        // 简化：直接递归 firstChild，兄弟用循环
        // 为了简单清晰，这里用递归
        if (node->firstChild != nullptr)
        {
            node = node->firstChild;
        }
        else if (node->nextSibling != nullptr)
        {
            node = node->nextSibling;
        }
        else
        {
            // 回溯到有未访问兄弟的祖先
            while (node->parent != nullptr && node->nextSibling == nullptr)
            {
                node = node->parent;
            }
            if (node->parent == nullptr)
            {
                break;
            }
            node = node->nextSibling;
        }
    }

    // 堆中元素按大小降序输出
    while (!minHeap.empty())
    {
        result.push_back(minHeap.top().second);
        minHeap.pop();
    }
    reverse(result.begin(), result.end());
    return result;
}

// 空目录检测：isDir=true 且 firstChild=nullptr
vector<TreeNode*> findEmptyDirs(TreeNode* root)
{
    vector<TreeNode*> result;
    if (root == nullptr)
    {
        return result;
    }

    if (root->isDir && root->firstChild == nullptr)
    {
        result.push_back(root);
    }

    vector<TreeNode*> childResult = findEmptyDirs(root->firstChild);
    result.insert(result.end(), childResult.begin(), childResult.end());

    vector<TreeNode*> siblingResult = findEmptyDirs(root->nextSibling);
    result.insert(result.end(), siblingResult.begin(), siblingResult.end());

    return result;
}

// 每层节点数（BFS 按层计数）
vector<int> countByLevel(TreeNode* root)
{
    vector<int> result;
    if (root == nullptr)
    {
        return result;
    }

    queue<TreeNode*> q;
    q.push(root);

    while (!q.empty())
    {
        int size = static_cast<int>(q.size());
        result.push_back(size);

        for (int i = 0; i < size; i++)
        {
            TreeNode* node = q.front();
            q.pop();

            // 入队所有孩子
            TreeNode* child = node->firstChild;
            while (child != nullptr)
            {
                q.push(child);
                child = child->nextSibling;
            }
        }
    }

    return result;
}
