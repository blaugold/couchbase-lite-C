# CBL C + Emscripten

This is a proof of concept (POC) for porting Couchbase Lite to Emscripten.

# Getting started

## Dependencies

### CMake

Install [CMake] and ensure it is available on the PATH.

The most recent tested version of CMake is `3.23.2`.

### NodeJS

Install [NodeJS] and ensure it is available on the PATH.

The most recent tested version of NodeJS is `16.13.1`.

### Emscripten

Install the [Emscripten SDK][ems sdk]. The SDK comes with the emsdk tool, which
allows you to install and activate specific versions of the SDK.

```
# Get the emsdk repo
git clone https://github.com/emscripten-core/emsdk.git

# Enter that directory
cd emsdk

# Fetch the latest version of the emsdk (not needed the first time you clone)
git pull

# Download and install the latest SDK tools.
./emsdk install latest

# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
./emsdk activate latest

# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh
```

The most recent tested version of emscripten is `3.1.12`.

## Get the Emscripten branch

```
git clone https://github.com/blaugold/couchbase-lite-C.git -b emscripten
cd couchbase-lite-C
git submodule update --init --recursive
```

## Install npm dependencies

```
npm install
```

This installs dependencies used for testing.

## Build with Emscripten

```
./scripts/build_emscripten.sh
```

This builds a debug version of the project with the emscripten toolchain.

## Run tests

Run tests written in C with NodeJS:

```
npm run test:c
```

A few tests that use the network are failing because Emscripten only has limited
support for [POSIX sockets][ems_networking].

Run tests written in JS (in test/js) with NodeJS:

```
npm run test:js
```

# Porting CBL C to Emscripten

This POC is a bit behind upstream, but the insights should still apply.

## Dependencies

Some dependencies had to be modified to support compiling with Emscripten:

