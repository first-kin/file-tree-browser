import { http } from './request'
export const searchApi = { name:(data:object)=>http.post('/search/name',data), pattern:(data:object)=>http.post('/search/pattern',data), ext:(data:object)=>http.post('/search/ext',data), size:(data:object)=>http.post('/search/size',data), time:(data:object)=>http.post('/search/time',data), duplicates:()=>http.get('/search/duplicates') }
