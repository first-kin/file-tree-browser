import axios, { type AxiosRequestConfig, type AxiosResponse } from 'axios'
import { ElMessage } from 'element-plus'
import { useUserStore } from '@/stores/user'
import router from '@/router'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

export interface ApiResponse<T = unknown> {
  code: number
  message: string
  data: T
}

export const http = axios.create({
  baseURL: import.meta.env.VITE_API_BASE || '/api',
  timeout: 12_000,
  headers: { 'Content-Type': 'application/json' }
})

http.interceptors.request.use((config) => {
  const user = useUserStore()
  user.restore()
  if (user.token) config.headers.Authorization = `Bearer ${user.token}`
  return config
})

http.interceptors.response.use(
  (response) => {
    const payload = response.data as Partial<ApiResponse>
    if (typeof payload.code === 'number' && payload.code !== 0) {
      const message = payload.message || '请求失败'
      if (payload.code === -11) {
        const user = useUserStore()
        user.clearSession()
        if (router.currentRoute.value.path !== '/login') {
          void router.push({ path: '/login', query: { redirect: router.currentRoute.value.fullPath } })
        }
      }
      ElMessage.error(message)
      return Promise.reject(new Error(message))
    }
    return response
  },
  (error) => {
    const message = error?.response?.data?.message || error?.message || '网络连接失败，请稍后重试'
    ElMessage.error(message)
    return Promise.reject(error)
  }
)

export async function request<T = unknown>(config: AxiosRequestConfig): Promise<T> {
  const response: AxiosResponse<ApiResponse<T> | T> = await http.request(config)
  const payload = response.data
  if (payload && typeof payload === 'object' && 'code' in payload && 'data' in payload) {
    return (payload as ApiResponse<T>).data
  }
  return payload as T
}
