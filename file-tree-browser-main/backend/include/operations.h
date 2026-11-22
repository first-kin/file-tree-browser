#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "tree_node.h"
#include <string>

// operations.h —— 增删改与撤销（纯 C++，禁止包含 Drogon 等框架头文件）
// 所有修改类函数的成功操作都会自动压入撤销栈，调用方无需再手动 pushOperation
// 撤销只做“取消链接/重新链接”，不销毁节点；待释放节点由 clearHistory 统一回收
// 磁盘同步：通过 bindDiskRoot 绑定扫描根目录后，所有操作同时作用于磁盘；
// 删除/撤销新增/撤销复制采用“移入与扫描根同级的 workspace.trash 目录”的假删方式。

// ===== 磁盘根绑定 =====

// 功能：把内存树根绑定到磁盘扫描根目录，并创建/清理 workspace.trash 目录
// 参数：root-树根节点，diskRoot-扫描根目录（绝对路径，或相对路径 "workspace"）
// 返回值：0 成功，-1 root 为空/非目录，-2 磁盘根不存在，-10 创建 workspace.trash 目录失败
// 复杂度：时间 O(1)（不含路径探测），空间 O(1)
int bindDiskRoot(TreeNode* root, const std::string& diskRoot);

// 功能：判断当前绑定是否就是指定树根
// 参数：root-待判断的树根
// 返回值：true 已绑定且根一致
// 复杂度：时间 O(1)，空间 O(1)
bool isDiskBoundFor(TreeNode* root);

// 功能：解除磁盘绑定，清空撤销历史与 workspace.trash 目录（删除树/登出时调用）
// 参数：无
// 返回值：无
// 复杂度：时间 O(m)，空间 O(1)
void unbindDiskRoot();

// 撤销栈操作类型（pushOperation 的 opType 取值）
const int OP_ADD = 1;
const int OP_REMOVE = 2;
const int OP_RENAME = 3;
const int OP_MOVE = 4;
const int OP_COPY = 5;

// 功能：在 parent 下新增节点，并压入撤销栈
// 参数：parent-父目录节点，name-新节点名称（不含路径分隔符），isDir-是否为目录
// 返回值：0 成功，-1 parent 为空/非目录，-7 同级重名，-12 名称非法
// 复杂度：时间 O(k)，空间 O(1)；k 为同级子节点数
int addNode(TreeNode* parent, const std::string& name, bool isDir);

// 功能：删除节点及其整棵子树（先从树中摘除，撤销栈持有内存）
// 参数：node-待删除节点（调用方需先二次确认，根节点不可删除）
// 返回值：0 成功，-1 node 为空/根节点/摘除失败
// 复杂度：时间 O(k)，空间 O(1)；k 为同级子节点数
int removeNode(TreeNode* node);

// 功能：重命名节点，文件同步更新扩展名
// 参数：node-待重命名节点，newName-新名称（不含路径分隔符）
// 返回值：0 成功，-1 node 为空，-7 同级重名，-12 名称非法
// 复杂度：时间 O(k)，空间 O(1)
int renameNode(TreeNode* node, const std::string& newName);

// 功能：把节点移动到新父目录下，禁止移动到自身或自身子孙
// 参数：node-待移动节点，newParent-目标父目录
// 返回值：0 成功，-1 参数为空/根节点/目标非目录，-7 重名，-8 目标是自身子孙
// 复杂度：时间 O(d + k)，空间 O(1)；d 为树深
int moveNode(TreeNode* node, TreeNode* newParent);

// 功能：深拷贝整棵子树并挂到 newParent 下
// 参数：node-被拷贝节点，newParent-目标父目录
// 返回值：0 成功，-1 参数为空/目标非目录，-7 重名，-8 复制到自身或自身子孙
// 复杂度：时间 O(m)，空间 O(m)；m 为被拷贝子树节点数
int copyNode(TreeNode* node, TreeNode* newParent);

// 功能：按名称在 srcParent 下取源节点，深拷贝整棵子树到 destParent 下
// 参数：srcParent-源节点所在父目录，destParent-目标父目录，name-源节点名称
// 返回值：0 成功，-1 参数为空/源节点不存在/目标非目录，-7 重名，-8 复制到自身或自身子孙
// 复杂度：时间 O(m + k)，空间 O(m)
int copyNode(TreeNode* srcParent, TreeNode* destParent, const std::string& name);

// 功能：手动压入一条撤销记录（增删改函数已自动压栈，一般无需调用）
// 参数：opType-操作类型（OP_ADD 等），target-操作目标节点，backup-备份信息（如旧名称）
// 返回值：无
// 复杂度：时间 O(1)，空间 O(1)
void pushOperation(int opType, TreeNode* target, const std::string& backup);

// 功能：撤销最近一次操作，只取消链接/恢复链接，不删除节点
// 参数：无
// 返回值：0 成功，-1 无历史记录或记录无效
// 复杂度：时间 O(k)，空间 O(1)
int undoLast();

// 功能：清空撤销历史，并释放尚未恢复的已删除/已撤销子树（登出或重新建树时调用）
// 参数：无
// 返回值：无
// 复杂度：时间 O(m)，空间 O(1)
void clearHistory();

#endif // OPERATIONS_H
