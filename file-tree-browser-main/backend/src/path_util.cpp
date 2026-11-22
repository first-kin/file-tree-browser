#include "../include/path_util.h"

#include <cctype>

using namespace std;

// 白名单根目录的静态存储（进程内共享，启动时由 main.cpp 注入一次）
vector<string> PathUtil::allowedRoots;

// ===== 内部辅助函数 =====

// 将 ASCII 字母转为小写，用于 Windows 路径的大小写无关比较
static char toLowerAscii(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

// 比较 path 前 root.size() 个字符是否与 root 相同（忽略大小写）
static bool regionEqualsIgnoreCase(const string& path, const string& root)
{
    if (path.size() < root.size())
    {
        return false;
    }
    for (size_t i = 0; i < root.size(); i++)
    {
        if (toLowerAscii(path[i]) != toLowerAscii(root[i]))
        {
            return false;
        }
    }
    return true;
}

// 判断已标准化的 path 是否位于已标准化的 root 之内（含路径边界判断）
static bool isUnderRoot(const string& path, const string& root)
{
    if (!regionEqualsIgnoreCase(path, root))
    {
        return false;
    }
    if (path.size() == root.size())
    {
        return true;
    }
    // root 本身即根路径（如 "C:/"、"/"），前缀相同就视为在其内部
    if (root.back() == '/')
    {
        return true;
    }
    // 边界完整：root 之后必须紧跟 '/'，防止 "C:/Test" 放行 "C:/Test_Evil"
    return path[root.size()] == '/';
}

// ===== 对外接口实现 =====

// 标准化路径：统一分隔符、折叠 "." 和 ".."、去除多余与末尾分隔符
string PathUtil::normalizePath(const string& inputPath)
{
    if (inputPath.empty())
    {
        return "";
    }

    // 1. 统一分隔符：所有 '\' 转为 '/'
    string s;
    s.reserve(inputPath.size());
    for (char c : inputPath)
    {
        s.push_back(c == '\\' ? '/' : c);
    }

    // 2. 识别路径前缀：UNC（//）、盘符（C:）、绝对路径（/）、相对路径（无前缀）
    string prefix;
    size_t pos = 0;
    if (s.size() >= 2 && s[0] == '/' && s[1] == '/')
    {
        prefix = "//";
        pos = 2;
    }
    else if (s.size() >= 2 && isalpha(static_cast<unsigned char>(s[0])) && s[1] == ':')
    {
        prefix = s.substr(0, 2);
        prefix[0] = static_cast<char>(toupper(static_cast<unsigned char>(prefix[0])));
        pos = 2;
    }
    else if (s[0] == '/')
    {
        prefix = "/";
        pos = 1;
    }

    // 3. 逐段解析，空段与 "." 直接丢弃，".." 出栈；越界立即判定非法
    vector<string> segments;
    while (pos < s.size())
    {
        size_t end = s.find('/', pos);
        if (end == string::npos)
        {
            end = s.size();
        }
        string seg = s.substr(pos, end - pos);
        pos = end + 1;

        if (seg.empty() || seg == ".")
        {
            continue;
        }
        if (seg == "..")
        {
            if (segments.empty())
            {
                // 已到达根目录仍试图回退，视为路径穿越
                return "";
            }
            segments.pop_back();
            continue;
        }
        segments.push_back(seg);
    }

    // 4. 拼接标准化结果
    string tail;
    for (size_t i = 0; i < segments.size(); i++)
    {
        if (i > 0)
        {
            tail.push_back('/');
        }
        tail += segments[i];
    }

    if (prefix.empty())
    {
        // 相对路径；整段被折叠为空时用 "." 表示当前目录
        return tail.empty() ? "." : tail;
    }
    if (prefix == "/")
    {
        return "/" + tail;
    }
    if (prefix == "//")
    {
        return "//" + tail;
    }

    // 盘符路径：保证形如 "C:/" 或 "C:/dir1/dir2"
    return prefix + "/" + tail;
}

// 注入白名单：逐个标准化后保存，非法根目录直接丢弃
void PathUtil::setAllowedRoots(const vector<string>& roots)
{
    allowedRoots.clear();
    for (const string& root : roots)
    {
        string normalized = normalizePath(root);
        if (!normalized.empty())
        {
            allowedRoots.push_back(normalized);
        }
    }
}

// 合法性校验主函数：标准化 -> 前缀匹配（忽略大小写 + 边界完整）
bool PathUtil::isValidScanPath(const string& inputPath)
{
    // 白名单为空时一律拒绝（失败关闭，避免无约束扫描）
    if (allowedRoots.empty())
    {
        return false;
    }

    string normalized = normalizePath(inputPath);
    if (normalized.empty())
    {
        return false;
    }

    for (const string& root : allowedRoots)
    {
        if (isUnderRoot(normalized, root))
        {
            return true;
        }
    }
    return false;
}