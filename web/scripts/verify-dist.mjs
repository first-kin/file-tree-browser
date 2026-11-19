// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。
import { access, readFile } from 'node:fs/promises'
import { constants } from 'node:fs'
import { resolve } from 'node:path'

const distRoot = resolve('dist')
const indexPath = resolve(distRoot, 'index.html')

await access(indexPath, constants.R_OK)
const html = await readFile(indexPath, 'utf8')

if (!html.includes('<div id="app"></div>')) {
  throw new Error('dist/index.html 缺少 Vue 挂载节点')
}

if (!/assets\/index-[^"']+\.js/.test(html) || !/assets\/index-[^"']+\.css/.test(html)) {
  throw new Error('dist/index.html 未引用生产 JS/CSS 资源')
}

console.log(`生产构建检查通过：${indexPath}`)
