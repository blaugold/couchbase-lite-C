import { consumeSliceResultToString } from './FleeceUtils.mjs'
import { cblite, globals } from './Globals.mjs'

export function checkError(result, error) {
  error = error ?? globals.error
  if (result == 0) {
    throw CouchbaseLiteError.fromCBLError(error)
  }

  return result
}

export class CouchbaseLiteError extends Error {
  constructor(domain, code, message) {
    super(`CouchbaseLiteError(domain: ${domain}, code: ${code}, message: ${message})`)

    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, CouchbaseLiteError)
    }

    this.domain = domain
    this.code = code
    this.errorMessage = message
  }

  static fromCBLError(error) {
    const domain = error.domain.constructor.name.replace('CBLErrorDomain_', '')

    let code
    switch (domain) {
      case 'CBL':
        code = cblite.CBLErrorCode.values[error.code].constructor.name.replace(
          'CBLErrorCode_',
          ''
        )
        break
      case 'Network':
        code = cblite.CBLNetworkErrorCode.values[
          error.code
        ].constructor.name.replace('CBLNetworkErrorCode_', '')
        break
      case 'POSIX':
      case 'SQLite':
      case 'Fleece':
      case 'WebSocket':
        code = error.code
        break
    }

    const message = consumeSliceResultToString(cblite.CBLError_Message(error))

    return new CouchbaseLiteError(domain, code, message)
  }
}
