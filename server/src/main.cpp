#include <drogon/drogon.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <string>

using namespace drogon;
using namespace std;

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

namespace
{
const filesystem::path FRONTEND_ENTRY = filesystem::path("../web/dist/index.html");

void serveFrontendEntry(const HttpRequestPtr&,
                        function<void(const HttpResponsePtr&)>&& callback)
{
    if (!filesystem::exists(FRONTEND_ENTRY))
    {
        auto response = HttpResponse::newHttpResponse();
        response->setStatusCode(k503ServiceUnavailable);
        response->setContentTypeCode(CT_TEXT_PLAIN);
        response->setBody("Frontend build is missing. Run npm ci && npm run build in web/.");
        callback(response);
        return;
    }

    auto response = HttpResponse::newFileResponse(FRONTEND_ENTRY.string());
    response->setStatusCode(k200OK);
    response->addHeader("Cache-Control", "no-cache");
    callback(response);
}
}

int main()
{
    const filesystem::path configPath = "config.json";
    if (!filesystem::exists(configPath))
    {
        cerr << "Missing server/config.json. Start dir_tree_server with server/ as working directory.\n";
        return 1;
    }

    auto& application = app();
    application.loadConfigFile(configPath.string());

    // Drogon 静态文件托管负责 /assets/*；这些精确路由负责 SPA history 刷新回退。
    constexpr array<const char*, 9> frontendRoutes = {
        "/", "/login", "/register", "/tree", "/search",
        "/stats", "/users", "/merkle", "/graph"
    };
    for (const char* route : frontendRoutes)
    {
        application.registerHandler(route, serveFrontendEntry, {Get});
    }

    cout << "Dir Tree Browser is starting on http://127.0.0.1:8080\n";
    application.run();
    return 0;
}
