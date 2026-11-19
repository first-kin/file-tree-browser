<script setup lang="ts" generic="TRow extends Record<string, unknown>">
import { computed } from 'vue'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

export interface ResultColumn<TRow extends Record<string, unknown> = Record<string, unknown>> {
  key: keyof TRow & string
  label: string
  width?: number | string
  align?: 'left' | 'center' | 'right'
  sortable?: boolean
}

const props = withDefaults(defineProps<{
  rows: TRow[]
  columns: ResultColumn<TRow>[]
  loading?: boolean
  page?: number
  pageSize?: number
  total?: number
  rowKey?: keyof TRow & string
}>(), {
  loading: false,
  page: 1,
  pageSize: 50,
  total: undefined,
  rowKey: undefined
})

const emit = defineEmits<{
  'update:page': [page: number]
  'update:pageSize': [pageSize: number]
  'row-click': [row: TRow]
}>()

const visibleRows = computed(() => {
  if (props.total !== undefined) return props.rows
  const start = (props.page - 1) * props.pageSize
  return props.rows.slice(start, start + props.pageSize)
})

const totalRows = computed(() => props.total ?? props.rows.length)

function handleRowClick(row: TRow) {
  emit('row-click', row)
}

function handlePage(value: number) {
  emit('update:page', value)
}

function handlePageSize(value: number) {
  emit('update:pageSize', value)
}
</script>

<template>
  <div class="result-table" v-loading="loading">
    <el-table
      :data="visibleRows"
      :row-key="rowKey"
      class="result-table-grid"
      :empty-text="loading ? '正在加载…' : '暂无结果'"
      @row-click="handleRowClick"
    >
      <el-table-column
        v-for="column in columns"
        :key="column.key"
        :prop="column.key"
        :label="column.label"
        :width="column.width"
        :align="column.align"
        :sortable="column.sortable"
        show-overflow-tooltip
      />
      <slot />
    </el-table>
    <el-pagination
      v-if="totalRows > pageSize"
      :current-page="page"
      :page-size="pageSize"
      :page-sizes="[20, 50, 100]"
      :total="totalRows"
      layout="total, sizes, prev, pager, next"
      background
      @current-change="handlePage"
      @size-change="handlePageSize"
    />
  </div>
</template>
