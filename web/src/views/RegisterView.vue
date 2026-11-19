<script setup lang="ts">
import { computed, reactive, ref } from 'vue'
import type { FormInstance, FormRules } from 'element-plus'
import { ElMessage } from 'element-plus'
import { useRouter } from 'vue-router'
import { register } from '@/api/auth'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

const router = useRouter()
const formRef = ref<FormInstance>()
const loading = ref(false)
const accepted = ref(false)
const form = reactive({ username: '', password: '', confirmPassword: '' })

const passwordStrength = computed(() => {
  let score = 0
  if (form.password.length >= 6) score++
  if (form.password.length >= 10) score++
  if (/[A-Z]/.test(form.password) && /[a-z]/.test(form.password)) score++
  if (/\d/.test(form.password) && /[^\w]/.test(form.password)) score++
  return score
})

const rules: FormRules = {
  username: [
    { required: true, message: '请设置用户名', trigger: 'blur' },
    { min: 3, max: 24, message: '使用 3–24 位字符', trigger: 'blur' },
    { pattern: /^[a-zA-Z0-9_-]+$/, message: '仅支持字母、数字、_ 和 -', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请设置密码', trigger: 'blur' },
    { min: 6, message: '密码至少 6 位', trigger: 'blur' }
  ],
  confirmPassword: [{
    validator: (_rule, value, callback) => {
      if (!value) callback(new Error('请再次输入密码'))
      else if (value !== form.password) callback(new Error('两次输入的密码不一致'))
      else callback()
    }, trigger: 'blur'
  }]
}

async function submit() {
  if (!formRef.value) return
  if (!accepted.value) return ElMessage.warning('请先确认使用规则')
  await formRef.value.validate(async (valid) => {
    if (!valid) return
    loading.value = true
    try {
      await register({ username: form.username, password: form.password })
      ElMessage.success('账号创建成功，请登录')
      await router.replace('/login')
    } catch { /* 错误由统一请求层提示 */ } finally {
      loading.value = false
    }
  })
}
</script>

<template>
  <div class="auth-page register-page">
    <div class="auth-grid"></div><div class="auth-orb orb-one"></div><div class="auth-orb orb-two"></div>
    <section class="auth-story register-story">
      <div class="auth-brand"><div class="brand-mark"><span></span><span></span><span></span></div><strong>Tree<span>line</span></strong></div>
      <div class="story-content">
        <div class="eyebrow"><span></span> START YOUR WORKSPACE</div>
        <h1>建立你的<br><em>数字目录空间。</em></h1>
        <p>一个账号，连接浏览、检索、统计与协作。默认以普通用户身份加入。</p>
        <div class="feature-stack">
          <div><i>01</i><p><b>可视化目录</b><span>层级结构一目了然</span></p></div>
          <div><i>02</i><p><b>高效检索</b><span>多条件定位目标节点</span></p></div>
          <div><i>03</i><p><b>安全隔离</b><span>角色与工作区双重保护</span></p></div>
        </div>
      </div>
      <div class="story-footer"><span>02</span><i></i><p>创建身份 · 开始探索</p></div>
    </section>
    <section class="auth-panel">
      <div class="auth-form-wrap register-form-wrap">
        <div class="auth-form-head"><div class="mini-status"><i></i>NEW ACCOUNT</div><h2>创建账号</h2><p>只需要一分钟，开始构建你的目录视图。</p></div>
        <el-form ref="formRef" :model="form" :rules="rules" label-position="top" class="auth-form" @keyup.enter="submit">
          <el-form-item label="用户名" prop="username"><el-input v-model="form.username" size="large" autocomplete="username" placeholder="字母、数字、_ 或 -"><template #prefix><span class="field-symbol">@</span></template></el-input></el-form-item>
          <el-form-item label="密码" prop="password">
            <el-input v-model="form.password" size="large" type="password" autocomplete="new-password" show-password placeholder="至少 6 位字符"><template #prefix><span class="field-symbol">＊</span></template></el-input>
            <div class="strength-meter"><span v-for="index in 4" :key="index" :class="{ on: passwordStrength >= index }"></span><em>{{ ['尚未设置', '基础', '良好', '稳健', '强'][passwordStrength] }}</em></div>
          </el-form-item>
          <el-form-item label="确认密码" prop="confirmPassword"><el-input v-model="form.confirmPassword" size="large" type="password" autocomplete="new-password" show-password placeholder="再次输入密码"><template #prefix><span class="field-symbol">✓</span></template></el-input></el-form-item>
          <label class="agreement-check"><input v-model="accepted" type="checkbox"><span></span><p>我已了解账号仅用于课程项目演示，并同意遵守工作区使用规则。</p></label>
          <button class="primary-auth-button" type="button" :disabled="loading" @click="submit"><span>{{ loading ? '正在创建账号…' : '创建我的账号' }}</span><i>{{ loading ? '···' : '↗' }}</i></button>
        </el-form>
        <div class="auth-switch"><span>已经有账号？</span><router-link to="/login">返回登录 <b>→</b></router-link></div>
      </div>
    </section>
  </div>
</template>
