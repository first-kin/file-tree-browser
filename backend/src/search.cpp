#include "../include/search.h"
#include "../../3rdparty/picosha2.h"
#include <fstream>
#include <iterator>
#include <unordered_map>

using namespace std;

// ===== 内部辅助函数 =====

// 通配符匹配：* 匹配任意长度字符，? 匹配单个字符
static bool wildcardMatch(const string& pattern, const string& str)
{
    int n = static_cast<int>(pattern.size());
    int m = static_cast<int>(str.size());
    int i = 0, j = 0;
    int star = -1, match = 0;

    while (i < m)
    {
        if (j < n && (pattern[j] == '?' || pattern[j] == str[i]))
        {
            i++;
            j++;
        }
        else if (j < n && pattern[j] == '*')
        {
            star = j++;
            match = i;
        }
        else if (star != -1)
        {
            j = star + 1;
            i = ++match;
        }
        else
        {
            return false;
        }
    }

    while (j < n && pattern[j] == '*')
    {
        j++;
    }
    return j == n;
}

// 通过 parent 指针回溯，拼接完整路径
static string buildFullPath(TreeNode* node)
{
    if (node == nullptr)
    {
        return "";
    }
    if (node->parent == nullptr)
    {
        return node->name;
    }
    return buildFullPath(node->parent) + "/" + node->name;
}

// 读取文件内容并计算 SHA256 哈希
static string hashFileContent(const string& filePath)
{
    ifstream ifs(filePath, ios::binary);
    if (!ifs.is_open())
    {
        return "";
    }
    string content((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
    ifs.close();
    return picosha2::hash256_hex_string(content);
}

// 前序遍历所有文件节点，收集到 results
static void collectAllFiles(TreeNode* node, vector<TreeNode*>& results)
{
    if (node == nullptr)
    {
        return;
    }
    if (!node->isDir)
    {
        results.push_back(node);
    }
    collectAllFiles(node->firstChild, results);
    collectAllFiles(node->nextSibling, results);
}

// ===== 接口实现 =====

// 按名称精确查找，返回第一个匹配节点
TreeNode* findByName(TreeNode* root, const string& name)
{
    if (root == nullptr)
    {
        return nullptr;
    }
    if (root->name == name)
    {
        return root;
    }

    TreeNode* found = findByName(root->firstChild, name);
    if (found != nullptr)
    {
        return found;
    }
    return findByName(root->nextSibling, name);
}

// 通配符查找（支持 * ?），返回所有匹配节点
vector<TreeNode*> findByPattern(TreeNode* root, const string& pattern)
{
    vector<TreeNode*> results;
    if (root == nullptr)
    {
        return results;
    }

    if (wildcardMatch(pattern, root->name))
    {
        results.push_back(root);
    }

    // 递归孩子和兄弟
    vector<TreeNode*> childResults = findByPattern(root->firstChild, pattern);
    results.insert(results.end(), childResults.begin(), childResults.end());

    vector<TreeNode*> siblingResults = findByPattern(root->nextSibling, pattern);
    results.insert(results.end(), siblingResults.begin(), siblingResults.end());

    return results;
}

// 按扩展名筛选（如 ".cpp"）
vector<TreeNode*> findByExt(TreeNode* root, const string& ext)
{
    vector<TreeNode*> results;
    if (root == nullptr)
    {
        return results;
    }

    if (!root->isDir && root->ext == ext)
    {
        results.push_back(root);
    }

    vector<TreeNode*> childResults = findByExt(root->firstChild, ext);
    results.insert(results.end(), childResults.begin(), childResults.end());

    vector<TreeNode*> siblingResults = findByExt(root->nextSibling, ext);
    results.insert(results.end(), siblingResults.begin(), siblingResults.end());

    return results;
}

// 按大小范围查找（字节，闭区间 [minSize, maxSize]），仅文件
vector<TreeNode*> findBySize(TreeNode* root, long long minSize, long long maxSize)
{
    vector<TreeNode*> results;
    if (root == nullptr)
    {
        return results;
    }

    if (!root->isDir && root->size >= minSize && root->size <= maxSize)
    {
        results.push_back(root);
    }

    vector<TreeNode*> childResults = findBySize(root->firstChild, minSize, maxSize);
    results.insert(results.end(), childResults.begin(), childResults.end());

    vector<TreeNode*> siblingResults = findBySize(root->nextSibling, minSize, maxSize);
    results.insert(results.end(), siblingResults.begin(), siblingResults.end());

    return results;
}

// 按修改时间范围查找（闭区间 [begin, end]）
vector<TreeNode*> findByTime(TreeNode* root, time_t begin, time_t end)
{
    vector<TreeNode*> results;
    if (root == nullptr)
    {
        return results;
    }

    if (root->modifyTime >= begin && root->modifyTime <= end)
    {
        results.push_back(root);
    }

    vector<TreeNode*> childResults = findByTime(root->firstChild, begin, end);
    results.insert(results.end(), childResults.begin(), childResults.end());

    vector<TreeNode*> siblingResults = findByTime(root->nextSibling, begin, end);
    results.insert(results.end(), siblingResults.begin(), siblingResults.end());

    return results;
}

// 重复文件检测：按 (大小+扩展名) 分桶，桶内多个时比内容哈希
vector<vector<TreeNode*>> findDuplicates(TreeNode* root)
{
    vector<vector<TreeNode*>> result;
    if (root == nullptr)
    {
        return result;
    }

    // 收集所有文件节点
    vector<TreeNode*> allFiles;
    collectAllFiles(root, allFiles);

    // 按 (size + ext) 分桶
    unordered_map<string, vector<TreeNode*>> sizeBuckets;
    for (TreeNode* file : allFiles)
    {
        string key = to_string(file->size) + "|" + file->ext;
        sizeBuckets[key].push_back(file);
    }

    // 对每个有多个元素的桶，按内容哈希再分桶
    for (auto& pair : sizeBuckets)
    {
        if (pair.second.size() < 2)
        {
            continue;
        }

        unordered_map<string, vector<TreeNode*>> hashBuckets;
        for (TreeNode* file : pair.second)
        {
            string fullPath = buildFullPath(file);
            string hash = hashFileContent(fullPath);
            if (!hash.empty())
            {
                hashBuckets[hash].push_back(file);
            }
        }

        // 哈希相同且 >=2 个的组为重复文件
        for (auto& hp : hashBuckets)
        {
            if (hp.second.size() >= 2)
            {
                result.push_back(hp.second);
            }
        }
    }

    return result;
}
