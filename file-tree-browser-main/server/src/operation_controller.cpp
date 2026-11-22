#include "../include/operation_controller.h"
#include "../../backend/include/operations.h"
#include "../../backend/include/tree_node.h"
#include "../include/api_result.h"
#include "../include/session_manager.h"
#include "../include/path_util.h"

#include <string>

using namespace drogon;
using namespace std;

// ===== 内部辅助函数 =====

// 读取 JSON 字符串字段：字段必须存在且为字符串
static bool readStringField(const shared_ptr<Json::Value>& body, const char* key, string& value)
{
    if (body == nullptr || !body->isMember(key))
    {
        return false;
    }
    const Json::Value& field = (*body)[key];
    if (!field.isString())
    {
        return false;
    }
    value = field.asString();
    return true;
}

// 读取 JSON 布尔字段：字段必须存在且为 bool
static bool readBoolField(const shared_ptr<Json::Value>& body, const char* key, bool& value)
{
    if (body == nullptr || !body->isMember(key))
    {
        return false;
    }
    const Json::Value& field = (*body)[key];
    if (!field.isBool())
    {
        return false;
    }
    value = field.asBool();
    return true;
}

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

// 路径解析失败时的统一错误响应：-3 越界，-2 不存在
static HttpResponsePtr pathErrorResponse(int code)
{
    return makeJsonResponse(ApiResult::error(code, code == -3 ? "路径越界或非法" : "路径不存在"));
}

// 当前扫描根是否就是 workspace 目录（根名允许是纯名称或完整路径）
static bool isWorkspaceRoot(TreeNode* root)
{
    if (root == nullptr)
    {
        return false;
    }
    string name = root->name;
    while (!name.empty() && (name.back() == '/' || name.back() == '\\'))
    {
        name.pop_back();
    }
    size_t pos = name.find_last_of("/\\");
    if (pos != string::npos)
    {
        name = name.substr(pos + 1);
    }
    return name == "workspace";
}

// 首次磁盘操作前自动把 workspace 扫描树绑定到磁盘根；
// 其他目录的树没有绑定信息，直接返回 -10。
static bool ensureDiskBinding(TreeNode* root)
{
    if (isDiskBoundFor(root))
    {
        return true;
    }
    if (!isWorkspaceRoot(root))
    {
        return false;
    }
    return bindDiskRoot(root, "workspace") == 0;
}

// 绑定失败时的统一提示
static HttpResponsePtr diskBindingErrorResponse()
{
    return makeJsonResponse(ApiResult::error(
        -10, "当前目录树未绑定磁盘根目录：仅支持扫描 workspace 后执行磁盘操作"));
}

// 底层错误码转中文提示
static string operationErrorMessage(int code)
{
    switch (code)
    {
    case -1:
        return "节点不存在或操作不允许";
    case -2:
        return "路径不存在";
    case -3:
        return "路径越界或非法";
    case -7:
        return "同级已存在同名项";
    case -8:
        return "不能移动到或复制到自身及其子孙目录";
    case -12:
        return "名称或参数非法";
    default:
        return "操作失败";
    }
}

// ===== 接口实现 =====

// 创建节点：parentPath 必须是已存在的目录
void OperationController::handleCreate(const HttpRequestPtr& req,
                                       function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::USER))
    {
        callback(makeJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }
    if (!ensureDiskBinding(root))
    {
        callback(diskBindingErrorResponse());
        return;
    }

    auto body = req->getJsonObject();
    string parentPath;
    string name;
    bool isDir = false;
    if (!readStringField(body, "parentPath", parentPath)
        || !readStringField(body, "name", name)
        || !readBoolField(body, "isDir", isDir))
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 parentPath、name 或 isDir 参数")));
        return;
    }

    TreeNode* parent = nullptr;
    int pathCode = resolveNodePath(root, parentPath, &parent);
    if (pathCode != 0)
    {
        callback(pathErrorResponse(pathCode));
        return;
    }
    if (!parent->isDir)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "父节点不是目录")));
        return;
    }

    int code = ::addNode(parent, name, isDir);
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, operationErrorMessage(code))));
        return;
    }

    TreeNode* created = parent->findChild(name);
    json data;
    data["path"] = (created != nullptr) ? nodeToPath(created) : parentPath + "/" + name;
    callback(makeJsonResponse(ApiResult::ok(data)));
}

// 删除节点：必须传 confirm=true，路径使用根相对路径
void OperationController::handleDelete(const HttpRequestPtr& req,
                                       function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::USER))
    {
        callback(makeJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }
    if (!ensureDiskBinding(root))
    {
        callback(diskBindingErrorResponse());
        return;
    }

    auto body = req->getJsonObject();
    string path;
    bool confirm = false;
    if (!readStringField(body, "path", path) || path.empty())
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 path 参数")));
        return;
    }
    if (!readBoolField(body, "confirm", confirm) || !confirm)
    {
        callback(makeJsonResponse(ApiResult::errorParam("删除操作需要 confirm=true 二次确认")));
        return;
    }

    TreeNode* target = nullptr;
    int pathCode = resolveNodePath(root, path, &target);
    if (pathCode != 0)
    {
        callback(pathErrorResponse(pathCode));
        return;
    }
    if (target == root)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "根节点不可删除")));
        return;
    }

    int code = ::removeNode(target);
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, operationErrorMessage(code))));
        return;
    }

    callback(makeJsonResponse(ApiResult::ok()));
}

