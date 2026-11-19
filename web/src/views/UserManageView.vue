<script setup lang="ts">
import { computed, onMounted, reactive, ref, watch } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { UserRecord } from '@/api/auth'
import { listUsers, resetUserPassword, setUserEnabled, setUserRole } from '@/api/auth'
import type { UserRole } from '@/stores/user'
import { useUserStore } from '@/stores/user'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

const currentUser = useUserStore()
const users = ref<UserRecord[]>([])
const loading = ref(false)
const saving = ref<string | null>(null)
const query = ref('')
const roleFilter = ref<'ALL' | UserRole>('ALL')
const page = ref(1)
const pageSize = ref(10)
const resetDialogVisible = ref(false)
const resetTarget = ref<UserRecord | null>(null)
const resetForm = reactive({ password: '' })

const filteredUsers = computed(() => users.value.filter((item) => {
  const matchesQuery = !query.value || item.username.toLowerCase().includes(query.value.toLowerCase())
  const matchesRole = roleFilter.value === 'ALL' || item.role === roleFilter.value
  return matchesQuery && matchesRole
}))
const pagedUsers = computed(() => filteredUsers.value.slice((page.value - 1) * pageSize.value, page.value * pageSize.value))

const activeCount = computed(() => users.value.filter((item) => item.enabled).length)
const adminCount = computed(() => users.value.filter((item) => item.role === 'ADMIN').length)
const roleLabel = (role: UserRole) => ({ ADMIN: '管理员', USER: '普通用户', GUEST: '访客' })[role]

function resetPage() {
  page.value = 1
}

watch([query, roleFilter], resetPage)

async function loadUsers() {
  loading.value = true
  try {
    users.value = await listUsers()
  } catch {
    users.value = []
  } finally {
    loading.value = false
  }
}

async function toggleEnabled(item: UserRecord) {
  if (item.username === currentUser.username) return ElMessage.warning('不能停用当前登录账号')
  saving.value = item.username
  try {
    await setUserEnabled(item.username, !item.enabled)
    item.enabled = !item.enabled
    ElMessage.success(item.enabled ? `${item.username} 已启用` : `${item.username} 已停用`)
  } catch { /* 错误由统一请求层提示 */ } finally {
    saving.value = null
  }
}

async function updateRole(item: UserRecord, role: UserRole, select: HTMLSelectElement) {
  if (item.username === currentUser.username) {
    select.value = item.role
    return ElMessage.warning('不能修改自己的角色')
  }
  try {
    await ElMessageBox.confirm(`确认将 ${item.username} 的角色调整为“${roleLabel(role)}”？`, '确认角色变更', {
      type: 'warning', confirmButtonText: '确认变更', cancelButtonText: '取消'
    })
  } catch {
    select.value = item.role
    return
  }
  saving.value = item.username
  try {
    await setUserRole(item.username, role)
    item.role = role
    ElMessage.success('角色已更新')
  } catch {
    select.value = item.role
  } finally {
    saving.value = null
  }
}

function openReset(item: UserRecord) {
  resetTarget.value = item
  resetForm.password = ''
  resetDialogVisible.value = true
}

async function confirmReset() {
  if (!resetTarget.value || resetForm.password.length < 6) return ElMessage.warning('新密码至少需要 6 位')
  saving.value = resetTarget.value.username
  try {
    await resetUserPassword(resetTarget.value.username, resetForm.password)
    ElMessage.success(`已重置 ${resetTarget.value.username} 的密码`)
    resetDialogVisible.value = false
  } catch { /* 错误由统一请求层提示 */ } finally {
    saving.value = null
  }
}

async function confirmDisable(item: UserRecord) {
  if (item.enabled) {
    try { await ElMessageBox.confirm(`停用后，${item.username} 将无法登录工作台。`, '确认停用账号', { type: 'warning', confirmButtonText: '确认停用', cancelButtonText: '取消' }) } catch { return }
  }
  await toggleEnabled(item)
}

onMounted(loadUsers)
</script>

