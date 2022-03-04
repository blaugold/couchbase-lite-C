import fs from 'fs'
import os from 'os'
import p from 'path'
import { arenaAutoDelete } from './Allocator.mjs'
import { checkError } from './ErrorUtils.mjs'
import { stringToFLSlice } from './FleeceUtils.mjs'
import { cblite, globals } from './Globals.mjs'

export const testDbName = 'test_db'
export const testDbDir = fs.mkdtempSync(p.join(os.tmpdir(), 'cblite_js_test-'))

export function createDatabaseConfiguration({ directory }) {
  const config = arenaAutoDelete(new cblite.CBLDatabaseConfiguration())
  config.directory = stringToFLSlice(directory)

  // For the Enterprise Edition we need to make sure to initialize the encryption key.
  if (cblite.CBLEncryptionKey) {
    const encryptionKey = arenaAutoDelete(new cblite.CBLEncryptionKey())
    encryptionKey.algorithm = cblite.CBLEncryptionAlgorithm.None
    config.encryptionKey = encryptionKey
  }

  return config
}

export function deleteTestDb() {
  checkError(
    cblite.CBL_DeleteDatabase(
      stringToFLSlice(testDbName),
      stringToFLSlice(testDbDir),
      globals.error
    )
  )
}

export function openTestDb() {
  const config = createDatabaseConfiguration({ directory: testDbDir })

  globals.db = checkError(
    cblite.CBLDatabase_Open(stringToFLSlice(testDbName), config, globals.error)
  )
}

export function closeTestDb() {
  checkError(cblite.CBLDatabase_Close(globals.db, globals.error))
  delete globals.db
}
