import { request } from './request'

// AI生成，需提交者人工审查后将本注释改为“AI生成，已手动审查修改”。

export interface MerkleBuildResult {
  rootHash: string
}

export interface MerkleDiffResult {
  same: boolean
  diffFiles: string[]
}

export interface GraphBuildResult {
  nodeCount: number
  edgeCount: number
}

export interface GraphCyclesResult {
  cycles: string[][]
}

export function buildMerkle(path: string) {
  return request<MerkleBuildResult>({ method: 'POST', url: '/merkle/build', data: { path } })
}

export function diffMerkle(pathA: string, pathB: string) {
  return request<MerkleDiffResult>({ method: 'POST', url: '/merkle/diff', data: { pathA, pathB } })
}

export function buildGraph() {
  return request<GraphBuildResult>({ method: 'POST', url: '/graph/build' })
}

export function getGraphCycles() {
  return request<GraphCyclesResult>({ method: 'GET', url: '/graph/cycles' })
}
