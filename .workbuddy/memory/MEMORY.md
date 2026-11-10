# 项目长期记忆 · 文件目录树浏览器（D:/git/--）

## 定版架构（全员冻结）
- 前后端分离 B/S：web(Vue3+TS+Vite+Element Plus+Pinia+Axios) → server(Drogon C++17, 薄控制器) → backend(纯 C++ 手写数据结构，无 HTTP)
- 构建：CMake（tree_backend 静态库 + dir_tree_server），Drogon 经 vcpkg；Node 18+/npm 9+
- 6 人分工：后端3（组长=树/建树/Merkle/服务骨架、A=遍历查找统计、B=增删改权限环检测）/ 前端2（C=框架路由请求层登录、D=功能页面组件）/ 测试1
- 错误码 0～-12（-11 token 失效、-12 参数非法为 B/S 版新增）

## 文档体系（v2.0 B/S 版，2026-11-03 完成对齐）
- 桌面提交文档 4 份已填充/升级：任务书、需求分析（旧版备份在桌面 docx_backup_2026-11-03/）、设计文档、总结（6 人小节+提示，个人心得待填）
- docs/ 协同文档：技术栈定版、模块接口对接（v2.0，含 REST API 明细第4章）、小组分工表.md（前后端分离版）、开发任务分配文档.docx（v1.0，37 任务卡 / 84 人日 / 关键路径）
- 文档源：_tmp/src_req.md、src_tech.md、src_api.md、src_task.md（管道格式）；构建工具 _tmp/build_docx.py（勿用 .ps1，PS5.1 ZipArchive 兼容 bug）；单份构建用 pairs_task.txt
- 任务编号规则：T-阶段+序号（0 启动/1 基建/2 后端/3 前端/4 联调/5 P2/6 收尾），进度统计口径见任务分配文档 9.3

## 关键经验
- editor_sdk：doc_find 的 locations 倒序返回；markdown 插入吃连续下划线；find_and_replace 前缀替换留残余需清理；覆盖 docx 前先 close_file
- 代码文件全部为 0 字节占位：文件架构已定，代码未开始（用户要求只搭架构不写代码）
- tests/api/api_test.http 与各 .h/.cpp 待开发
