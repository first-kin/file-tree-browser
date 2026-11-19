<script setup lang="ts">
import { computed, reactive, ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useRoute, useRouter } from 'vue-router'
import { changePassword, logout } from '@/api/auth'
import { useUserStore } from '@/stores/user'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

const route = useRoute()
const router = useRouter()
const user = useUserStore()
const isCollapsed = ref(false)
const passwordDialogVisible = ref(false)
const passwordSaving = ref(false)
const passwordForm = reactive({ oldPassword: '', newPassword: '', confirmPassword: '' })

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

function openPasswordDialog() {
  passwordForm.oldPassword = ''
  passwordForm.newPassword = ''
  passwordForm.confirmPassword = ''
  passwordDialogVisible.value = true
}

async function submitPasswordChange() {
  if (!passwordForm.oldPassword) return ElMessage.warning('请输入当前密码')
  if (passwordForm.newPassword.length < 6) return ElMessage.warning('新密码至少需要 6 位')
  if (passwordForm.newPassword !== passwordForm.confirmPassword) return ElMessage.warning('两次输入的新密码不一致')
  passwordSaving.value = true
  try {
    await changePassword({ oldPassword: passwordForm.oldPassword, newPassword: passwordForm.newPassword })
    passwordDialogVisible.value = false
    ElMessage.success('密码修改成功')
  } catch { /* 错误由统一请求层提示 */ } finally {
    passwordSaving.value = false
  }
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
        <div v-if="!isCollapsed" class="system-note"><span class="pulse-dot"></span><span>前端服务已加载</span><small>WORKSPACE READY</small></div>
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
          <div v-if="user.isLoggedIn" class="user-identity">
            <div><strong>{{ user.username }}</strong><small>{{ user.displayRole }}</small></div>
            <button class="account-action" type="button" @click="openPasswordDialog">修改密码</button>
            <button class="avatar-button" type="button" @click="handleLogout" :title="'退出 ' + user.username">
              <span>{{ user.initials }}</span><i></i>
            </button>
          </div>
          <router-link v-else class="login-link" to="/login">登录工作台 <b>↗</b></router-link>
        </div>
      </header>

      <section class="page-canvas">
        <div class="ambient ambient-a"></div><div class="ambient ambient-b"></div>
        <router-view v-slot="{ Component }">
          <transition name="page" mode="out-in"><component :is="Component" /></transition>
        </router-view>
      </section>
    </main>
    <el-dialog v-model="passwordDialogVisible" title="修改我的密码" width="420px" class="dark-dialog" :close-on-click-modal="false">
      <div class="password-dialog-fields"><el-input v-model="passwordForm.oldPassword" type="password" show-password placeholder="当前密码" /><el-input v-model="passwordForm.newPassword" type="password" show-password placeholder="新密码（至少 6 位）" /><el-input v-model="passwordForm.confirmPassword" type="password" show-password placeholder="确认新密码" @keyup.enter="submitPasswordChange" /></div>
      <template #footer><button class="dialog-cancel" type="button" @click="passwordDialogVisible = false">取消</button><button class="dialog-confirm" type="button" :disabled="passwordSaving" @click="submitPasswordChange">{{ passwordSaving ? '保存中…' : '保存修改' }}</button></template>
    </el-dialog>
  </div>
</template>
