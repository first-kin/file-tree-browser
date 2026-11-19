import { request } from './request'
import type { UserRole, UserSession } from '@/stores/user'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

export interface UserRecord {
  username: string
  role: UserRole
  enabled: boolean
  lastLoginAt?: number
}

export interface LoginPayload {
  username: string
  password: string
}

export function login(payload: LoginPayload) {
  return request<UserSession>({ method: 'POST', url: '/auth/login', data: payload })
}

export function register(payload: LoginPayload) {
  return request<void>({ method: 'POST', url: '/auth/register', data: payload })
}

export function logout() {
  return request<void>({ method: 'POST', url: '/auth/logout' })
}

export function getMe() {
  return request<{ username: string; role: UserRole }>({ method: 'GET', url: '/auth/me' })
}

export function changePassword(payload: { oldPassword: string; newPassword: string }) {
  return request<void>({ method: 'PUT', url: '/auth/password', data: payload })
}

export function listUsers() {
  return request<UserRecord[]>({ method: 'GET', url: '/users' })
}

export function setUserEnabled(username: string, enabled: boolean) {
  return request<void>({ method: 'PUT', url: `/users/${encodeURIComponent(username)}/enabled`, data: { enabled } })
}

export function resetUserPassword(username: string, newPassword: string) {
  return request<void>({ method: 'PUT', url: `/users/${encodeURIComponent(username)}/password`, data: { newPassword } })
}

export function setUserRole(username: string, role: UserRole) {
  return request<void>({ method: 'PUT', url: `/users/${encodeURIComponent(username)}/role`, data: { role } })
}
