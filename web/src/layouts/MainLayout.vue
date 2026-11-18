<script setup lang="ts">
import { computed, ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useRoute, useRouter } from 'vue-router'
import { logout } from '@/api/auth'
import { useUserStore } from '@/stores/user'

const route = useRoute()
const router = useRouter()
const user = useUserStore()
const isCollapsed = ref(false)

const menuItems = computed(() => [
  { path: '/tree', label: '目录空间', hint: 'TREE', glyph: '⌘', visible: true },
  { path: '/search', label: '智能查找', hint: 'FIND', glyph: '⌕', visible: true },
  { path: '/stats', label: '数据洞察', hint: 'DATA', glyph: '◌', visible: true },
  { path: '/users', label: '成员权限', hint: 'TEAM', glyph: '♙', visible: user.isAdmin },
  { path: '/merkle', label: '完整性校验', hint: 'HASH', glyph: '◇', visible: user.isAdmin },
  { path: '/graph', label: '环路检测', hint: 'GRAPH', glyph: '∞', visible: user.isAdmin }
].filter((item) => item.visible))

const currentLabel = computed(() => (route.meta.label as string) || '目录空间')
const todayLabel = new Intl.DateTimeFormat('zh-CN', { month: 'short', day: 'numeric', weekday: 'short' }).format(new Date())

async function handleLogout() {
  try {
    await ElMessageBox.confirm('退出后需要重新登录才能访问工作台。', '确认退出工作台', {
      confirmButtonText: '退出登录',
      cancelButtonText: '留在这里',
      type: 'warning'
    })
    try { await logout() } catch { /* token 失效时仍然清理本地状态 */ }
    user.clearSession()
    await router.push('/login')
    ElMessage.success('已安全退出')
  } catch { /* 用户取消 */ }
}
</script>

<template>
  <div class="app-shell" :class="{ 'is-collapsed': isCollapsed }">
    <aside class="app-sidebar">
      <div class="brand-lockup">
        <div class="brand-mark"><span></span><span></span><span></span></div>
        <div v-if="!isCollapsed" class="brand-copy">
          <strong>Tree<span>line</span></strong>
          <small>FILE INTELLIGENCE</small>
        </div>
      </div>

      <div v-if="!isCollapsed" class="workspace-label">WORKSPACE <span>01</span></div>
      <nav class="side-nav" aria-label="主导航">
        <router-link
          v-for="item in menuItems"
          :key="item.path"
          :to="item.path"
          class="nav-item"
          :class="{ active: route.path === item.path }"
          :title="isCollapsed ? item.label : undefined"
        >
          <span class="nav-glyph">{{ item.glyph }}</span>
          <span v-if="!isCollapsed" class="nav-text"><b>{{ item.label }}</b><small>{{ item.hint }}</small></span>
          <i v-if="!isCollapsed && route.path === item.path" class="active-dot"></i>
        </router-link>
      </nav>

      <div class="sidebar-foot">
        <div v-if="!isCollapsed" class="system-note"><span class="pulse-dot"></span><span>系统运行正常</span><small>API ONLINE</small></div>
        <button class="collapse-button" type="button" @click="isCollapsed = !isCollapsed">
          <span>{{ isCollapsed ? '→' : '←' }}</span><em v-if="!isCollapsed">收起导航</em>
        </button>
      </div>
    </aside>

    <main class="app-main">
      <header class="topbar">
        <div class="breadcrumb"><span>WORKSPACE</span><b>/</b><strong>{{ currentLabel }}</strong></div>
        <div class="topbar-actions">
          <div class="date-stamp"><span class="live-dot"></span>{{ todayLabel }}</div>
          <button class="avatar-button" type="button" @click="handleLogout" :title="'退出 ' + user.username">
            <span>{{ user.initials }}</span><i></i>
          </button>
        </div>
      </header>

      <section class="page-canvas">
        <div class="ambient ambient-a"></div><div class="ambient ambient-b"></div>
        <router-view v-slot="{ Component }">
          <transition name="page" mode="out-in"><component :is="Component" /></transition>
        </router-view>
      </section>
    </main>
  </div>
</template>
