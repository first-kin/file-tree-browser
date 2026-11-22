import { http } from './request'
export const statsApi = { count:()=>http.get('/stats/count'), size:()=>http.get('/stats/total-size'), depth:()=>http.get('/stats/max-depth'), top:(n=10)=>http.get('/stats/top-n',{params:{n}}), empty:()=>http.get('/stats/empty-dirs'), levels:()=>http.get('/stats/by-level') }
