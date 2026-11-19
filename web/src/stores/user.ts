import { computed, ref } from 'vue'
import { defineStore } from 'pinia'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

export type UserRole = 'GUEST' | 'USER' | 'ADMIN'

export interface UserSession {
  token: string
  username: string
  role: UserRole
  expiresIn?: number
}

const STORAGE_KEY = 'dir-tree-session'

export const useUserStore = defineStore('user', () => {
  const token = ref('')
  const username = ref('')
  const role = ref<UserRole>('GUEST')
  const expiresIn = ref<number | undefined>()
  const restored = ref(false)

  const isLoggedIn = computed(() => Boolean(token.value))
  const isAdmin = computed(() => role.value === 'ADMIN')
  const displayRole = computed(() => ({ ADMIN: '管理员', USER: '登录用户', GUEST: '访客' })[role.value])
  const initials = computed(() => username.value.trim().slice(0, 2).toUpperCase() || 'GU')

  function setSession(session: UserSession) {
    token.value = session.token
    username.value = session.username
    role.value = session.role
    expiresIn.value = session.expiresIn
    localStorage.setItem(STORAGE_KEY, JSON.stringify(session))
  }

  function restore() {
    if (restored.value) return
    restored.value = true
    try {
      const raw = localStorage.getItem(STORAGE_KEY)
      if (!raw) return
      const session = JSON.parse(raw) as Partial<UserSession>
      if (session.token && session.username && session.role) {
        token.value = session.token
        username.value = session.username
        role.value = session.role
        expiresIn.value = session.expiresIn
      }
    } catch {
      clearSession()
    }
  }

  function clearSession() {
    token.value = ''
    username.value = ''
    role.value = 'GUEST'
    expiresIn.value = undefined
    localStorage.removeItem(STORAGE_KEY)
  }

  return {
    token,
    username,
    role,
    expiresIn,
    isLoggedIn,
    isAdmin,
    displayRole,
    initials,
    setSession,
    restore,
    clearSession
  }
})
