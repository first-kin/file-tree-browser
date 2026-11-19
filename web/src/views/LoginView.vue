<script setup lang="ts">
import { reactive, ref } from 'vue'
import type { FormInstance, FormRules } from 'element-plus'
import { ElMessage } from 'element-plus'
import { useRoute, useRouter } from 'vue-router'
import { login } from '@/api/auth'
import { useUserStore } from '@/stores/user'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

const route = useRoute()
const router = useRouter()
const user = useUserStore()
const formRef = ref<FormInstance>()
const loading = ref(false)
const showPassword = ref(false)
const form = reactive({ username: '', password: '' })

const rules: FormRules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 24, message: '用户名长度应为 3–24 位', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, message: '密码至少 6 位', trigger: 'blur' }
  ]
}

async function submit() {
  if (!formRef.value) return
  await formRef.value.validate(async (valid) => {
    if (!valid) return
    loading.value = true
    try {
      const session = await login(form)
      user.setSession(session)
      ElMessage.success(`欢迎回来，${session.username}`)
      const requestedRedirect = typeof route.query.redirect === 'string' ? route.query.redirect : ''
      const redirect = requestedRedirect.startsWith('/') && !requestedRedirect.startsWith('//')
        ? requestedRedirect
        : '/tree'
      await router.replace(redirect)
    } catch { /* 错误由统一请求层提示 */ } finally {
      loading.value = false
    }
  })
}
</script>

<template>
  <div class="auth-page">
    <div class="auth-grid"></div>
    <div class="auth-orb orb-one"></div><div class="auth-orb orb-two"></div>

    <section class="auth-story">
      <div class="auth-brand"><div class="brand-mark"><span></span><span></span><span></span></div><strong>Tree<span>line</span></strong></div>
      <div class="story-content">
        <div class="eyebrow"><span></span> FILE SYSTEM / VISUALIZED</div>
        <h1>让复杂目录<br><em>变得清晰可见。</em></h1>
        <p>从磁盘结构到可交互的数据视图，快速检索、分析并管理每一个节点。</p>
        <div class="story-metrics">
          <div><b>10K+</b><span>节点流畅浏览</span></div>
          <div><b>3</b><span>角色权限模型</span></div>
          <div><b>SHA</b><span>完整性校验</span></div>
        </div>
      </div>
      <div class="story-footer"><span>01</span><i></i><p>探索结构 · 洞察空间</p></div>
    </section>

    <section class="auth-panel">
      <div class="auth-form-wrap">
        <div class="auth-form-head">
          <div class="mini-status"><i></i>SECURE ACCESS</div>
          <h2>欢迎回来</h2>
          <p>登录你的工作台，继续管理目录空间。</p>
        </div>

        <el-form ref="formRef" :model="form" :rules="rules" label-position="top" class="auth-form" @keyup.enter="submit">
          <el-form-item label="用户名" prop="username">
            <el-input v-model="form.username" size="large" autocomplete="username" placeholder="输入用户名">
              <template #prefix><span class="field-symbol">@</span></template>
            </el-input>
          </el-form-item>
          <el-form-item label="密码" prop="password">
            <el-input v-model="form.password" size="large" :type="showPassword ? 'text' : 'password'" autocomplete="current-password" placeholder="输入登录密码">
              <template #prefix><span class="field-symbol">＊</span></template>
              <template #suffix><button type="button" class="reveal-button" @click="showPassword = !showPassword">{{ showPassword ? '隐藏' : '显示' }}</button></template>
            </el-input>
          </el-form-item>
          <button class="primary-auth-button" type="button" :disabled="loading" @click="submit">
            <span>{{ loading ? '正在验证身份…' : '进入工作台' }}</span><i>{{ loading ? '···' : '↗' }}</i>
          </button>
        </el-form>

        <div class="auth-switch"><span>还没有账号？</span><router-link to="/register">创建新账号 <b>→</b></router-link></div>
        <div class="auth-switch guest-entry"><span>暂不登录？</span><router-link to="/tree">以访客身份只读浏览 <b>→</b></router-link></div>
        <div class="security-note"><span>◇</span><p><b>安全提示</b><small>系统仅保存服务端生成的会话标识，不在浏览器保存明文密码。</small></p></div>
      </div>
    </section>
  </div>
</template>
