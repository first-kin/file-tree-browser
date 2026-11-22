#include "../include/search_controller.h"
#include "../../backend/include/search.h"
#include "../../backend/include/tree_node.h"
#include "../include/api_result.h"
#include "../include/session_manager.h"
#include "../../3rdparty/json.hpp"

using namespace drogon;
using namespace std;
using json = nlohmann::json;

// 从请求头提取 token（Authorization: Bearer xxx）
static string extractToken(const HttpRequestPtr& req)
{
    string auth = req->getHeader("Authorization");
    if (auth.size() > 7 && auth.substr(0, 7) == "Bearer ")
    {
        return auth.substr(7);
    }
    return "";
}

// 将节点列表转为 JSON 数组
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

// 按名称精确查找
void SearchController::searchByName(const HttpRequestPtr& req,
                                    function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::GUEST))
    {
        auto resp = HttpResponse::newHttpJsonResponse(ApiResult::errorNoAuth());
        callback(resp);
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        auto resp = HttpResponse::newHttpJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描"));
        callback(resp);
        return;
    }

    auto body = req->getJsonBody();
    if (body == nullptr || !body->isMember("name"))
    {
        auto resp = HttpResponse::newHttpJsonResponse(ApiResult::errorParam("缺少 name 参数"));
        callback(resp);
        return;
    }

    string name = (*body)["name"].asString();
    TreeNode* found = findByName(root, name);

    json data;
    if (found != nullptr)
    {
        data["name"] = found->name;
        data["isDir"] = found->isDir;
        data["size"] = found->size;
        data["ext"] = found->ext;
        data["modifyTime"] = found->modifyTime;
    }
    else
    {
        data = nullptr;
    }

    auto resp = HttpResponse::newHttpJsonResponse(ApiResult::ok(data));
    callback(resp);
}

// 通配符查找
void SearchController::searchByPattern(const HttpRequestPtr& req,
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

    auto body = req->getJsonBody();
    if (body == nullptr || !body->isMember("pattern"))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorParam("缺少 pattern 参数")));
        return;
    }

    string pattern = (*body)["pattern"].asString();
    vector<TreeNode*> results = findByPattern(root, pattern);

    auto resp = HttpResponse::newHttpJsonResponse(ApiResult::ok(nodesToJson(results)));
    callback(resp);
}

// 按扩展名筛选
void SearchController::searchByExt(const HttpRequestPtr& req,
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

    auto body = req->getJsonBody();
    if (body == nullptr || !body->isMember("ext"))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorParam("缺少 ext 参数")));
        return;
    }

    string ext = (*body)["ext"].asString();
    vector<TreeNode*> results = findByExt(root, ext);

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(nodesToJson(results))));
}

// 按大小范围查找
void SearchController::searchBySize(const HttpRequestPtr& req,
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

    auto body = req->getJsonBody();
    if (body == nullptr || !body->isMember("minSize") || !body->isMember("maxSize"))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorParam("缺少 minSize 或 maxSize 参数")));
        return;
    }

    long long minSize = (*body)["minSize"].asInt64();
    long long maxSize = (*body)["maxSize"].asInt64();
    vector<TreeNode*> results = findBySize(root, minSize, maxSize);

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(nodesToJson(results))));
}

// 按修改时间范围查找
void SearchController::searchByTime(const HttpRequestPtr& req,
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

    auto body = req->getJsonBody();
    if (body == nullptr || !body->isMember("begin") || !body->isMember("end"))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorParam("缺少 begin 或 end 参数")));
        return;
    }

    time_t begin = static_cast<time_t>((*body)["begin"].asInt64());
    time_t end = static_cast<time_t>((*body)["end"].asInt64());
    vector<TreeNode*> results = findByTime(root, begin, end);

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(nodesToJson(results))));
}

// 重复文件检测
void SearchController::searchDuplicates(const HttpRequestPtr& req,
                                        function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::USER))
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::errorForbidden("需要登录用户权限")));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(HttpResponse::newHttpJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }

    vector<vector<TreeNode*>> groups = findDuplicates(root);

    json data = json::array();
    for (const auto& group : groups)
    {
        data.push_back(nodesToJson(group));
    }

    callback(HttpResponse::newHttpJsonResponse(ApiResult::ok(data)));
}
