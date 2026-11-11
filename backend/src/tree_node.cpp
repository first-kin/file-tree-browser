#include "../include/tree_node.h"

using namespace std;

// 默认构造函数：所有字段初始化为安全默认值
TreeNode::TreeNode()
    : name("")
    , isDir(false)
    , size(-1)
    , ext("")
    , modifyTime(0)
    , firstChild(nullptr)
    , nextSibling(nullptr)
    , parent(nullptr)
{
}

// 便捷构造函数：设置名称与目录标记，其余字段同默认值
TreeNode::TreeNode(const string& n, bool d)
    : name(n)
    , isDir(d)
    , size(-1)
    , ext("")
    , modifyTime(0)
    , firstChild(nullptr)
    , nextSibling(nullptr)
    , parent(nullptr)
{
}

// 析构函数：递归删除 firstChild 子树
// 注意：不删除 nextSibling，由父节点的 firstChild 链统一管理，避免重复释放
TreeNode::~TreeNode()
{
    delete firstChild;
    firstChild = nullptr;
}

// 添加子节点到 firstChild 链表尾部，并设置 parent 指针
void TreeNode::addChild(TreeNode* child)
{
    if (child == nullptr)
    {
        return;
    }

    child->parent = this;

    if (firstChild == nullptr)
    {
        firstChild = child;
        return;
    }

    TreeNode* cur = firstChild;
    while (cur->nextSibling != nullptr)
    {
        cur = cur->nextSibling;
    }
    cur->nextSibling = child;
}

// 按名称从 firstChild 链中摘除子节点（不释放内存，由调用方决定）
bool TreeNode::removeChild(const string& n)
{
    if (firstChild == nullptr)
    {
        return false;
    }

    // 待删除节点是第一个孩子
    if (firstChild->name == n)
    {
        TreeNode* target = firstChild;
        firstChild = firstChild->nextSibling;
        target->nextSibling = nullptr;
        target->parent = nullptr;
        return true;
    }

    // 在链表中间/尾部查找
    TreeNode* prev = firstChild;
    TreeNode* cur = firstChild->nextSibling;
    while (cur != nullptr)
    {
        if (cur->name == n)
        {
            prev->nextSibling = cur->nextSibling;
            cur->nextSibling = nullptr;
            cur->parent = nullptr;
            return true;
        }
        prev = cur;
        cur = cur->nextSibling;
    }

    return false;
}

// 查找是否存在指定名称的子节点
bool TreeNode::hasChild(const string& n) const
{
    TreeNode* cur = firstChild;
    while (cur != nullptr)
    {
        if (cur->name == n)
        {
            return true;
        }
        cur = cur->nextSibling;
    }
    return false;
}

// 按名称查找子节点并返回指针
TreeNode* TreeNode::findChild(const string& n) const
{
    TreeNode* cur = firstChild;
    while (cur != nullptr)
    {
        if (cur->name == n)
        {
            return cur;
        }
        cur = cur->nextSibling;
    }
    return nullptr;
}
