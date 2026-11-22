import { http } from './request'
export const login = (data: {username:string;password:string}) => http.post('/auth/login', data)
export const register = (data: {username:string;password:string}) => http.post('/auth/register', data)
export const me = () => http.get('/auth/me')
export const logout = () => http.post('/auth/logout')
export const users = () => http.get('/users')
export const updateUser = (name:string, data:object) => http.put(`/users/${encodeURIComponent(name)}/enabled`, data)
export const setRole = (name:string, role:string) => http.put(`/users/${encodeURIComponent(name)}/role`, {role})
