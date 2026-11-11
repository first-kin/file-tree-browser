#ifndef SEARCH_H
#define SEARCH_H

#include "tree_node.h"
#include <vector>
#include <string>
#include <ctime>

using namespace std;

// 按名称精确查找，返回第一个匹配节点，未找到返回 nullptr
TreeNode* findByName(TreeNode* root, const string& name);

// 通配符查找（支持 * 和 ?），返回所有匹配节点
vector<TreeNode*> findByPattern(TreeNode* root, const string& pattern);

// 按扩展名筛选（如 ".cpp"），返回所有匹配的文件节点
vector<TreeNode*> findByExt(TreeNode* root, const string& ext);

// 按大小范围查找（字节，闭区间 [minSize, maxSize]），仅文件节点
vector<TreeNode*> findBySize(TreeNode* root, long long minSize, long long maxSize);

// 按修改时间范围查找（闭区间 [begin, end]）
vector<TreeNode*> findByTime(TreeNode* root, time_t begin, time_t end);

// 重复文件检测：先按 (大小+扩展名) 分桶，桶内多个时再比内容哈希
// 返回值：每个内层 vector 是一组重复文件（>=2 个）
vector<vector<TreeNode*>> findDuplicates(TreeNode* root);

#endif // SEARCH_H
