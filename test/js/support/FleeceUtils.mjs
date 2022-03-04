import { Arena } from './Allocator.mjs'
import { cblite, globals } from './Globals.mjs'

export function consumeSliceToString(slice) {
  const string = cblite.FLSliceToString(slice)
  slice.delete()
  return string
}

export function consumeSliceResultToString(sliceResult) {
  const string = cblite.FLSliceToString(sliceResult)
  cblite.FLSliceResult_Release(sliceResult)
  sliceResult.delete()
  return string
}

export function stringToFLSlice(string, allocator) {
  allocator = allocator ?? globals.arena

  const slice = new cblite.FLSlice()
  cblite.stringToFLSlice(string, slice, allocator.allocate.bind(allocator))

  if (allocator instanceof Arena) {
    allocator.addFinalizer(() => slice.delete())
  }

  return slice
}
