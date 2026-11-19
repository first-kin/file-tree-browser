#include "../include/path_util.h"
#include "../../backend/include/path_util.h"

#include <drogon/drogon.h>

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

using namespace std;

// ===== 扫描白名单 =====

// 从 config.json 的 custom_config.allowed_scan_roots 读取白名单，
// 不再硬编码；第一次调用时初始化，之后复用。
static void initializeWhitelist()
{
    vector<string> roots;
    const Json::Value& config = drogon::app().getCustomConfig();
    if (config.isMember("allowed_scan_roots") && config["allowed_scan_roots"].isArray())
    {
        const Json::Value& rootsValue = config["allowed_scan_roots"];
        for (Json::ArrayIndex i = 0; i < rootsValue.size(); i++)
        {
            if (rootsValue[i].isString())
            {
                roots.push_back(rootsValue[i].asString());
            }
        }
    }

    // workspace 演示目录：按运行目录的常见位置探测，不写死绝对路径
    const char* workspaceCandidates[] = {
        "workspace",
        "../workspace",
        "../../workspace",
        "../../../workspace",
        "file-tree-browser/workspace",
        "../file-tree-browser/workspace",
    };
    for (const char* candidate : workspaceCandidates)
    {
        std::error_code ec;
        std::filesystem::path relative = std::filesystem::u8path(std::string(candidate));
        if (!std::filesystem::is_directory(relative, ec))
        {
            continue;
        }
        std::filesystem::path absolute = std::filesystem::absolute(relative, ec);
        if (ec)
        {
            continue;
        }
        std::filesystem::path normalized = std::filesystem::weakly_canonical(absolute, ec);
        if (ec)
        {
            normalized = absolute.lexically_normal();
        }
        const std::string workspaceRoot = normalized.generic_u8string();

        bool duplicated = false;
        for (const std::string& root : roots)
        {
            if (root == workspaceRoot)
            {
                duplicated = true;
                break;
            }
        }
        if (!duplicated)
        {
            roots.push_back(workspaceRoot);
        }
    }

    if (!roots.empty())
    {
        PathUtil::setAllowedRoots(roots);
    }
}

// 标准化与白名单匹配统一交给 backend/PathUtil，避免重复实现
bool isValidScanPath(const string& path)
{
    static once_flag initFlag;
    call_once(initFlag, initializeWhitelist);
    return PathUtil::isValidScanPath(path);
}

// ===== 根相对路径 <-> TreeNode* =====

// AI生成，已手动审查修改：路径逐段解析，遇到 ".." 或盘符绝对路径直接判定穿越
int resolveNodePath(TreeNode* root, const string& path, TreeNode** out)
{
    if (out != nullptr)
    {
        *out = nullptr;
    }
    if (root == nullptr || out == nullptr)
    {
        return -1;
    }

    // 根路径
    if (path.empty() || path == "/" || path == "\\")
    {
        *out = root;
        return 0;
    }

    TreeNode* current = root;
    string segment;
    for (size_t i = 0; i <= path.size(); i++)
    {
        char c = (i < path.size()) ? path[i] : '/';
        if (c == '/' || c == '\\')
        {
            if (segment.empty() || segment == ".")
            {
                segment.clear();
                continue;
            }
            if (segment == "..")
            {
                return -3; // 不允许回退到父目录
            }
            // 根相对路径中不允许出现盘符或 UNC 绝对路径
            if (segment.find(':') != string::npos)
            {
                return -3;
            }
            if (!current->isDir)
            {
                return -2;
            }
            TreeNode* next = current->findChild(segment);
            if (next == nullptr)
            {
                return -2;
            }
            current = next;
            segment.clear();
        }
        else
        {
            segment.push_back(c);
        }
    }

    *out = current;
    return 0;
}

// 根相对路径解析，失败统一返回 nullptr
TreeNode* pathToNode(TreeNode* root, const string& path)
{
    TreeNode* node = nullptr;
    if (resolveNodePath(root, path, &node) != 0)
    {
        return nullptr;
    }
    return node;
}

// 从节点向父指针回溯拼接路径；根节点返回 "/"
string nodeToPath(TreeNode* node)
{
    if (node == nullptr)
    {
        return "";
    }

    vector<string> names;
    TreeNode* current = node;
    while (current->parent != nullptr)
    {
        names.push_back(current->name);
        current = current->parent;
    }

    string path;
    for (size_t i = names.size(); i > 0; i--)
    {
        path.push_back('/');
        path += names[i - 1];
    }
    return path.empty() ? "/" : path;
}
