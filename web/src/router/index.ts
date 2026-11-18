import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router'
import { useUserStore, type UserRole } from '@/stores/user'

const routes: RouteRecordRaw[] = [
  {
    path: '/login',
    name: 'login',
    component: () => import('@/views/LoginView.vue'),
    meta: { guestOnly: true }
  },
  {
    path: '/register',
    name: 'register',
    component: () => import('@/views/RegisterView.vue'),
    meta: { guestOnly: true }
  },
  {
    path: '/',
    component: () => import('@/layouts/MainLayout.vue'),
    meta: { requiresAuth: true },
    children: [
      { path: '', redirect: '/tree' },
      { path: 'tree', name: 'tree', component: () => import('@/views/TreeView.vue'), meta: { label: '目录空间' } },
      { path: 'search', name: 'search', component: () => import('@/views/SearchView.vue'), meta: { label: '智能查找' } },
      { path: 'stats', name: 'stats', component: () => import('@/views/StatsView.vue'), meta: { label: '数据洞察' } },
      { path: 'users', name: 'users', component: () => import('@/views/UserManageView.vue'), meta: { label: '成员权限', roles: ['ADMIN'] as UserRole[] } },
      { path: 'merkle', name: 'merkle', component: () => import('@/views/P2View.vue'), meta: { label: '完整性校验', roles: ['ADMIN'] as UserRole[] } },
      { path: 'graph', name: 'graph', component: () => import('@/views/P2View.vue'), meta: { label: '环路检测', roles: ['ADMIN'] as UserRole[] } }
    ]
  },
  { path: '/:pathMatch(.*)*', redirect: '/' }
]

const router = createRouter({
  history: createWebHistory(),
  routes,
  scrollBehavior: () => ({ top: 0 })
})

router.beforeEach((to) => {
  const user = useUserStore()
  user.restore()

  if (to.meta.guestOnly && user.isLoggedIn) return '/tree'
  if (to.meta.requiresAuth && !user.isLoggedIn) {
    return { path: '/login', query: { redirect: to.fullPath } }
  }

  const roles = to.meta.roles as UserRole[] | undefined
  if (roles && !roles.includes(user.role)) return '/tree'
  return true
})

export default router
