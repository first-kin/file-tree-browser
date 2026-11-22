// test_operations.cpp —— 内存树操作模块独立单元测试（纯 C++，不依赖 Drogon）
// 注意：本文件含独立 main，需单独编译运行；若把 tests/*.cpp 一次性 GLOB 进同一个
// 可执行目标，会与 test_auth.cpp 的 main 冲突，建议每个测试文件建独立目标。

#include "../backend/include/operations.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace std;

// 断言失败时打印模块名、原因与行号，并在 NDEBUG 下也能终止进程
#define CHECK(condition, module, reason)                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            cerr << "[" << module << "] " << reason                             \
                 << " (line " << __LINE__ << ")" << endl;                       \
            assert(condition);                                                  \
            abort();                                                            \
        }                                                                       \
    } while (0)

int main()
{
    // 1. 新建根节点
    TreeNode root("root", true);

    // 2. 创建子节点，验证 findChild 可按名称找到
    CHECK(addNode(&root, "docs", true) == 0,
          "test_operations", "创建目录 docs 失败");
    CHECK(addNode(&root, "readme.txt", false) == 0,
          "test_operations", "创建文件 readme.txt 失败");

    TreeNode* docs = root.findChild("docs");
    TreeNode* readme = root.findChild("readme.txt");
    CHECK(docs != nullptr,
          "test_operations", "findChild 未找到 docs");
    CHECK(readme != nullptr,
          "test_operations", "findChild 未找到 readme.txt");
    CHECK(!readme->isDir && readme->ext == ".txt",
          "test_operations", "文件节点类型或扩展名不正确");

    // 3. 同级重名创建应返回 -7
    CHECK(addNode(&root, "docs", true) == -7,
          "test_operations", "重名创建应返回 -7");

    // 准备环检测数据：docs 下创建子目录 sub
    CHECK(addNode(docs, "sub", true) == 0,
          "test_operations", "创建 docs/sub 失败");
    TreeNode* sub = docs->findChild("sub");
    CHECK(sub != nullptr,
          "test_operations", "findChild 未找到 docs/sub");

    // 4. 删除与撤销：删除后节点从树上消失，撤销后同一节点恢复
    CHECK(removeNode(readme) == 0,
          "test_operations", "删除 readme.txt 失败");
    CHECK(root.findChild("readme.txt") == nullptr,
          "test_operations", "删除后 readme.txt 仍挂在树上");

    CHECK(undoLast() == 0,
          "test_operations", "撤销删除失败");
    TreeNode* restored = root.findChild("readme.txt");
    CHECK(restored != nullptr,
          "test_operations", "撤销后 readme.txt 未恢复");
    CHECK(restored == readme,
          "test_operations", "撤销恢复的应是原节点而非新副本");

    // 5. 防环移动：docs 移到自己或自己的子目录 sub 下均应返回 -8
    CHECK(moveNode(docs, sub) == -8,
          "test_operations", "移动到自身子目录应返回 -8");
    CHECK(moveNode(docs, docs) == -8,
          "test_operations", "移动到自身应返回 -8");
    CHECK(docs->parent == &root,
          "test_operations", "非法移动失败后 docs 的父节点不应改变");

    cout << "test_operations passed!" << endl;
    return 0;
}
