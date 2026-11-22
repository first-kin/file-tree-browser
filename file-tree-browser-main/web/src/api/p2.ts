import { http } from './request'
export const graphApi={build:()=>http.post('/graph/build'),cycles:()=>http.get('/graph/cycles')}
