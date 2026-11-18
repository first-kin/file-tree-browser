import { request } from './request'
import type { UserRole, UserSession } from '@/stores/user'

export interface UserRecord {
  username: string
  role: UserRole
  enabled: boolean
  lastLoginAt?: string
}

export interface LoginPayload {
  username: string
  password: string
}

export function login(payload: LoginPayload) {
  return request<UserSession>({ method: 'POST', url: '/api/auth/login', data: payload })
}

export function register(payload: LoginPayload) {
  return request<void>({ method: 'POST', url: '/api/auth/register', data: payload })
}

export function logout() {
  return request<void>({ method: 'POST', url: '/api/auth/logout' })
}

export function getMe() {
  return request<{ username: string; role: UserRole }>({ method: 'GET', url: '/api/auth/me' })
}

export function changePassword(payload: { oldPassword: string; newPassword: string }) {
  return request<void>({ method: 'PUT', url: '/api/auth/password', data: payload })
}

export function listUsers() {
  return request<UserRecord[]>({ method: 'GET', url: '/api/users' })
}

export function setUserEnabled(username: string, enabled: boolean) {
  return request<void>({ method: 'PUT', url: `/api/users/${encodeURIComponent(username)}/enabled`, data: { enabled } })
}

export function resetUserPassword(username: string, newPassword: string) {
  return request<void>({ method: 'PUT', url: `/api/users/${encodeURIComponent(username)}/password`, data: { newPassword } })
}

export function setUserRole(username: string, role: UserRole) {
  return request<void>({ method: 'PUT', url: `/api/users/${encodeURIComponent(username)}/role`, data: { role } })
}