// 重命名节点
void OperationController::handleRename(const HttpRequestPtr& req,
                                        function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::USER))
    {
        callback(makeJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }
    if (!ensureDiskBinding(root))
    {
        callback(diskBindingErrorResponse());
        return;
    }

    auto body = req->getJsonObject();
    string path;
    string newName;
    if (!readStringField(body, "path", path) || path.empty())
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 path 参数")));
        return;
    }
    if (!readStringField(body, "newName", newName) || newName.empty())
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 newName 参数")));
        return;
    }

    TreeNode* target = nullptr;
    int pathCode = resolveNodePath(root, path, &target);
    if (pathCode != 0)
    {
        callback(pathErrorResponse(pathCode));
        return;
    }

    int code = ::renameNode(target, newName);
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, operationErrorMessage(code))));
        return;
    }

    json data;
    data["path"] = nodeToPath(target);
    callback(makeJsonResponse(ApiResult::ok(data)));
}

// 移动节点：path 可以是路径，newParentPath 目标父目录
void OperationController::handleMove(const HttpRequestPtr& req,
                                      function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::USER))
    {
        callback(makeJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }
    if (!ensureDiskBinding(root))
    {
        callback(diskBindingErrorResponse());
        return;
    }

    auto body = req->getJsonObject();
    string path;
    string newParentPath;
    if (!readStringField(body, "path", path) || path.empty())
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 path 参数")));
        return;
    }
    if (!readStringField(body, "newParentPath", newParentPath))
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 newParentPath 参数")));
        return;
    }

    TreeNode* node = nullptr;
    int pathCode = resolveNodePath(root, path, &node);
    if (pathCode != 0)
    {
        callback(pathErrorResponse(pathCode));
        return;
    }

    TreeNode* newParent = nullptr;
    pathCode = resolveNodePath(root, newParentPath, &newParent);
    if (pathCode != 0)
    {
        callback(pathErrorResponse(pathCode));
        return;
    }

    int code = ::moveNode(node, newParent);
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, operationErrorMessage(code))));
        return;
    }

    json data;
    data["path"] = nodeToPath(node);
    callback(makeJsonResponse(ApiResult::ok(data)));
}

// 复制节点：深拷贝到新父目录
void OperationController::handleCopy(const HttpRequestPtr& req,
                                      function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::USER))
    {
        callback(makeJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }
    if (!ensureDiskBinding(root))
    {
        callback(diskBindingErrorResponse());
        return;
    }

    auto body = req->getJsonObject();
    string path;
    string newParentPath;
    if (!readStringField(body, "path", path) || path.empty())
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 path 参数")));
        return;
    }
    if (!readStringField(body, "newParentPath", newParentPath))
    {
        callback(makeJsonResponse(ApiResult::errorParam("缺少 newParentPath 参数")));
        return;
    }

    TreeNode* node = nullptr;
    int pathCode = resolveNodePath(root, path, &node);
    if (pathCode != 0)
    {
        callback(pathErrorResponse(pathCode));
        return;
    }

    TreeNode* newParent = nullptr;
    pathCode = resolveNodePath(root, newParentPath, &newParent);
    if (pathCode != 0)
    {
        callback(pathErrorResponse(pathCode));
        return;
    }

    int code = ::copyNode(node, newParent);
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, operationErrorMessage(code))));
        return;
    }

    TreeNode* copied = newParent->findChild(node->name);
    json data;
    data["path"] = (copied != nullptr) ? nodeToPath(copied) : nodeToPath(newParent);
    callback(makeJsonResponse(ApiResult::ok(data)));
}

// 撤销：底层历史栈为权威，只取消链接/恢复链接
void OperationController::handleUndo(const HttpRequestPtr& req,
                                      function<void(const HttpResponsePtr&)>&& callback)
{
    string token = extractToken(req);
    if (!SessionManager::instance().checkPermission(token, Role::USER))
    {
        callback(makeJsonResponse(ApiResult::errorNoAuth()));
        return;
    }

    TreeNode* root = SessionManager::instance().getTreeRoot(token);
    if (root == nullptr)
    {
        callback(makeJsonResponse(ApiResult::error(-1, "尚未构建目录树，请先扫描")));
        return;
    }
    if (!ensureDiskBinding(root))
    {
        callback(diskBindingErrorResponse());
        return;
    }

    int code = ::undoLast();
    if (code != 0)
    {
        callback(makeJsonResponse(ApiResult::error(code, "没有可撤销的操作")));
        return;
    }

    callback(makeJsonResponse(ApiResult::ok()));
}
