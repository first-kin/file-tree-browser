#ifndef API_RESULT_H
#define API_RESULT_H

#include "../../3rdparty/json.hpp"
#include <string>

using namespace std;
using json = nlohmann::json;

// api_result.h —— 统一 HTTP 响应封装
// 所有接口返回格式统一为 {code, message, data}
// code: 0 成功，负数为错误码（见技术栈文档 3.5 节）
class ApiResult
{
public:
    // 功能：成功响应，附带数据
    // 参数：data-业务数据（json 对象、数组或任意值）
    // 返回值：{code:0, message:"成功", data:data}
    static json ok(const json& data)
    {
        json j;
        j["code"] = 0;
        j["message"] = "成功";
        j["data"] = data;
        return j;
    }

    // 功能：成功响应，无数据
    // 参数：无
    // 返回值：{code:0, message:"成功", data:null}
    static json ok()
    {
        json j;
        j["code"] = 0;
        j["message"] = "成功";
        j["data"] = nullptr;
        return j;
    }

    // 功能：失败响应
    // 参数：code-错误码，message-错误提示
    // 返回值：{code:code, message:message, data:null}
    static json error(int code, const string& message)
    {
        json j;
        j["code"] = code;
        j["message"] = message;
        j["data"] = nullptr;
        return j;
    }

    // 功能：参数非法（-12）快捷方法
    // 参数：message-具体提示
    // 返回值：{code:-12, message, data:null}
    static json errorParam(const string& message = "参数非法")
    {
        return error(-12, message);
    }

    // 功能：未登录/token 失效（-11）快捷方法
    // 参数：message-具体提示
    // 返回值：{code:-11, message, data:null}
    static json errorNoAuth(const string& message = "未登录或登录已过期")
    {
        return error(-11, message);
    }

    // 功能：权限不足（-9）快捷方法
    // 参数：message-具体提示
    // 返回值：{code:-9, message, data:null}
    static json errorForbidden(const string& message = "权限不足")
    {
        return error(-9, message);
    }
};

#endif // API_RESULT_H
