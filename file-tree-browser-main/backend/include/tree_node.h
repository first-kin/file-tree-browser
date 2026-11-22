#ifndef TREE_NODE_H
#define TREE_NODE_H

#include <string>
#include <ctime>

using namespace std;

// tree_node.h —— 目录树节点（孩子兄弟表示法）
// 字段说明：
//   name       : 文件名/目录名（不含路径）
//   isDir      : true=目录, false=文件
//   size       : 文件大小(字节)，目录存 -1 表示需动态计算
//   ext        : 扩展名（文件有效，目录存 ""），如 ".cpp"
//   modifyTime : 最后修改时间（time_t 时间戳）
//   firstChild : 第一个孩子指针（目录才有，文件为 nullptr）
//   nextSibling: 下一个兄弟指针（链表方式连接同级节点）
//   parent     : 父节点指针（方便回溯，根节点 parent=nullptr）
struct TreeNode
{
    string name;
    bool isDir;
    long long size;
    string ext;
    time_t modifyTime;
    TreeNode* firstChild;
    TreeNode* nextSibling;
    TreeNode* parent;

    // 功能：默认构造函数，初始化所有字段
    // 参数：无
    // 返回值：无
    TreeNode();

    // 功能：便捷构造函数，设置名称与类型
    // 参数：n-文件名/目录名，d-是否为目录
    // 返回值：无
    TreeNode(const string& n, bool d);

    // 功能：析构函数，递归删除整棵子树（只删 firstChild 链，不删 nextSibling）
    // 参数：无
    // 返回值：无
    ~TreeNode();

    // 功能：添加子节点到 firstChild 链表尾部
    // 参数：child-待添加的子节点指针
    // 返回值：无（void）
    void addChild(TreeNode* child);

    // 功能：按名称从 firstChild 链中摘除子节点（不释放内存）
    // 参数：n-子节点名称
    // 返回值：true 摘除成功，false 未找到
    bool removeChild(const string& n);

    // 功能：查找是否存在指定名称的子节点
    // 参数：n-子节点名称
    // 返回值：true 存在，false 不存在
    bool hasChild(const string& n) const;

    // 功能：按名称查找子节点并返回指针
    // 参数：n-子节点名称
    // 返回值：找到返回节点指针，未找到返回 nullptr
    TreeNode* findChild(const string& n) const;
};

#endif // TREE_NODE_H