- [LiteCore](https://github.com/blaugold/couchbase-lite-core/tree/emscripten)
- [Fleece](https://github.com/blaugold/fleece/tree/emscripten)
- [sockpp](https://github.com/blaugold/sockpp/tree/emscripten)

## Tests

The default set of tests in `Fleece`, `LiteCore` and `CBL_C` are passing on
NodeJS with a handful of exceptions, which are related to POSIX sockets not
being available.

This excludes EE tests and tests which use a live Sync Gateway.

## Emscripten options

A number of compiler options are required to build Couchbase Lite with
Emscripten. These in turn require a number of WASM features. See the [WASM
Roadmap][wasm_roadmap] for the state of feature implementations across WASM
engines.

### Exception handling

Support for exception handling is necessary because LiteCore uses C++
exceptions.

Emscripten supports efficient, WASM based exception handling through the
[`-fwasm-exceptions`][ems_wasm_exceptions] flag.

Most WASM engines shipped by browsers already support the required WASM feature
and in NodeJS it is available behind an experimental flag
(`--experimental-wasm-eh`).

### Multi-threading

Emscripten supports [pthreads][ems_pthreads] through the `-pthread` flag.

This requires WASM threading support, which is already available in most WASM
engines shipped by browsers as well as NodeJS.

Note that `SharedArrayBuffer`, which is part of the WASM threading support,
[requires sites to enable cross-origin isolation](https://developer.chrome.com/blog/enabling-shared-array-buffer/)
on the web.

In JS environments WASM code always runs in the context of a JS thread which has
transitioned to executing WASM code by calling a WASM function that has been
exported from a WASM module. It is dangerous for WASM code to synchronously
create threads from the main JS thread. For example creating a thread and
immediately joining it will lead to a deadlock. After a thread has been created
from the main JS thread it needs to yield to the event loop before waiting on
the new thread to allow for the creation of the backing JS worker.

This is only an issue for code that runs as a consequence of a call of a public
API and therefore possibly on the main thread. Code that runs in a thread
created by Couchbase Lite does not have this limitation. As far as I can tell
there are no instances of code in Couchbase Lite which are problematic in this
way.

### Memory allocation

The default for Emscripten is to preallocates a WASM memory of fixed size when
creating the WASM module instance.

We would like to allow the memory allocated for a Couchbase Lite WASM instance
to be allocated dynamically. When growable memory is combined with pthreads,
accessing that memory from JS can have
[bad performance](https://gist.github.com/kripken/949eab99b7bc34f67c12140814d2b595),
especially for code that does a lot of individual loads and stores. A
corresponding warning messages is logged during compilation.

We need to read WASM memory for example when a C function returns a string.
Reading chunks of contiguous memory still does not have optimal performance but
is much less problematic (e.g. for a string).

### Back traces

When `DEMANGLE_SUPPORT` is enabled [`emscripten_get_callstack`][ems_callstack]
can be used to get symbolicated stack traces.

[Backtrace.cc](https://github.com/blaugold/fleece/blob/e5560e9de872a1fb3b638c6bef2c74c3ded24a6f/Fleece/Support/Backtrace.cc#L387-L403)
makes use of this API.

## Platforms

Since CBL is already available on many platforms as native binaries, the most
interesting target for a WASM build is the browser. In general, it would be
possible to distribute CBL as a WASM module that can be used with any WASM
engine embedder, such as NodeJS, [Wasmer](https://wasmer.io/) or
[Wasmtime](https://wasmtime.dev/).

What it comes down to is which APIs are available on the embedder and how to
integrate them. This POC focuses on JS environments, such as NodeJS and the
Browser and makes use of the APIs available there.

## APIs

### Collation

The collation implementation is based on the
[`Intl.Collator`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Intl/Collator)
JS API in
[UnicodeCollator_JS.cc](https://github.com/blaugold/couchbase-lite-core/blob/9e3f4d7af598e582f59291cc2481a20fb335cd4a/LiteCore/Storage/UnicodeCollator_JS.cc).

Changing string casing is implemented in
[StringUtil_JS.cc](https://github.com/blaugold/couchbase-lite-core/blob/9e3f4d7af598e582f59291cc2481a20fb335cd4a/LiteCore/Support/StringUtil_JS.cc)
through the `toUpperCase` and `toLowerCase` methods of the JS `String` class.

### WebSocket

[WebWebSocket.cc](src/WebWebSocket.cc) implements a `C4SocketFactory` based on
the `WebSocket` JS API. It does not implement back pressure in the transmitting
direction, yet. `WebSocket` does not support back pressure in the receiving
direction. There is a proposal for the
[`WebSocketStream`](https://web.dev/websocketstream/) API to address this
limitation.

NodeJS does not support the `WebSocket` API, so it either would need to be
shimmed or an alternative implementation of a `C4SocketFactory` would be needed.

### POSIX sockets

Emscripten has [support for POSIX sockets][ems_networking], but only by proxying
them through another host.

Besides `BuiltInWebSocket`, which can be replaced by an implementation that uses
another API available on the platform, `LiteCoreREST` uses POSIX sockets.

### File system

CBL enables WAL journaling for SQLite, which requires support for memory mapped
files and file locking.

By default Emscripten uses a in-memory file system.

For the browser an
[`IndexedDB` based file system](https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-idbfs)
is available. It has the downside that files are fully loaded into memory on
first access and need to be manually flushed to persistent storage. As far as I
can tell, it does not support file locking, so opening multiple `Database`
instances is not save.

For NodeJS a
[backend](https://emscripten.org/docs/api_reference/Filesystem-API.html#noderawfs)
is available which uses the NodeJS `fs` API. This backend is used in tests.

Emscripten is currently in the process of implementing a
[new files system layer](https://emscripten.org/docs/api_reference/Filesystem-API.html#new-file-system-wasmfs)
that will likely support file locking and a range of options for storage
backends.

This [issue](https://github.com/emscripten-core/emscripten/issues/15070)
discusses file locking in the new file system layer.

## Exposing the CBL API

[CBL_JSAPI.cc](src/CBL_JSAPI.cc) and [CBL_JSAPI.js](src/CBL_JSAPI.js) partially
expose a low level API to JavaScript that is very close the the C API.

The tests in [test/js](test/js) make use of this API.

[cmake]: https://cmake.org/install/
[nodejs]: https://nodejs.org/
[ems sdk]: https://emscripten.org/docs/getting_started/downloads.html
[ems_networking]: https://emscripten.org/docs/porting/networking.html
[ems_settings]: https://emsettings.surma.technology/
[ems_wasm_exceptions]:
  https://emscripten.org/docs/porting/exceptions.html#webassembly-exception-handling-proposal
[ems_pthreads]: https://emscripten.org/docs/porting/pthreads.html
[ems_callstack]:
  https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_get_callstack
[ems_demangle_support]: https://emsettings.surma.technology/#DEMANGLE_SUPPORT
[wasm_roadmap]: https://webassembly.org/roadmap/
