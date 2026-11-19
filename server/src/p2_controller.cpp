#include "../include/p2_controller.h"
#include "../../backend/include/graph_detect.h"
#include "../../backend/include/tree_node.h"
#include "../include/api_result.h"
#include "../include/session_manager.h"

#include <string>

using namespace drogon;
using namespace std;

// 从请求头提取 Bearer token
static string extractToken(const HttpRequestPtr& req)
{
    string auth = req->getHeader("Authorization");
    if (auth.size() > 7 && auth.substr(0, 7) == "Bearer ")
    {
        return auth.substr(7);
    }
    return "";
}

// 把组长 api_result 产生的 nlohmann JSON 序列化为 Drogon 响应
static HttpResponsePtr makeJsonResponse(const json& body)
{
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(body.dump());
    return resp;
}

// 管理员校验：0 通过，-11 未登录，-9 非管理员
static int checkAdmin(const HttpRequestPtr& req)
{
    string token = extractToken(req);
    Session* session = SessionManager::instance().getSession(token);
    if (session == nullptr)
    {
        return -11;
    }
    if (session->role != Role::ADMIN)
    {
        return -9;
    }
    return 0;
}

// 建图：P2 功能仅管理员可用，使用当前会话的目录树
void P2Controller::handleBuildGraph(const HttpRequestPtr& req,
                                    function<void(const HttpResponsePtr&)>&& callback)
{
    int authCode = checkAdmin(req);
    if (authCode != 0)
    {
        callback(makeJsonResponse(ApiResult::error(
            authCode, authCode == -11 ? "未登录或会话已失效" : "权限不足，仅管理员可操作")));
        return;
    }

    string token = extractToken(req);
    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }

    int code = ::buildGraph(root);
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, "建图失败：目录树为空或根节点非法")));
        return;
    }

    json data;
    data["nodeCount"] = getGraphNodeCount();
    data["edgeCount"] = getGraphEdgeCount();
    callback(makeJsonResponse(ApiResult::ok(data)));
}

// 环路径：返回当前图上的所有环
void P2Controller::handleGraphCycles(const HttpRequestPtr& req,
                                     function<void(const HttpResponsePtr&)>&& callback)
{
    int authCode = checkAdmin(req);
    if (authCode != 0)
    {
        callback(makeJsonResponse(ApiResult::error(
            authCode, authCode == -11 ? "未登录或会话已失效" : "权限不足，仅管理员可操作")));
        return;
    }

    json data;
    data["cycles"] = getCyclePaths();
    callback(makeJsonResponse(ApiResult::ok(data)));
}
