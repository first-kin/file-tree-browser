#include "../include/graph_detect.h"

#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

// ===== 模块内部状态 =====

// 全局图存储；单机版同一时间只服务一个会话，重复建图时整体替换
struct GraphStorage
{
    vector<GraphNode*> nodes;                          // 全部图节点（持有点）
    vector<TreeNode*> dirNodes;                        // 全部目录树节点
    vector<TreeNode*> files;                           // 全部文件节点（用于解析快捷方式）
    unordered_map<TreeNode*, GraphNode*> dirToGraph;   // 目录树节点 -> 图节点
    unordered_map<string, GraphNode*> nameToDir;       // 小写目录名 -> 首个图节点
    int edgeCount = 0;                                 // 边数

    // 释放所有图节点并清空中间数据
    void clear()
    {
        for (GraphNode* node : nodes)
        {
            delete node;
        }
        nodes.clear();
        dirNodes.clear();
        files.clear();
        dirToGraph.clear();
        nameToDir.clear();
        edgeCount = 0;
    }

    ~GraphStorage()
    {
        clear();
    }
};

static GraphStorage g_graph;
static mutex g_graphMutex;

// ===== 内部辅助函数 =====

// ASCII 转小写，用于 Windows 风格的不区分大小写名称匹配
static string toLowerAscii(const string& text)
{
    string result = text;
    for (char& c : result)
    {
        if (c >= 'A' && c <= 'Z')
        {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return result;
}

// 判断是否为快捷方式文件（文件名以 .lnk 结尾，忽略大小写）
static bool isShortcutFile(const TreeNode* node)
{
    if (node == nullptr || node->isDir || node->name.size() <= 4)
    {
        return false;
    }
    return toLowerAscii(node->name.substr(node->name.size() - 4)) == ".lnk";
}

// 取快捷方式的目标名称（去掉末尾 4 个字符 ".lnk"）
static string shortcutTargetName(const TreeNode* node)
{
    return node->name.substr(0, node->name.size() - 4);
}

// 在 parent 的直接孩子中按名称查找目录（忽略大小写）
static TreeNode* findChildDirIgnoreCase(TreeNode* parent, const string& name)
{
    string lower = toLowerAscii(name);
    for (TreeNode* child = parent->firstChild; child != nullptr; child = child->nextSibling)
    {
        if (child->isDir && toLowerAscii(child->name) == lower)
        {
            return child;
        }
    }
    return nullptr;
}

// 先序收集目录/文件节点并建立 GraphNode；visited 防止损坏指针导致死循环
static void collectTreeNodes(TreeNode* root, GraphStorage& build)
{
    vector<TreeNode*> stack;
    unordered_set<TreeNode*> visited;
    stack.push_back(root);

    while (!stack.empty())
    {
        TreeNode* node = stack.back();
        stack.pop_back();
        if (node == nullptr || visited.count(node) != 0)
        {
            continue;
        }
        visited.insert(node);

        if (node->isDir)
        {
            GraphNode* graphNode = new GraphNode();
            graphNode->dirNode = node;
            graphNode->color = 0;

            build.nodes.push_back(graphNode);
            build.dirNodes.push_back(node);
            build.dirToGraph[node] = graphNode;

            string key = toLowerAscii(node->name);
            if (build.nameToDir.find(key) == build.nameToDir.end())
            {
                build.nameToDir[key] = graphNode; // 同名目录取先序第一个
            }
        }
        else
        {
            build.files.push_back(node);
        }

        vector<TreeNode*> children;
        unordered_set<TreeNode*> seenChildren;
        for (TreeNode* child = node->firstChild; child != nullptr; child = child->nextSibling)
        {
            if (seenChildren.count(child) != 0)
            {
                break;
            }
            seenChildren.insert(child);
            children.push_back(child);
        }
        for (size_t i = children.size(); i > 0; i--)
        {
            stack.push_back(children[i - 1]);
        }
    }
}

// 建立图边：目录 -> 孩子目录；快捷方式所在目录 -> 目标目录
static void buildGraphEdges(GraphStorage& build)
{
    // 孩子目录边
    for (TreeNode* dirNode : build.dirNodes)
    {
        auto fromIt = build.dirToGraph.find(dirNode);
        if (fromIt == build.dirToGraph.end())
        {
            continue;
        }

        unordered_set<TreeNode*> seenChildren;
        for (TreeNode* child = dirNode->firstChild; child != nullptr; child = child->nextSibling)
        {
            if (seenChildren.count(child) != 0)
            {
                break;
            }
            seenChildren.insert(child);

            if (!child->isDir)
            {
                continue;
            }
            auto toIt = build.dirToGraph.find(child);
            if (toIt != build.dirToGraph.end())
            {
                fromIt->second->neighbors.push_back(toIt->second);
                build.edgeCount++;
            }
        }
    }

    // 快捷方式边：X.lnk 所在目录 -> 名为 X 的目录
    // 优先匹配快捷方式所在目录的同名子目录，找不到再匹配全树首个同名目录，减少同名误报
    for (TreeNode* file : build.files)
    {
        if (!isShortcutFile(file) || file->parent == nullptr)
        {
            continue;
        }
        auto fromIt = build.dirToGraph.find(file->parent);
        if (fromIt == build.dirToGraph.end())
        {
            continue;
        }

        string targetName = shortcutTargetName(file);
        GraphNode* target = nullptr;

        TreeNode* localDir = findChildDirIgnoreCase(file->parent, targetName);
        if (localDir != nullptr)
        {
            auto localIt = build.dirToGraph.find(localDir);
            if (localIt != build.dirToGraph.end())
            {
                target = localIt->second;
            }
        }
        if (target == nullptr)
        {
            auto toIt = build.nameToDir.find(toLowerAscii(targetName));
            if (toIt != build.nameToDir.end())
            {
                target = toIt->second;
            }
        }

        if (target != nullptr)
        {
            fromIt->second->neighbors.push_back(target);
            build.edgeCount++;
        }
    }
}

// 把三个颜色重置为白色，供每次 DFS 重新使用同一份图
static void resetColors(GraphStorage& build)
{
    for (GraphNode* node : build.nodes)
    {
        node->color = 0;
    }
}

// DFS 三色标记：遇到灰色节点说明存在后向边，即存在环
static bool dfsVisit(GraphNode* node)
{
    node->color = 1; // 灰：正在访问

    for (GraphNode* next : node->neighbors)
    {
        if (next == nullptr)
        {
            continue;
        }
        if (next->color == 1)
        {
            return true;
        }
        if (next->color == 0 && dfsVisit(next))
        {
            return true;
        }
    }

    node->color = 2; // 黑：子树访问完成
    return false;
}

// AI生成，已手动审查修改：在普通三色 DFS 上增加显式路径栈，回溯出每个环的节点序列
// DFS 收集所有环路径；path 保存当前递归栈，灰边回指时截取环
static void dfsCollectCycles(GraphNode* node,
                             vector<GraphNode*>& path,
                             unordered_set<string>& seen,
                             vector<vector<string>>& cycles)
{
    node->color = 1;
    path.push_back(node);

    for (GraphNode* next : node->neighbors)
    {
        if (next == nullptr)
        {
            continue;
        }

        if (next->color == 1)
        {
            // 在栈中找到回边指向的起点，截取环路径
            size_t index = path.size();
            for (size_t i = 0; i < path.size(); i++)
            {
                if (path[i] == next)
                {
                    index = i;
                    break;
                }
            }
            if (index == path.size())
            {
                continue;
            }

            vector<string> cycle;
            for (size_t i = index; i < path.size(); i++)
            {
                cycle.push_back(path[i]->dirNode->name);
            }
            cycle.push_back(next->dirNode->name); // 首尾闭合

            // 同一条环可能由不同入口重复发现，用节点名序列去重
            string key;
            for (const string& name : cycle)
            {
                key += name;
                key.push_back('\x1f');
            }
            if (seen.insert(key).second)
            {
                cycles.push_back(cycle);
            }
        }
        else if (next->color == 0)
        {
            dfsCollectCycles(next, path, seen, cycles);
        }
    }

    path.pop_back();
    node->color = 2;
}

// ===== 对外接口实现 =====

// 建图主入口：清空旧图 -> 收集节点 -> 建边
int buildGraph(TreeNode* root)
{
    lock_guard<mutex> lock(g_graphMutex);

    g_graph.clear();
    if (root == nullptr || !root->isDir)
    {
        return -1;
    }

    collectTreeNodes(root, g_graph);
    buildGraphEdges(g_graph);
    return 0;
}

// 环存在性检测：重置颜色后从每个白节点 DFS
bool hasCycle()
{
    lock_guard<mutex> lock(g_graphMutex);

    resetColors(g_graph);
    for (GraphNode* node : g_graph.nodes)
    {
        if (node->color == 0 && dfsVisit(node))
        {
            return true;
        }
    }
    return false;
}

// 环路径检测：重置颜色后 DFS 收集所有环
vector<vector<string>> getCyclePaths()
{
    lock_guard<mutex> lock(g_graphMutex);

    resetColors(g_graph);

    vector<vector<string>> cycles;
    unordered_set<string> seen;
    vector<GraphNode*> path;

    for (GraphNode* node : g_graph.nodes)
    {
        if (node->color == 0)
        {
            dfsCollectCycles(node, path, seen, cycles);
        }
    }
    return cycles;
}

// 带访问标记的安全遍历：重复指针只打印一次，防止损坏树导致死循环
void printSafe(TreeNode* root)
{
    lock_guard<mutex> lock(g_graphMutex);

    if (root == nullptr)
    {
        return;
    }

    vector<pair<TreeNode*, int>> stack;
    unordered_set<TreeNode*> visited;
    stack.push_back(make_pair(root, 0));

    while (!stack.empty())
    {
        TreeNode* node = stack.back().first;
        int depth = stack.back().second;
        stack.pop_back();

        if (node == nullptr || visited.count(node) != 0)
        {
            continue;
        }
        visited.insert(node);

        cout << string(static_cast<size_t>(depth) * 2, ' ') << node->name << endl;

        vector<TreeNode*> children;
        unordered_set<TreeNode*> seenChildren;
        for (TreeNode* child = node->firstChild; child != nullptr; child = child->nextSibling)
        {
            if (seenChildren.count(child) != 0)
            {
                break;
            }
            seenChildren.insert(child);
            children.push_back(child);
        }
        for (size_t i = children.size(); i > 0; i--)
        {
            stack.push_back(make_pair(children[i - 1], depth + 1));
        }
    }
}

// 最近一次建图的节点数
int getGraphNodeCount()
{
    lock_guard<mutex> lock(g_graphMutex);
    return static_cast<int>(g_graph.nodes.size());
}

// 最近一次建图的边数
int getGraphEdgeCount()
{
    lock_guard<mutex> lock(g_graphMutex);
    return g_graph.edgeCount;
}
