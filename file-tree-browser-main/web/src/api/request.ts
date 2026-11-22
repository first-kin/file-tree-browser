import axios from 'axios'
import { ElMessage } from 'element-plus'
import router from '../router'

export const http = axios.create({ baseURL: '/api', timeout: 15000 })
http.interceptors.request.use(config => { const token = localStorage.getItem('ftb_token'); if (token) config.headers.Authorization = `Bearer ${token}`; return config })
http.interceptors.response.use(response => { const body = response.data; if (body && typeof body.code === 'number' && body.code !== 0 && body.code !== 200) return Promise.reject(new Error(body.message || '请求失败')); return body?.data !== undefined ? { ...response, data: body.data } : response }, error => { if (error.response?.status === 401) { localStorage.removeItem('ftb_token'); localStorage.removeItem('ftb_user'); router.push('/login') } ElMessage.error(error.response?.data?.message || error.message || '网络请求失败'); return Promise.reject(error) })
