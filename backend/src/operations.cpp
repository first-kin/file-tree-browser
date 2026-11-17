#include "../include/operations.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
using namespace std;

// ===== 模块内部数据结构 =====

// 撤销记录：parent/prevSibling 用于恢复内存位置；
// diskPath/trashPath/newDiskPath 用于恢复磁盘位置。
struct UndoRecord
{
    int opType;
    TreeNode* target;
    string backup;
    TreeNode* parent;
    TreeNode* prevSibling;
    string diskPath;
    string trashPath;
    string newDiskPath;
};

// 操作历史栈（后进先出）
static vector<UndoRecord> g_history;

// 已从树上摘除但尚未清理的子树
struct DetachedPool
{
    vector<TreeNode*> nodes;

    ~DetachedPool()
    {
        for (TreeNode* node : nodes)
        {
            delete node;
        }
    }
};
static DetachedPool g_detached;

// 单机版仍可能收到同一账号的并发请求，用互斥量保护历史栈、磁盘绑定与workspace.trash 目录
static mutex g_mutex;

// 磁盘根绑定：单机版只维护一份
static TreeNode* g_boundRoot = nullptr;
static fs::path g_boundRootPath;
static fs::path g_trashPath;
static vector<string> g_trashPaths;

// ===== 内部辅助函数 =====

