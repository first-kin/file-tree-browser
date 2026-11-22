import { createRouter, createWebHistory } from 'vue-router'
import { useUserStore } from '../stores/user'
import MainLayout from '../layouts/MainLayout.vue'
const router = createRouter({history:createWebHistory(),routes:[{path:'/login',component:()=>import('../views/LoginView.vue'),meta:{guest:true}},{path:'/register',component:()=>import('../views/RegisterView.vue'),meta:{guest:true}},{path:'/',component:MainLayout,children:[{path:'',redirect:'/tree'},{path:'tree',component:()=>import('../views/TreeView.vue'),meta:{title:'目录浏览'}},{path:'search',component:()=>import('../views/SearchView.vue'),meta:{title:'智能查找'}},{path:'stats',component:()=>import('../views/StatsView.vue'),meta:{title:'统计分析'}},{path:'graph',component:()=>import('../views/P2View.vue'),meta:{title:'图与环检测'}},{path:'users',component:()=>import('../views/UserManageView.vue'),meta:{title:'用户管理',admin:true}}]}]})
router.beforeEach(to=>{const store=useUserStore();if(to.matched.some(r=>r.meta.guest))return store.username?'/tree':true;if(!store.username)return '/login';if(to.matched.some(r=>r.meta.admin)&&!store.isAdmin)return '/tree';return true})
export default router
