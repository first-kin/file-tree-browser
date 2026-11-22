#ifndef STATS_CONTROLLER_H
#define STATS_CONTROLLER_H

#include <drogon/drogon.h>

// stats_controller.h —— 统计相关 HTTP 接口
class StatsController : public drogon::HttpController<StatsController>
{
public:
    METHOD_LIST_BEGIN
    METHOD_ADD(StatsController::count, "/api/stats/count", Get);
    METHOD_ADD(StatsController::totalSize, "/api/stats/total-size", Get);
    METHOD_ADD(StatsController::maxDepth, "/api/stats/max-depth", Get);
    METHOD_ADD(StatsController::topN, "/api/stats/top-n", Get);
    METHOD_ADD(StatsController::emptyDirs, "/api/stats/empty-dirs", Get);
    METHOD_ADD(StatsController::byLevel, "/api/stats/by-level", Get);
    METHOD_LIST_END

    // 统计文件数和目录数
    void count(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 目录总大小
    void totalSize(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 最大深度
    void maxDepth(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 最大的 N 个文件
    void topN(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 空目录检测
    void emptyDirs(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 每层节点数
    void byLevel(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

#endif // STATS_CONTROLLER_H