<template>
  <div class="workspace-page users-page">
    <div class="page-heading">
      <div><div class="eyebrow"><span></span> ACCESS CONTROL / 06</div><h1>成员权限</h1><p>管理工作区成员、角色与访问状态。</p></div>
      <button class="ghost-button" type="button" :disabled="loading" @click="loadUsers"><span>↻</span>刷新列表</button>
    </div>

    <div class="metric-row">
      <div class="metric-card metric-primary"><span class="metric-index">01</span><div><small>成员总数</small><strong>{{ users.length }}</strong><em>个工作区账号</em></div><i>♙</i></div>
      <div class="metric-card"><span class="metric-index">02</span><div><small>当前启用</small><strong>{{ activeCount }}</strong><em>可正常访问</em></div><i class="green-icon">●</i></div>
      <div class="metric-card"><span class="metric-index">03</span><div><small>管理员</small><strong>{{ adminCount }}</strong><em>拥有完整权限</em></div><i>◇</i></div>
      <div class="metric-card metric-tip"><div><small>权限原则</small><strong>最小授权</strong><em>所有变更均会被记录</em></div><i>↗</i></div>
    </div>

    <section class="content-card member-card">
      <div class="card-toolbar">
        <div><span class="section-kicker">DIRECTORY</span><h2>成员目录 <sup>{{ filteredUsers.length }}</sup></h2></div>
        <div class="toolbar-controls"><div class="search-box"><span>⌕</span><input v-model="query" placeholder="搜索成员…"></div><select v-model="roleFilter"><option value="ALL">全部角色</option><option value="ADMIN">管理员</option><option value="USER">普通用户</option><option value="GUEST">访客</option></select></div>
      </div>
      <div v-loading="loading" class="member-table-wrap">
        <table class="member-table"><thead><tr><th>成员</th><th>角色</th><th>最近登录</th><th>状态</th><th class="align-right">管理操作</th></tr></thead>
          <tbody>
            <tr v-for="item in pagedUsers" :key="item.username">
              <td><div class="member-identity"><span class="member-avatar" :class="item.role.toLowerCase()">{{ item.username.slice(0, 2).toUpperCase() }}</span><div><strong>{{ item.username }}</strong><small>{{ item.username === currentUser.username ? '这是你当前的账号' : '工作区成员' }}</small></div></div></td>
              <td><select class="role-select" :value="item.role" :disabled="item.username === currentUser.username || saving === item.username" @change="updateRole(item, ($event.target as HTMLSelectElement).value as UserRole, $event.target as HTMLSelectElement)"><option value="ADMIN">管理员</option><option value="USER">普通用户</option><option value="GUEST">访客</option></select></td>
              <td><span class="last-login">{{ item.lastLoginAt || '尚未登录' }}</span></td>
              <td><span class="status-pill" :class="item.enabled ? 'online' : 'offline'"><i></i>{{ item.enabled ? '已启用' : '已停用' }}</span></td>
              <td class="align-right"><div class="row-actions"><button type="button" class="text-action" @click="openReset(item)">重置密码</button><button type="button" class="text-action" :class="{ danger: item.enabled }" :disabled="item.username === currentUser.username || saving === item.username" @click="confirmDisable(item)">{{ item.enabled ? '停用' : '启用' }}</button></div></td>
            </tr>
            <tr v-if="!loading && !filteredUsers.length"><td colspan="5"><div class="empty-state"><span>⌁</span><b>没有匹配的成员</b><small>尝试更换搜索关键词或筛选条件</small></div></td></tr>
          </tbody>
        </table>
      </div>
      <div class="table-foot"><span>权限由服务端强制校验，前端菜单隐藏不构成安全边界。</span><b>ROLE / {{ roleLabel(currentUser.role) }}</b></div>
      <el-pagination v-if="filteredUsers.length > pageSize" class="member-pagination" :current-page="page" :page-size="pageSize" :page-sizes="[10, 20, 50]" :total="filteredUsers.length" layout="total, sizes, prev, pager, next" background @current-change="page = $event" @size-change="pageSize = $event; resetPage()" />
    </section>

    <el-dialog v-model="resetDialogVisible" title="重置成员密码" width="420px" class="dark-dialog" :close-on-click-modal="false">
      <p class="dialog-copy">为 <b>{{ resetTarget?.username }}</b> 设置一个新的临时密码。</p>
      <el-input v-model="resetForm.password" type="password" show-password placeholder="输入至少 6 位的新密码" @keyup.enter="confirmReset" />
      <template #footer><button class="dialog-cancel" type="button" @click="resetDialogVisible = false">取消</button><button class="dialog-confirm" type="button" @click="confirmReset">确认重置</button></template>
    </el-dialog>
  </div>
</template>
