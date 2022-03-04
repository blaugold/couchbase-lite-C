import cbliteJs from '../../../build/cblite_js.js'
import { Arena } from './Allocator.mjs'

export const cblite = {}
export const globals = {}

let initialize

export async function initializeGlobals() {
  if (!initialize) {
    initialize = (async () => {
      Object.assign(cblite, await cbliteJs())
      globals.error = new cblite.CBLError()
      globals.arena = new Arena()
    })()
  }

  return initialize
}
