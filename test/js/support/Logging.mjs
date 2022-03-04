import { cblite } from './Globals.mjs'

export function setupTestLogging() {
  cblite.CBLLog_SetConsoleLevel(cblite.CBLLogLevel.Info)
}
