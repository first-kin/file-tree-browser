<script setup lang="ts">
// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。
defineProps<{
  eyebrow?: string
  title: string
  description?: string
}>()
</script>

<template>
  <header class="page-heading reusable-page-heading">
    <div>
      <div v-if="eyebrow" class="eyebrow"><span></span>{{ eyebrow }}</div>
      <h1>{{ title }}</h1>
      <p v-if="description">{{ description }}</p>
    </div>
    <div v-if="$slots.actions" class="page-heading-actions"><slot name="actions" /></div>
  </header>
</template>
