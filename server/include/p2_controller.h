#ifndef P2_CONTROLLER_H
#define P2_CONTROLLER_H

#include <drogon/drogon.h>

// p2_controller.h —— P2 高级功能 HTTP 接口（环检测部分由组员B实现）
// 路由按《模块接口对接文档》第 13.6 节
// 复杂度说明：控制器只做鉴权、调用 backend 与响应封装，
// 建图/DFS 复杂度见 backend/graph_detect.h。
class P2Controller : public drogon::HttpController<P2Controller>
{
public:
    METHOD_LIST_BEGIN
    METHOD_ADD(P2Controller::handleBuildGraph, "/api/graph/build", Post);
    METHOD_ADD(P2Controller::handleGraphCycles, "/api/graph/cycles", Get);
    METHOD_LIST_END

    // 功能：对当前会话目录树建立链接图
    // 参数：req-HTTP 请求（Authorization 头）
    // 返回值：无（data 为 {nodeCount, edgeCount}）
    void handleBuildGraph(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 功能：获取最近一次建图检测到的所有环路径
    // 参数：req-HTTP 请求（Authorization 头）
    // 返回值：无（data 为 {cycles:[[名称...], ...]}）
    void handleGraphCycles(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

#endif // P2_CONTROLLER_H