// 校验节点名称：非空、非 "."/".."、不含路径分隔符、不含 Windows 非法字符
static bool isValidName(const string& name)
{
    if (name.empty() || name.size() > 255 || name == "." || name == "..")
    {
        return false;
    }
    for (char c : name)
    {
        if (c == '/' || c == '\\' || c == '\0')
        {
            return false;
        }
    }
    if (name.find_first_of("<>:\"|?*") != string::npos)
    {
        return false;
    }
    if (name.back() == ' ' || name.back() == '.')
    {
        return false;
    }

    // Windows 保留设备名（不区分大小写，扩展名前的主干命中即拒绝）
    string base = name.substr(0, name.find('.'));
    for (char& c : base)
    {
        if (c >= 'a' && c <= 'z')
        {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    if (base == "CON" || base == "PRN" || base == "AUX" || base == "NUL")
    {
        return false;
    }
    if (base.size() == 4 && (base.compare(0, 3, "COM") == 0 || base.compare(0, 3, "LPT") == 0)
        && base[3] >= '1' && base[3] <= '9')
    {
        return false;
    }

    return true;
}

// 由文件名推导扩展名；以 '.' 开头的隐藏文件视为无扩展名
static string extensionOf(const string& name)
{
    size_t pos = name.rfind('.');
    if (pos == string::npos || pos == 0)
    {
        return "";
    }
    return name.substr(pos);
}

// 判断 ancestor 是否为 node 的祖先（含 node 自身）
static bool isAncestorOf(const TreeNode* ancestor, const TreeNode* node)
{
    const TreeNode* cur = node;
    while (cur != nullptr)
    {
        if (cur == ancestor)
        {
            return true;
        }
        cur = cur->parent;
    }
    return false;
}

// 判断 node 是否为 parent 的直接孩子
static bool isChildOf(const TreeNode* parent, const TreeNode* node)
{
    for (const TreeNode* child = parent->firstChild; child != nullptr; child = child->nextSibling)
    {
        if (child == node)
        {
            return true;
        }
    }
    return false;
}

// 取 node 在 parent 孩子链表中的前驱；找不到或本来就是第一个孩子时返回 nullptr
static TreeNode* prevSiblingOf(TreeNode* parent, TreeNode* node)
{
    TreeNode* prev = nullptr;
    for (TreeNode* child = parent->firstChild; child != nullptr; child = child->nextSibling)
    {
        if (child == node)
        {
            return prev;
        }
        prev = child;
    }
    return nullptr;
}

// 把 node 插到 parent 的 prev 之后；prev 为空表示插到链表头
static void insertAfter(TreeNode* parent, TreeNode* prev, TreeNode* node)
{
    if (parent == nullptr || node == nullptr)
    {
        return;
    }

    node->parent = parent;
    node->nextSibling = nullptr;

    if (prev != nullptr && isChildOf(parent, prev))
    {
        node->nextSibling = prev->nextSibling;
        prev->nextSibling = node;
        return;
    }

    node->nextSibling = parent->firstChild;
    parent->firstChild = node;
}

// 深拷贝单棵子树（孩子兄弟链表逐层复制，不共享任何节点）
static TreeNode* deepCopyNode(const TreeNode* src)
{
    TreeNode* dst = new TreeNode(src->name, src->isDir);
    dst->size = src->size;
    dst->ext = src->ext;
    dst->modifyTime = src->modifyTime;

    for (const TreeNode* child = src->firstChild; child != nullptr; child = child->nextSibling)
    {
        dst->addChild(deepCopyNode(child));
    }
    return dst;
}

// 将 fs::path 转为 UTF-8 字符串，便于存入 UndoRecord
static string pathToString(const fs::path& path)
{
    return path.generic_u8string();
}

// 解析磁盘根目录：绝对路径直接使用；相对路径依次尝试当前目录、上级、上两级、上三级
static fs::path resolveDiskRootPath(const string& diskRoot)
{
    error_code ec;
    fs::path raw = fs::u8path(diskRoot);

    if (raw.is_absolute())
    {
        fs::path absolute = fs::absolute(raw, ec);
        if (ec)
        {
            return {};
        }
        fs::path canonical = fs::weakly_canonical(absolute, ec);
        return ec ? absolute.lexically_normal() : canonical;
    }

    const char* prefixes[] = { "", "../", "../../", "../../../" };
    for (const char* prefix : prefixes)
    {
        fs::path candidate = fs::u8path(string(prefix) + diskRoot);
        if (!fs::is_directory(candidate, ec))
        {
            continue;
        }
        fs::path absolute = fs::absolute(candidate, ec);
        if (ec)
        {
            continue;
        }
        fs::path canonical = fs::weakly_canonical(absolute, ec);
        return ec ? absolute.lexically_normal() : canonical;
    }

    // 从项目根目录启动时，workspace 位于 file-tree-browser/workspace
    if (diskRoot == "workspace")
    {
        const char* projectPrefixes[] = { "file-tree-browser/workspace", "../file-tree-browser/workspace" };
        for (const char* candidateText : projectPrefixes)
        {
            fs::path candidate = fs::u8path(string(candidateText));
            if (!fs::is_directory(candidate, ec))
            {
                continue;
            }
            fs::path absolute = fs::absolute(candidate, ec);
            if (ec)
            {
                continue;
            }
            fs::path canonical = fs::weakly_canonical(absolute, ec);
            return ec ? absolute.lexically_normal() : canonical;
        }
    }
    return {};
}

// 由节点回溯到绑定根，得到绝对磁盘路径；未绑定/越界返回空路径
static fs::path diskPathOfNoLock(TreeNode* node)
{
    if (g_boundRoot == nullptr || node == nullptr)
    {
        return {};
    }

    fs::path relative;
    const TreeNode* current = node;
    while (current != g_boundRoot)
    {
        if (current == nullptr || current->parent == nullptr)
        {
            return {};
        }
        relative = fs::u8path(current->name) / relative;
        current = current->parent;
    }

    fs::path full = (g_boundRootPath / relative).lexically_normal();
    const string rootText = g_boundRootPath.generic_u8string();
    const string fullText = full.generic_u8string();

    if (fullText == rootText)
    {
        return full;
    }
    if (fullText.size() <= rootText.size())
    {
        return {};
    }
    if (fullText.compare(0, rootText.size(), rootText) != 0)
    {
        return {};
    }
    if (rootText.back() != '/' && fullText[rootText.size()] != '/')
    {
        return {};
    }
    return full;
}

// 在workspace.trash 目录内生成唯一路径
static fs::path makeTrashPathNoLock(const string& name)
{
    error_code ec;
    fs::create_directories(g_trashPath, ec);
    if (ec)
    {
        return {};
    }

    long long stamp = chrono::duration_cast<chrono::milliseconds>(
                          chrono::system_clock::now().time_since_epoch())
                          .count();
    for (int i = 0; i < 10000; i++)
    {
        fs::path candidate = g_trashPath / fs::u8path(to_string(stamp) + "_" + to_string(i) + "_" + name);
        bool exists = fs::exists(candidate, ec);
        if (ec)
        {
            return {};
        }
        if (!exists)
        {
            return candidate;
        }
    }
    return {};
}

// 记录一条待清理的workspace.trash 目录路径
static void rememberTrashPath(const string& path)
{
    if (!path.empty())
    {
        g_trashPaths.push_back(path);
    }
}

// 撤销删除后，把对应workspace.trash 目录路径从待清理列表移除
static void forgetTrashPath(const string& path)
{
    for (size_t i = 0; i < g_trashPaths.size(); i++)
    {
        if (g_trashPaths[i] == path)
        {
            g_trashPaths.erase(g_trashPaths.begin() + i);
            return;
        }
    }
}

// 撤销删除后，节点已经回到树上，不能再由 clearHistory 重复 delete
static void forgetDetachedNode(TreeNode* node)
{
    for (size_t i = 0; i < g_detached.nodes.size(); i++)
    {
        if (g_detached.nodes[i] == node)
        {
            g_detached.nodes.erase(g_detached.nodes.begin() + i);
            return;
        }
    }
}

// 清空历史、释放待恢复节点、清空 workspace.trash 目录内容（调用方已持锁）
static void clearHistoryNoLock()
{
    g_history.clear();

    for (TreeNode* node : g_detached.nodes)
    {
        delete node;
    }
    g_detached.nodes.clear();

    error_code ec;
    for (const string& path : g_trashPaths)
    {
        fs::remove_all(fs::u8path(path), ec);
    }
    g_trashPaths.clear();

    // 连同未登记到历史中的残留一起清掉，但保留 workspace.trash 目录本身
    fs::path trashToClear = g_trashPath;
    if (trashToClear.empty())
    {
        // 进程重启后尚未绑定：按 workspace 默认位置清理上次登录的残留
        fs::path workspacePath = resolveDiskRootPath("workspace");
        if (!workspacePath.empty())
        {
            fs::path parent = workspacePath.parent_path();
            trashToClear = parent.empty()
                               ? workspacePath / fs::u8path(".trash")
                               : parent / fs::u8path(workspacePath.filename().u8string() + ".trash");
        }
    }
    if (!trashToClear.empty())
    {
        fs::remove_all(trashToClear, ec);
        ec.clear();
        fs::create_directories(trashToClear, ec);
    }
}

// 将撤销记录压入历史栈
static void pushRecord(int opType, TreeNode* target, const string& backup,
                       TreeNode* parent, TreeNode* prevSibling,
                       const string& diskPath = "", const string& trashPath = "",
                       const string& newDiskPath = "")
{
    UndoRecord record;
    record.opType = opType;
    record.target = target;
    record.backup = backup;
    record.parent = parent;
    record.prevSibling = prevSibling;
    record.diskPath = diskPath;
    record.trashPath = trashPath;
    record.newDiskPath = newDiskPath;
    g_history.push_back(record);
}

// 内存拷贝实现（调用方已持锁，磁盘部分已单独处理）
static int copyNodeMemoryNoLock(TreeNode* node, TreeNode* newParent, TreeNode** copiedOut)
{
    if (copiedOut != nullptr)
    {
        *copiedOut = nullptr;
    }
    if (node == nullptr || newParent == nullptr || !newParent->isDir)
    {
        return -1;
    }
    if (isAncestorOf(node, newParent))
    {
        return -8;
    }
    if (newParent->hasChild(node->name))
    {
        return -7;
    }

    TreeNode* copied = nullptr;
    try
    {
        copied = deepCopyNode(node);
    }
    catch (...)
    {
        return -10;
    }

    newParent->addChild(copied);
    if (copiedOut != nullptr)
    {
        *copiedOut = copied;
    }
    return 0;
}

// ===== 磁盘绑定实现 =====

// 绑定磁盘根：校验目录、清空旧历史、创建 <root> 同级的 workspace.trash 目录
int bindDiskRoot(TreeNode* root, const string& diskRoot)
{
    lock_guard<mutex> lock(g_mutex);

    if (root == nullptr || !root->isDir)
    {
        return -1;
    }

    fs::path resolved = resolveDiskRootPath(diskRoot);
    error_code ec;
    if (resolved.empty() || !fs::is_directory(resolved, ec))
    {
        return -2;
    }

    if (g_boundRoot == root && g_boundRootPath == resolved)
    {
        error_code ensureEc;
        fs::create_directories(g_trashPath, ensureEc);
        return ensureEc ? -10 : 0;
    }

    clearHistoryNoLock();

    g_boundRoot = root;
    g_boundRootPath = resolved;

    // workspace.trash 目录与 workspace 同级，避免重新扫描 workspace 时出现在树里
    fs::path parent = resolved.parent_path();
    if (parent.empty())
    {
        g_trashPath = resolved / fs::u8path(".trash");
    }
    else
    {
        g_trashPath = parent / fs::u8path(resolved.filename().u8string() + ".trash");
    }

    // 重新绑定前清理上次残留，避免崩溃后堆积
    fs::remove_all(g_trashPath, ec);
    ec.clear();
    fs::create_directories(g_trashPath, ec);
    if (ec)
    {
        g_boundRoot = nullptr;
        g_boundRootPath.clear();
        g_trashPath.clear();
        return -10;
    }

    return 0;
}

// 当前绑定是否就是指定树根
bool isDiskBoundFor(TreeNode* root)
{
    lock_guard<mutex> lock(g_mutex);
    return root != nullptr && g_boundRoot == root && !g_boundRootPath.empty();
}

// 解除绑定并清理 workspace.trash 目录内容
void unbindDiskRoot()
{
    lock_guard<mutex> lock(g_mutex);

    clearHistoryNoLock();

    error_code ec;
    fs::path trashToClear;
    if (!g_trashPath.empty())
    {
        trashToClear = g_trashPath;
    }
    else
    {
        // 进程重启后还没绑定过磁盘根：按 workspace 默认位置清理上次登录的
        // workspace.trash 目录内容，保证第二次登录看不到上一次登录删除的文件。
        fs::path workspacePath = resolveDiskRootPath("workspace");
        if (!workspacePath.empty())
        {
            fs::path parent = workspacePath.parent_path();
            trashToClear = parent.empty()
                               ? workspacePath / fs::u8path(".trash")
                               : parent / fs::u8path(workspacePath.filename().u8string() + ".trash");
        }
    }

    if (!trashToClear.empty())
    {
        // 只清内容，保留 workspace.trash 目录本身，避免登录后目录消失
        fs::remove_all(trashToClear, ec);
        ec.clear();
        fs::create_directories(trashToClear, ec);
    }

    g_boundRoot = nullptr;
    g_boundRootPath.clear();
    g_trashPath.clear();
}

// ===== 增删改实现 =====

// 新增节点：先创建磁盘文件/目录，再挂到内存树
int addNode(TreeNode* parent, const string& name, bool isDir)
{
    lock_guard<mutex> lock(g_mutex);

    if (parent == nullptr || !parent->isDir)
    {
        return -1;
    }
    if (!isValidName(name))
    {
        return -12;
    }
    if (parent->hasChild(name))
    {
        return -7;
    }

    fs::path parentDisk = diskPathOfNoLock(parent);
    if (parentDisk.empty())
    {
        return -10;
    }

    error_code ec;
    fs::path childDisk = parentDisk / fs::u8path(name);
    if (fs::exists(childDisk, ec))
    {
        return -7;
    }
    if (ec)
    {
        return -10;
    }

    if (isDir)
    {
        if (!fs::create_directory(childDisk, ec) || ec)
        {
            return -10;
        }
    }
    else
    {
        ofstream ofs(childDisk, ios::binary);
        if (!ofs.is_open())
        {
            return -10;
        }
        ofs.close();
        if (!ofs)
        {
            fs::remove(childDisk, ec);
            return -10;
        }
    }

    TreeNode* node = nullptr;
    try
    {
        node = new TreeNode(name, isDir);
    }
    catch (...)
    {
        if (isDir)
        {
            fs::remove_all(childDisk, ec);
        }
        else
        {
            fs::remove(childDisk, ec);
        }
        return -10;
    }

    node->modifyTime = time(nullptr);
    if (!isDir)
    {
        node->size = 0;
        node->ext = extensionOf(name);
    }
    parent->addChild(node);

    pushRecord(OP_ADD, node, "", nullptr, nullptr, pathToString(childDisk));
    return 0;
}

// 递归遍历：把 node 对应的磁盘子树复制到 workspace.trash
static int backupSubtreeNoLock(TreeNode* node, const fs::path& source, const fs::path& target)
{
    if (node == nullptr)
    {
        return -1;
    }

    error_code ec;
    if (node->isDir)
    {
        if (!fs::create_directory(target, ec) || ec)
        {
            return -10;
        }
        for (TreeNode* child = node->firstChild; child != nullptr; child = child->nextSibling)
        {
            fs::path childSource = source / fs::u8path(child->name);
            fs::path childTarget = target / fs::u8path(child->name);
            int code = backupSubtreeNoLock(child, childSource, childTarget);
            if (code != 0)
            {
                return code;
            }
        }
        return 0;
    }

    fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec);
    return ec ? -10 : 0;
}

// 递归后序遍历：删除 node 对应的磁盘子树（先删孩子，再删自己）
static int deleteSubtreeNoLock(TreeNode* node, const fs::path& source)
{
    if (node == nullptr)
    {
        return -1;
    }

    if (node->isDir)
    {
        for (TreeNode* child = node->firstChild; child != nullptr; child = child->nextSibling)
        {
            fs::path childSource = source / fs::u8path(child->name);
            int code = deleteSubtreeNoLock(child, childSource);
            if (code != 0)
            {
                return code;
            }
        }
    }

    error_code ec;
    fs::remove(source, ec);
    return ec ? -10 : 0;
}

// 删除失败时从 workspace.trash 恢复原磁盘子树
static int restoreSubtreeNoLock(const fs::path& source, const fs::path& trash)
{
    error_code ec;
    fs::remove_all(source, ec);
    ec.clear();
    fs::rename(trash, source, ec);
    if (!ec)
    {
        return 0;
    }

    fs::copy(trash, source,
             fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
    if (ec)
    {
        return -10;
    }
    fs::remove_all(trash, ec);
    return 0;
}

// 删除节点：递归遍历备份到 workspace.trash，再递归后序遍历删除原文件
int removeNode(TreeNode* node)
{
    lock_guard<mutex> lock(g_mutex);

    if (node == nullptr || node->parent == nullptr)
    {
        return -1;
    }

    fs::path diskPath = diskPathOfNoLock(node);
    if (diskPath.empty())
    {
        return -10;
    }

    error_code ec;
    bool diskExists = fs::exists(diskPath, ec);
    if (ec)
    {
        return -10;
    }
    if (!diskExists)
    {
        return -2;
    }

    fs::path trashPath = makeTrashPathNoLock(node->name);
    if (trashPath.empty())
    {
        return -10;
    }

    // 先完整备份，备份失败则原文件不动
    int backupCode = backupSubtreeNoLock(node, diskPath, trashPath);
    if (backupCode != 0)
    {
        error_code cleanupEc;
        fs::remove_all(trashPath, cleanupEc);
        return backupCode;
    }

    // 再递归后序删除原文件；删除失败则从备份恢复
    int deleteCode = deleteSubtreeNoLock(node, diskPath);
    if (deleteCode != 0)
    {
        restoreSubtreeNoLock(diskPath, trashPath);
        return deleteCode;
    }

    TreeNode* parent = node->parent;
    TreeNode* prevSibling = prevSiblingOf(parent, node);
    if (!parent->removeChild(node->name))
    {
        restoreSubtreeNoLock(diskPath, trashPath);
        return -1;
    }

    g_detached.nodes.push_back(node);
    rememberTrashPath(pathToString(trashPath));
    pushRecord(OP_REMOVE, node, "", parent, prevSibling,
               pathToString(diskPath), pathToString(trashPath));
    return 0;
}

// 重命名节点：磁盘先改名，内存后改名
int renameNode(TreeNode* node, const string& newName)
{
    lock_guard<mutex> lock(g_mutex);

    if (node == nullptr)
    {
        return -1;
    }
    // 根节点改名会破坏“扫描根 -> 磁盘目录”的绑定关系，禁止
    if (node->parent == nullptr)
    {
        return -1;
    }
    if (!isValidName(newName))
    {
        return -12;
    }
    if (node->name == newName)
    {
        return 0;
    }
    if (node->parent->hasChild(newName))
    {
        return -7;
    }

    fs::path oldDisk = diskPathOfNoLock(node);
    if (oldDisk.empty())
    {
        return -10;
    }
    fs::path newDisk = oldDisk.parent_path() / fs::u8path(newName);

    error_code ec;
    bool newExists = fs::exists(newDisk, ec);
    if (ec)
    {
        return -10;
    }
    if (newExists)
    {
        return -7;
    }
    fs::rename(oldDisk, newDisk, ec);
    if (ec)
    {
        return -10;
    }

    string oldName = node->name;
    node->name = newName;
    if (!node->isDir)
    {
        node->ext = extensionOf(newName);
    }

    pushRecord(OP_RENAME, node, oldName, nullptr, nullptr,
               pathToString(oldDisk), "", pathToString(newDisk));
    return 0;
}

// 移动节点：磁盘 rename 成功后改内存父子链
int moveNode(TreeNode* node, TreeNode* newParent)
{
    lock_guard<mutex> lock(g_mutex);

    if (node == nullptr || newParent == nullptr || !newParent->isDir)
    {
        return -1;
    }
    if (node->parent == nullptr)
    {
        return -1;
    }
    if (node == newParent || isAncestorOf(node, newParent))
    {
        return -8;
    }
    if (node->parent == newParent)
    {
        return 0;
    }
    if (newParent->hasChild(node->name))
    {
        return -7;
    }

    fs::path sourceDisk = diskPathOfNoLock(node);
    fs::path newParentDisk = diskPathOfNoLock(newParent);
    if (sourceDisk.empty() || newParentDisk.empty())
    {
        return -10;
    }

    fs::path targetDisk = newParentDisk / fs::u8path(node->name);
    error_code ec;
    bool targetExists = fs::exists(targetDisk, ec);
    if (ec)
    {
        return -10;
    }
    if (targetExists)
    {
        return -7;
    }
    fs::rename(sourceDisk, targetDisk, ec);
    if (ec)
    {
        return -10;
    }

    TreeNode* oldParent = node->parent;
    TreeNode* prevSibling = prevSiblingOf(oldParent, node);
    if (!oldParent->removeChild(node->name))
    {
        error_code rollbackEc;
        fs::rename(targetDisk, sourceDisk, rollbackEc);
        return -1;
    }
    newParent->addChild(node);

    pushRecord(OP_MOVE, node, "", oldParent, prevSibling,
               pathToString(sourceDisk), "", pathToString(targetDisk));
    return 0;
}

// 拷贝节点：磁盘递归复制，成功后再做内存深拷贝
int copyNode(TreeNode* node, TreeNode* newParent)
{
    lock_guard<mutex> lock(g_mutex);

    if (node == nullptr || newParent == nullptr || !newParent->isDir)
    {
        return -1;
    }
    if (isAncestorOf(node, newParent))
    {
        return -8;
    }
    if (newParent->hasChild(node->name))
    {
        return -7;
    }

    fs::path sourceDisk = diskPathOfNoLock(node);
    fs::path newParentDisk = diskPathOfNoLock(newParent);
    if (sourceDisk.empty() || newParentDisk.empty())
    {
        return -10;
    }

    fs::path targetDisk = newParentDisk / fs::u8path(node->name);
    error_code ec;
    bool targetExists = fs::exists(targetDisk, ec);
    if (ec)
    {
        return -10;
    }
    if (targetExists)
    {
        return -7;
    }

    try
    {
        fs::copy(sourceDisk, targetDisk,
                 fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
    }
    catch (...)
    {
        ec = make_error_code(errc::io_error);
    }
    if (ec)
    {
        error_code cleanupEc;
        fs::remove_all(targetDisk, cleanupEc);
        return -10;
    }

    TreeNode* copied = nullptr;
    int memoryCode = copyNodeMemoryNoLock(node, newParent, &copied);
    if (memoryCode != 0)
    {
        error_code cleanupEc;
        fs::remove_all(targetDisk, cleanupEc);
        return memoryCode;
    }

    pushRecord(OP_COPY, copied, "", newParent, nullptr, pathToString(targetDisk));
    return 0;
}

// 按名称拷贝
int copyNode(TreeNode* srcParent, TreeNode* destParent, const string& name)
{
    lock_guard<mutex> lock(g_mutex);

    if (srcParent == nullptr || destParent == nullptr)
    {
        return -1;
    }

    TreeNode* source = srcParent->findChild(name);
    if (source == nullptr)
    {
        return -1;
    }

    // 复用外部加锁版本的核心逻辑：这里直接展开调用，避免二次加锁
    if (isAncestorOf(source, destParent))
    {
        return -8;
    }
    if (destParent->hasChild(source->name))
    {
        return -7;
    }

    fs::path sourceDisk = diskPathOfNoLock(source);
    fs::path destDisk = diskPathOfNoLock(destParent);
    if (sourceDisk.empty() || destDisk.empty())
    {
        return -10;
    }

    fs::path targetDisk = destDisk / fs::u8path(source->name);
    error_code ec;
    bool targetExists = fs::exists(targetDisk, ec);
    if (ec)
    {
        return -10;
    }
    if (targetExists)
    {
        return -7;
    }

    try
    {
        fs::copy(sourceDisk, targetDisk,
                 fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
    }
    catch (...)
    {
        ec = make_error_code(errc::io_error);
    }
    if (ec)
    {
        error_code cleanupEc;
        fs::remove_all(targetDisk, cleanupEc);
        return -10;
    }

    TreeNode* copied = nullptr;
    int memoryCode = copyNodeMemoryNoLock(source, destParent, &copied);
    if (memoryCode != 0)
    {
        error_code cleanupEc;
        fs::remove_all(targetDisk, cleanupEc);
        return memoryCode;
    }

    pushRecord(OP_COPY, copied, "", destParent, nullptr, pathToString(targetDisk));
    return 0;
}

// 手动压入撤销记录（外部调用没有磁盘上下文，仅作为内存记录）
void pushOperation(int opType, TreeNode* target, const string& backup)
{
    lock_guard<mutex> lock(g_mutex);
    pushRecord(opType, target, backup, nullptr, nullptr);
}

// AI生成，已手动审查修改：撤销同时处理内存链接与磁盘workspace.trash 目录，先盘后存、失败回滚
// 撤销最近一次操作：只取消链接/恢复链接，不销毁节点
int undoLast()
{
    lock_guard<mutex> lock(g_mutex);

    if (g_history.empty())
    {
        return -1;
    }

    UndoRecord record = g_history.back();

    switch (record.opType)
    {
    case OP_ADD:
    case OP_COPY:
    {
        TreeNode* node = record.target;
        if (node == nullptr || node->parent == nullptr)
        {
            return -1;
        }

        error_code ec;
        fs::path diskPath = diskPathOfNoLock(node);
        fs::path trashPath;
        if (!diskPath.empty())
        {
            bool diskExists = fs::exists(diskPath, ec);
            if (ec)
            {
                return -10;
            }
            if (diskExists)
            {
                trashPath = makeTrashPathNoLock(node->name);
                if (trashPath.empty())
                {
                    return -10;
                }
                fs::rename(diskPath, trashPath, ec);
                if (ec)
                {
                    return -10;
                }
            }
        }

        TreeNode* parent = node->parent;
        if (!parent->removeChild(node->name))
        {
            if (!trashPath.empty())
            {
                error_code rollbackEc;
                fs::rename(trashPath, diskPath, rollbackEc);
            }
            return -1;
        }

        g_detached.nodes.push_back(node);
        rememberTrashPath(pathToString(trashPath));
        g_history.pop_back();
        return 0;
    }
    case OP_REMOVE:
    {
        TreeNode* node = record.target;
        TreeNode* parent = record.parent;
        if (node == nullptr || parent == nullptr)
        {
            return -1;
        }

        error_code ec;
        if (!record.trashPath.empty() && !record.diskPath.empty())
        {
            fs::path trashPath = fs::u8path(record.trashPath);
            fs::path diskPath = fs::u8path(record.diskPath);
            bool originalExists = fs::exists(diskPath, ec);
            if (ec)
            {
                return -10;
            }
            if (originalExists)
            {
                return -7; // 原位置已被占用，保留workspace.trash 目录内容
            }
            bool trashExists = fs::exists(trashPath, ec);
            if (ec || !trashExists)
            {
                return -10;
            }
            fs::rename(trashPath, diskPath, ec);
            if (ec)
            {
                return -10;
            }
            forgetTrashPath(record.trashPath);
        }

        insertAfter(parent, record.prevSibling, node);
        forgetDetachedNode(node);
        g_history.pop_back();
        return 0;
    }
    case OP_RENAME:
    {
        TreeNode* node = record.target;
        if (node == nullptr)
        {
            return -1;
        }

        error_code ec;
        if (!record.newDiskPath.empty() && !record.diskPath.empty())
        {
            fs::path oldDisk = fs::u8path(record.diskPath);
            fs::path newDisk = fs::u8path(record.newDiskPath);
            bool oldExists = fs::exists(oldDisk, ec);
            if (ec)
            {
                return -10;
            }
            if (oldExists)
            {
                return -7;
            }
            bool newExists = fs::exists(newDisk, ec);
            if (ec)
            {
                return -10;
            }
            if (!newExists)
            {
                // 新旧路径都不存在，撤销会得到“内存改了、磁盘没有”的假成功
                return -10;
            }
            fs::rename(newDisk, oldDisk, ec);
            if (ec)
            {
                return -10;
            }
        }

        node->name = record.backup;
        if (!node->isDir)
        {
            node->ext = extensionOf(record.backup);
        }
        g_history.pop_back();
        return 0;
    }
    case OP_MOVE:
    {
        TreeNode* node = record.target;
        TreeNode* oldParent = record.parent;
        if (node == nullptr || oldParent == nullptr || node->parent == nullptr)
        {
            return -1;
        }

        error_code ec;
        if (!record.newDiskPath.empty() && !record.diskPath.empty())
        {
            fs::path oldDisk = fs::u8path(record.diskPath);
            fs::path newDisk = fs::u8path(record.newDiskPath);
            bool oldExists = fs::exists(oldDisk, ec);
            if (ec)
            {
                return -10;
            }
            if (oldExists)
            {
                return -7;
            }
            bool newExists = fs::exists(newDisk, ec);
            if (ec || !newExists)
            {
                return -10;
            }
            fs::rename(newDisk, oldDisk, ec);
            if (ec)
            {
                return -10;
            }
        }

        if (!node->parent->removeChild(node->name))
        {
            // 内存回退失败：把磁盘再挪回新位置，保证内存/磁盘一致
            if (!record.newDiskPath.empty() && !record.diskPath.empty())
            {
                error_code rollbackEc;
                fs::rename(fs::u8path(record.diskPath), fs::u8path(record.newDiskPath), rollbackEc);
            }
            return -1;
        }
        insertAfter(oldParent, record.prevSibling, node);
        g_history.pop_back();
        return 0;
    }
    default:
        return -1;
    }
}

// 清空历史：释放未恢复节点，删除workspace.trash 目录中未恢复内容
void clearHistory()
{
    lock_guard<mutex> lock(g_mutex);
    clearHistoryNoLock();
}
