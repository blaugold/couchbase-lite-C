Module['FLSliceToString'] = (slice) => {
  if (slice.buf === 0) {
    return null
  }
  return UTF8ToString(slice.buf, slice.size)
}

Module['stringToFLSlice'] = (string, slice, allocate) => {
  if (!string) {
    slice.buf = 0
    slice.size = 0
    return
  }
  const bufferSize = lengthBytesUTF8(string) + 1
  const buffer = (allocate ?? Module.CBL_malloc)(bufferSize)
  stringToUTF8(string, buffer, bufferSize)
  slice.buf = buffer
  slice.size = bufferSize - 1
}

Module['removeCallback'] = removeFunction

Module['addLogCallback'] = (callback) =>
  addFunction((logMessagePtr) => {
    const logMessage = Module['_CBLLog_LogMessage_FromPointer'](logMessagePtr)

    try {
      callback(logMessage.domain, logMessage.level, logMessage.message)
    } finally {
      logMessage.delete()
    }
  }, 'vi')

Module['addDatabaseChangeCallback'] = (callback) =>
  addFunction((_, db, numDocs, docIDs) => {
    callback(
      db,
      numDocs,
      Array.from({ length: numDocs }, (_, i) =>
        Module.FLSlice_FromPointer(docIDs, i)
      )
    )
  }, 'viiii')

Module['addConflictHandlerCallback'] = (callback) =>
  addFunction((documentBeingSavedPtr, conflictingDocumentPtr) => {
    return callback(documentBeingSavedPtr, conflictingDocumentPtr)
  }, 'iii')

Module['addDocumentChangeCallback'] = (callback) =>
  addFunction((_, db, docID) => {
    callback(db, Module.FLSlice_FromPointer(docID, 0))
  }, 'viii')
