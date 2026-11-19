<script setup lang="ts">
import PageHeader from '@/components/PageHeader.vue'
import ResultTable from '@/components/ResultTable.vue'

interface StatsPreviewRow extends Record<string, unknown> {
  name: string
  value: string
  share: string
}

const previewRows: StatsPreviewRow[] = []
const previewColumns = [
  { key: 'name', label: '指标' },
  { key: 'value', label: '数值', width: 140 },
  { key: 'share', label: '占比', width: 120 }
] as const
</script>

<template>
  <div class="workspace-page placeholder-page">
    <PageHeader eyebrow="DATA / INSIGHTS" title="数据洞察" description="从节点数量到目录体积，观察你的文件空间。" />
    <section class="content-card feature-placeholder-card">
      <div class="card-toolbar"><div><span class="section-kicker">INSIGHTS / PREVIEW</span><h2>统计结果</h2></div></div>
      <ResultTable :rows="previewRows" :columns="previewColumns" />
      <div class="placeholder-card"><span>◌</span><div><b>统计模块等待功能页面接入</b><p>分页结果表格与空状态已经就绪。</p></div><small>MODULE / STATS</small></div>
    </section>
  </div>
</template>
