<script setup lang="ts">
import { computed, reactive, ref } from 'vue'
import { ElMessage } from 'element-plus'
import { useRoute } from 'vue-router'
import { buildGraph, buildMerkle, diffMerkle, getGraphCycles, type GraphBuildResult, type GraphCyclesResult, type MerkleBuildResult, type MerkleDiffResult } from '@/api/p2'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

const route = useRoute()
const isGraph = computed(() => route.name === 'graph')
const merkleForm = reactive({ path: '', pathA: '', pathB: '' })
const graphResult = ref<GraphBuildResult>()
const cyclesResult = ref<GraphCyclesResult>()
const rootResult = ref<MerkleBuildResult>()
const diffResult = ref<MerkleDiffResult>()
const loading = ref(false)

async function runMerkleBuild() {
  if (!merkleForm.path.trim()) return ElMessage.warning('请输入要校验的目录路径')
  loading.value = true
  try {
    rootResult.value = await buildMerkle(merkleForm.path.trim())
    ElMessage.success('根哈希已生成')
  } catch { /* 错误由统一请求层提示 */ } finally { loading.value = false }
}

async function runMerkleDiff() {
  if (!merkleForm.pathA.trim() || !merkleForm.pathB.trim()) return ElMessage.warning('请填写两个目录路径')
  loading.value = true
  try {
    diffResult.value = await diffMerkle(merkleForm.pathA.trim(), merkleForm.pathB.trim())
  } catch { /* 错误由统一请求层提示 */ } finally { loading.value = false }
}

async function runGraph() {
  loading.value = true
  try {
    graphResult.value = await buildGraph()
    cyclesResult.value = await getGraphCycles()
    ElMessage.success('目录链接图分析完成')
  } catch { /* 错误由统一请求层提示 */ } finally { loading.value = false }
}

async function copyText(value: string) {
  await navigator.clipboard.writeText(value)
  ElMessage.success('已复制到剪贴板')
}
</script>

<template>
  <div class="workspace-page p2-page">
    <div class="page-heading">
      <div>
        <div class="eyebrow"><span></span> ADVANCED / P2</div>
        <h1>{{ isGraph ? '环路检测' : '完整性校验' }}</h1>
        <p>{{ isGraph ? '识别目录链接形成的环路，避免遍历死循环。' : '通过 Merkle 根哈希比对两个目录的完整性。' }}</p>
      </div>
    </div>

    <template v-if="!isGraph">
      <section class="content-card p2-card">
        <div class="card-toolbar"><div><span class="section-kicker">MERKLE / BUILD</span><h2>构建根哈希</h2></div></div>
        <div class="p2-form-row"><el-input v-model="merkleForm.path" placeholder="输入服务端可访问的目录路径" clearable @keyup.enter="runMerkleBuild" /><button class="ghost-button" type="button" :disabled="loading" @click="runMerkleBuild">生成哈希</button></div>
        <div v-if="rootResult" class="p2-result"><span>ROOT HASH</span><code>{{ rootResult.rootHash }}</code><button class="text-action" type="button" @click="copyText(rootResult.rootHash)">复制</button></div>
      </section>
      <section class="content-card p2-card">
        <div class="card-toolbar"><div><span class="section-kicker">MERKLE / DIFF</span><h2>比对两个目录</h2></div></div>
        <div class="p2-form-grid"><el-input v-model="merkleForm.pathA" placeholder="目录 A" clearable /><el-input v-model="merkleForm.pathB" placeholder="目录 B" clearable /></div>
        <button class="primary-auth-button p2-submit" type="button" :disabled="loading" @click="runMerkleDiff">开始比对 <i>↗</i></button>
        <div v-if="diffResult" class="p2-diff-result"><b>{{ diffResult.same ? '两个目录内容一致' : `发现 ${diffResult.diffFiles.length} 个差异文件` }}</b><ul v-if="!diffResult.same"><li v-for="file in diffResult.diffFiles" :key="file"><code>{{ file }}</code><button class="text-action" type="button" @click="copyText(file)">复制</button></li></ul></div>
      </section>
    </template>

    <section v-else class="content-card p2-card">
      <div class="card-toolbar"><div><span class="section-kicker">GRAPH / CYCLE SCAN</span><h2>目录链接图</h2></div><button class="ghost-button" type="button" :disabled="loading" @click="runGraph">开始检测</button></div>
      <div v-if="graphResult" class="metric-row p2-metrics"><div class="metric-card"><small>节点数</small><strong>{{ graphResult.nodeCount }}</strong></div><div class="metric-card"><small>边数</small><strong>{{ graphResult.edgeCount }}</strong></div><div class="metric-card"><small>环数量</small><strong>{{ cyclesResult?.cycles.length ?? 0 }}</strong></div></div>
      <div v-if="cyclesResult" class="p2-cycle-list"><div v-if="!cyclesResult.cycles.length" class="empty-state"><b>未检测到目录环</b><small>当前链接图可以安全遍历</small></div><ul v-else><li v-for="(cycle, index) in cyclesResult.cycles" :key="index"><span>{{ String(index + 1).padStart(2, '0') }}</span><code>{{ cycle.join(' → ') }}</code><button class="text-action" type="button" @click="copyText(cycle.join(' → '))">复制</button></li></ul></div>
      <div v-else class="placeholder-card"><span>∞</span><div><b>等待开始检测</b><p>构建链接图后，这里会显示完整环路径。</p></div></div>
    </section>
  </div>
</template>
