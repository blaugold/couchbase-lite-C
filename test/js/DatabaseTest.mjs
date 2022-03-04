import assert from 'assert'
import p from 'path'
import { arenaAutoDelete } from './support/Allocator.mjs'
import {
  closeTestDb,
  deleteTestDb,
  openTestDb,
  testDbDir,
  testDbName,
} from './support/DatabaseUtils.mjs'
import { checkError } from './support/ErrorUtils.mjs'
import {
  consumeSliceResultToString,
  consumeSliceToString,
  stringToFLSlice,
} from './support/FleeceUtils.mjs'
import { cblite, globals, initializeGlobals } from './support/Globals.mjs'
import { setupTestLogging } from './support/Logging.mjs'

let db

suiteSetup(initializeGlobals)

setup(() => {
  setupTestLogging()
  deleteTestDb()
  openTestDb()
  db = globals.db
})

teardown(() => {
  closeTestDb()
  deleteTestDb()
  globals.arena.finalizeAll()
})

suite('CBLDatabase', () => {
  test('file operations', () => {
    // Copy test db
    const testDbPath = consumeSliceResultToString(cblite.CBLDatabase_Path(db))
    const copyDbName = 'copy'
    checkError(
      cblite.CBL_CopyDatabase(
        stringToFLSlice(testDbPath),
        stringToFLSlice(copyDbName),
        arenaAutoDelete(cblite.CBLDatabase_Config(db)),
        globals.error
      )
    )

    // Check that copy exists
    assert.ok(
      cblite.CBL_DatabaseExists(
        stringToFLSlice(copyDbName),
        stringToFLSlice(testDbDir)
      )
    )

    // Delete copy
    checkError(
      cblite.CBL_DeleteDatabase(
        stringToFLSlice(copyDbName),
        stringToFLSlice(testDbDir),
        globals.error
      )
    )

    // Check that copy does not exist
    assert.ok(
      !cblite.CBL_DatabaseExists(
        stringToFLSlice(copyDbName),
        stringToFLSlice(testDbDir)
      )
    )
  })

  test('properties', () => {
    const name = consumeSliceToString(cblite.CBLDatabase_Name(db))
    assert.equal(name, testDbName)

    const path = consumeSliceResultToString(cblite.CBLDatabase_Path(db))
    assert.equal(path, p.resolve(testDbDir, `${testDbName}.cblite2`) + p.sep)

    const count = cblite.CBLDatabase_Count(db)
    assert.equal(count, 0)

    const config = arenaAutoDelete(cblite.CBLDatabase_Config(db))
    const directory = consumeSliceToString(config.directory)
    assert.equal(directory, testDbDir + p.sep)
  })
})
