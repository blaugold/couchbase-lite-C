import { cblite, globals } from './Globals.mjs'

export class Allocator {
  allocate(size) {}

  free(ptr) {}
}

export class MallocAllocator extends Allocator {
  allocate(size) {
    return cblite.CBL_malloc(size)
  }

  free(ptr) {
    cblite.CBL_free(ptr)
  }
}

export const malloc = new MallocAllocator()

export class Arena extends Allocator {
  constructor(allocator) {
    super()
    this.allocator = allocator ?? malloc
  }

  _allocations = []
  _finalizers = []

  allocate(size) {
    const ptr = this.allocator.allocate(size)
    this._allocations.push(ptr)
    return ptr
  }

  free(ptr) {
    this.allocator.free(ptr)
    this._allocations.splice(this._allocations.indexOf(ptr), 1)
  }

  addFinalizer(finalizer) {
    this._finalizers.push(finalizer)
  }

  finalizeAll() {
    this._finalizers.forEach((finalizer) => finalizer())
    this._finalizers = []
    this._allocations.forEach((ptr) => this.free(ptr))
    this._allocations = []
  }
}

export function arenaAutoDelete(value, arena) {
  arena = arena ?? globals.arena
  arena.addFinalizer(() => value.delete())
  return value
}
