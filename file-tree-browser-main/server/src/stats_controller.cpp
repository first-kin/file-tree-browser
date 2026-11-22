#include "../include/stats_controller.h"
#include "../../backend/include/stats.h"
#include "../../backend/include/tree_node.h"
#include "../include/api_result.h"
#include "../include/session_manager.h"
#include "../../3rdparty/json.hpp"

using namespace drogon;
using namespace std;
using json = nlohmann::json;

// 从请求头提取 token
static string extractToken(const HttpRequestPtr& req)
{
    string auth = req->getHeader("Authorization");
    if (auth.size() > 7 && auth.substr(0, 7) == "Bearer ")
    {
        return auth.substr(7);
    }
    return "";
}

// 节点列表转 JSON
static json nodesToJson(const vector<TreeNode*>& nodes)
{
    json arr = json::array();
    for (TreeNode* node : nodes)
    {
        json j;
        j["name"] = node->name;
        j["isDir"] = node->isDir;
        j["size"] = node->size;
        j["ext"] = node->ext;
        j["modifyTime"] = node->modifyTime;
        arr.push_back(j);
    }
    return arr;
}

// 统计文件数和目录数
void StatsController::count(const HttpRequestPtr& req,
                            function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::GUEST))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }

    int fileCount = 0, dirCount = 0;
    countNodes(root, fileCount, dirCount);

    json data;
    data["fileCount"] = fileCount;
    data["dirCount"] = dirCount;
    data["total"] = fileCount + dirCount;

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(data)));
}

// 目录总大小
void StatsController::totalSize(const HttpRequestPtr& req,
                                function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::GUEST))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }

    long long total = calcTotalSize(root);

    json data;
    data["totalSize"] = total;

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(data)));
}

// 最大深度
void StatsController::maxDepth(const HttpRequestPtr& req,
                               function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::GUEST))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }

    int depth = maxDepth(root);

    json data;
    data["maxDepth"] = depth;

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(data)));
}

// 最大的 N 个文件
void StatsController::topN(const HttpRequestPtr& req,
                           function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::GUEST))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }

    string nStr = req->getParameter("n");
    int n = 10;
    if (!nStr.empty())
    {
        n = stoi(nStr);
    }

    vector<TreeNode*> results = topNFiles(root, n);

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(nodesToJson(results))));
}

// 空目录检测
void StatsController::emptyDirs(const HttpRequestPtr& req,
                                function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::GUEST))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }

    vector<TreeNode*> results = findEmptyDirs(root);

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(nodesToJson(results))));
}

// 每层节点数
void StatsController::byLevel(const HttpRequestPtr& req,
                              function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::GUEST))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }

    vector<int> counts = countByLevel(root);

    json data = json::array();
    for (int c : counts)
    {
        data.push_back(c);
    }

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(data)));
}
