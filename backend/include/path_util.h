#ifndef PATH_UTIL_H
#define PATH_UTIL_H

#include <string>
#include <vector>

// path_util.h —— 扫描路径安全工具（纯业务层，禁止依赖 Drogon 等框架）
// 职责：路径标准化、路径穿越拦截、白名单根目录校验
class PathUtil
{
public:
    // 功能：标准化路径字符串，统一分隔符为 '/'，折叠 "." 与 ".."
    // 参数：inputPath-待标准化的原始路径（允许混合使用 / 和 \）
    // 返回值：标准化后的路径；路径越界、非法或为空时返回空字符串
    // 复杂度：时间 O(n)，空间 O(n)
    static std::string normalizePath(const std::string& inputPath);

    // 功能：设置允许扫描的根目录白名单（系统启动时由 main.cpp 注入）
    // 参数：roots-白名单根目录列表，如 {"C:/TestWorkspace"}
    // 返回值：无
    // 复杂度：时间 O(n)，空间 O(n)
    static void setAllowedRoots(const std::vector<std::string>& roots);

    // 功能：校验请求路径是否位于白名单根目录内，用于拦截路径穿越
    // 参数：inputPath-前端传入的待扫描路径
    // 返回值：true 合法可扫描；false 非法、越界或白名单为空
    // 复杂度：时间 O(n * r)，空间 O(n)；r 为白名单根目录数
    static bool isValidScanPath(const std::string& inputPath);

private:
    static std::vector<std::string> allowedRoots; // 已标准化的白名单根目录
};

#endif // PATH_UTIL_H
