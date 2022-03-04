
#pragma once
#include "CBL_Compat.h"
#include "c4Socket.hh"
#include <mutex>

CBL_ASSUME_NONNULL_BEGIN

/**
 * Registers a C4SocketFactory that is implemented based on the WebSocket API.
 *
 * Note that WebSocket is a browser API and not available in other environments like NodeJS.
 *
 * The only configuration that is supported by the WebSocket API is the URL to connect to and
 * the supported protocols (`kC4SocketOptionWSProtocols` is used to determine them).
 */
void registerWebWebSocket();

namespace cbl_internal {
    struct WebWebSocketFactory final : public C4SocketFactory {
        WebWebSocketFactory();
    };

    struct WebWebSocketContext {
        std::mutex mutex;
        int webSocket;
        bool didOpen;           // Whether the WebSocket has been opened.
        bool didClose;          // Whether the WebSocket has has closed.
        bool requestedClose;    // Whether the C4Socket has requested to close.

        ~WebWebSocketContext();
    };
}

CBL_ASSUME_NONNULL_END
