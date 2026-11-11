#ifndef SEARCH_CONTROLLER_H
#define SEARCH_CONTROLLER_H

#include <drogon/drogon.h>

// search_controller.h —— 查找相关 HTTP 接口
class SearchController : public drogon::HttpController<SearchController>
{
public:
    METHOD_LIST_BEGIN
    METHOD_ADD(SearchController::searchByName, "/api/search/name", Post);
    METHOD_ADD(SearchController::searchByPattern, "/api/search/pattern", Post);
    METHOD_ADD(SearchController::searchByExt, "/api/search/ext", Post);
    METHOD_ADD(SearchController::searchBySize, "/api/search/size", Post);
    METHOD_ADD(SearchController::searchByTime, "/api/search/time", Post);
    METHOD_ADD(SearchController::searchDuplicates, "/api/search/duplicates", Get);
    METHOD_LIST_END

    // 按名称精确查找
    void searchByName(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 通配符查找
    void searchByPattern(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 按扩展名筛选
    void searchByExt(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 按大小范围查找
    void searchBySize(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 按修改时间范围查找
    void searchByTime(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // 重复文件检测
    void searchDuplicates(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

#endif // SEARCH_CONTROLLER_H
