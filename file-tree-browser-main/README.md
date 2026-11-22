# 文件目录树浏览器（C++ + Vue3 前后端分离版）

将磁盘文件与目录结构抽象为树状数据结构，实现扫描建树、多方式遍历展示、条件查找、统计分析、增删改、权限管理、Merkle 完整性校验与目录环检测。

## 1. 架构

| 层 | 技术 | 目录 |
|---|---|---|
| 前端 | Vue3 + TypeScript + Vite + Element Plus + Pinia + Axios | `web/` |
| 服务端 | C++17 + Drogon（HTTP/JSON） | `server/` |
| 业务核心 | C++17 + STL（手写树 / 图 / 哈希 / 堆） | `backend/` |
| 数据 | 本地文件 `data/users.dat` | `data/` |

调用方向为单向：`web → server → backend`，`server` 只做参数解析、权限校验与响应封装，`backend` 不感知 HTTP。

## 2. 目录结构

```
├── CMakeLists.txt        # 顶层构建：tree_backend 静态库 + dir_tree_server 可执行
├── backend/              # 业务核心（数据结构与算法，无 UI / 无 HTTP）
├── server/               # Drogon 服务层：Controller、会话、路径解析
├── web/                  # Vue3 前端工程
├── 3rdparty/             # json.hpp、picosha2.h（header-only）
├── data/                 # 用户表持久化
├── tests/                # 后端单元测试 + API 测试
└── docs/                 # 需求、技术栈、接口文档
```

## 3. 环境要求

- Visual Studio 2022（v143）或 MSVC 生成工具，CMake 3.16+
- vcpkg（用于安装 Drogon）
- Node.js 18+、npm 9+

## 4. 构建与运行

```bash
# 1. 安装 Drogon（vcpkg）
vcpkg install drogon:x64-windows

# 2. 构建后端
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# 3. 启动服务（默认 8080）
./build/Release/dir_tree_server

# 4. 启动前端开发服务器（默认 5173，/api 已代理到 8080）
cd web && npm install && npm run dev
```

前端生产构建：`cd web && npm run build`，产物在 `web/dist`，可由 Drogon 的 `document_root` 直接托管。

## 5. 文档

- `docs/文件目录树浏览器_系统需求分析文档.docx`
- `docs/技术栈定版与开发规范.docx`
- `docs/模块接口对接文档.docx`
- `docs/小组分工表.md`
