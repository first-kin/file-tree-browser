#ifndef OPERATION_CONTROLLER_H
#define OPERATION_CONTROLLER_H

#include <drogon/drogon.h>

// operation_controller.h —— 节点增删改、复制与撤销 HTTP 接口
// 严格按《模块接口对接文档》第 13.5 节注册路由
// 复杂度说明：控制器只做参数解析、鉴权、调用 backend 与响应封装，
// 路径解析复杂度见 server/path_util.h，其余业务复杂度见 backend/operations.h。
class OperationController : public drogon::HttpController<OperationController>
{
public:
    METHOD_LIST_BEGIN
    METHOD_ADD(OperationController::handleCreate, "/api/nodes", Post);
    METHOD_ADD(OperationController::handleDelete, "/api/nodes", Delete);
    METHOD_ADD(OperationController::handleRename, "/api/nodes/rename", Put);
    METHOD_ADD(OperationController::handleMove, "/api/nodes/move", Post);
    METHOD_ADD(OperationController::handleCopy, "/api/nodes/copy", Post);
    METHOD_ADD(OperationController::handleUndo, "/api/operations/undo", Post);
    METHOD_LIST_END

    // 功能：在指定父目录下创建节点
    // 参数：req-JSON Body 含 parentPath、name、isDir
    // 返回值：无（data 为 {path}）
    void handleCreate(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：删除节点（必须传 confirm=true，二次确认）
    // 参数：req-JSON Body 含 path、confirm
    // 返回值：无（成功 data 为 null）
    void handleDelete(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：重命名节点
    // 参数：req-JSON Body 含 path、newName
    // 返回值：无（data 为 {path}）
    void handleRename(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：移动节点到新父目录
    // 参数：req-JSON Body 含 path、newParentPath
    // 返回值：无（data 为 {path}）
    void handleMove(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：复制节点到新父目录
    // 参数：req-JSON Body 含 path、newParentPath
    // 返回值：无（data 为 {path}）
    void handleCopy(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：撤销最近一次操作（只取消链接/恢复链接，不删除节点）
    // 参数：req-HTTP 请求（无业务参数）
    // 返回值：无（成功 data 为 null）
    void handleUndo(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

#endif // OPERATION_CONTROLLER_H
