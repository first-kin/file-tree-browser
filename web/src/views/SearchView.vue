<script setup lang="ts">
import PageHeader from '@/components/PageHeader.vue'
import ResultTable from '@/components/ResultTable.vue'

interface SearchPreviewRow extends Record<string, unknown> {
  name: string
  path: string
  type: string
}

const previewRows: SearchPreviewRow[] = []
const previewColumns = [
  { key: 'name', label: '名称' },
  { key: 'path', label: '路径' },
  { key: 'type', label: '类型', width: 120 }
] as const
</script>

<template>
  <div class="workspace-page placeholder-page">
    <PageHeader eyebrow="SEARCH / INDEX" title="智能查找" description="通过名称、扩展名、大小与时间快速定位文件。" />
    <section class="content-card feature-placeholder-card">
      <div class="card-toolbar"><div><span class="section-kicker">RESULTS / PREVIEW</span><h2>检索结果</h2></div></div>
      <ResultTable :rows="previewRows" :columns="previewColumns" />
      <div class="placeholder-card"><span>⌕</span><div><b>查找模块等待功能页面接入</b><p>结果表格、分页与行点击契约已经就绪。</p></div><small>MODULE / SEARCH</small></div>
    </section>
  </div>
</template>
