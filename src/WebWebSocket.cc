#include "WebWebSocket.hh"
#include "c4Replicator.hh"
#include "emscripten/websocket.h"
#include "fleece/Expert.hh"
#include <thread>

using namespace fleece;

void registerWebWebSocket() {
    C4Socket::registerFactory(cbl_internal::WebWebSocketFactory());
}

namespace cbl_internal {
    WebWebSocketFactory::WebWebSocketFactory() {
        // Framing is done by the WebSocket implementation.
        framing = kC4NoFraming;

        open = [](C4Socket *socket, const C4Address *addr, C4Slice options, void *context) {
            Dict optionsDict = ValueFromData(options).asDict();

            // Create WebSocket options.
            // Heartbeat is not supported by WebSocket.
            auto url = addr->toURL().asString();
            auto protocols = optionsDict[kC4SocketOptionWSProtocols].asString().asString();

            EmscriptenWebSocketCreateAttributes wsOptions;
            emscripten_websocket_init_create_attributes(&wsOptions);
            wsOptions.url = url.c_str();
            if (!protocols.empty()) {
                wsOptions.protocols = protocols.c_str();
            }

            // Create the WebSocket, which immediately starts connecting.
            auto ws = emscripten_websocket_new(&wsOptions);

            // Handle not being able to create the WebSocket.
            if (ws <= 0) {
                // The C4Socket must be closed asynchronously.
                auto thread = std::thread([=] {
                    auto error = C4Error::printf(LiteCoreDomain, kC4ErrorUnexpectedError,
                        "Unable to create WebSocket: %d", ws);
                    socket->closed(error);
                });
                thread.detach();

                socket->setNativeHandle(nullptr);
                return;
            }

            // Associate the context with the C4Socket.
            auto wsContext = new WebWebSocketContext;
            wsContext->webSocket = ws;
            wsContext->didOpen = false;
            wsContext->didClose = false;
            wsContext->requestedClose = false;
            socket->setNativeHandle((void *)wsContext);

            emscripten_websocket_set_onopen_callback(ws, socket, [](
                int eventType,
                const EmscriptenWebSocketOpenEvent *websocketEvent,
                void *userData
            ) {
                auto socket = (C4Socket *)userData;
                auto context = (WebWebSocketContext *)socket->getNativeHandle();
                {
                    std::scoped_lock<std::mutex> lock(context->mutex);
                    context->didOpen = true;
                }
                socket->opened();

                return 1;
            });

            emscripten_websocket_set_onclose_callback(ws, socket, [](
                int eventType,
                const EmscriptenWebSocketCloseEvent *websocketEvent,
                void *userData
            ) {
                auto socket = (C4Socket *)userData;
                auto context = (WebWebSocketContext *)socket->getNativeHandle();

                bool closeC4Socket = false;
                bool requestC4SocketClose = false;
                C4Error error{};

                {
                    std::scoped_lock<std::mutex> lock(context->mutex);
                    context->didClose = true;
                    if (context->requestedClose) {
                        if (context->didOpen) {
                            // The C4Socket initiated the closing of the WebSocket after it had
                            // opened.
                            closeC4Socket = true;
                        }
                    } else {
                        if (context->didOpen) {
                            // The other side closed the WebSocket.
                            requestC4SocketClose = true;
                        } else {
                            // The WebSocket has not opened yet, which means that opening failed.
                            error = C4Error::make(
                                WebSocketDomain,
                                websocketEvent->code,
                                websocketEvent->reason
                            );
                            closeC4Socket = true;
                        }
                    }
                }

                if (requestC4SocketClose) {
                    socket->closeRequested(websocketEvent->code, slice(websocketEvent->reason));
                } else if (closeC4Socket) {
                    socket->closed(error);
                }

                return 1;
            });

            emscripten_websocket_set_onmessage_callback(ws, socket, [](
                int eventType,
                const EmscriptenWebSocketMessageEvent *websocketEvent,
                void *userData
            ) {
                auto socket = (C4Socket *)userData;
                socket->received(slice(websocketEvent->data, websocketEvent->numBytes));

                return 1;
            });
        };

        write = [](C4Socket *socket, C4SliceResult allocatedData) {
            auto context = (WebWebSocketContext *)socket->getNativeHandle();
            emscripten_websocket_send_binary(
                context->webSocket,
                (void*)allocatedData.buf,
                (uint32_t)allocatedData.size
            );

            // TODO: Use bufferedAmount to delay the write acknowledgement if necessary.
            socket->completedWrite(allocatedData.size);
        };

        completedReceive = [](C4Socket *socket, size_t byteCount) {
            // There is nothing we can do to throttle the WebSocket.
        };

        // We don't need to implement this because this implementation is operating at the
        // message level.
        close = nullptr;

        requestClose = [](C4Socket *socket, int status, C4String message) {
            auto context = (WebWebSocketContext *)socket->getNativeHandle();
            if (!context) {
                // The WebSocket could not be created and this C4Socket will be closed
                // asynchronously from the open handler.
                return;
            }

            bool closeC4Socket = false;
            bool closeWebSocket = false;

            {
                std::scoped_lock<std::mutex> lock(context->mutex);
                context->requestedClose = true;

                if (context->didClose) {
                    // The C4Socket is acknowledging that the peer closed the WebSocket
                    // so we can finally close the C4Socket.
                    closeC4Socket = true;
                } else {
                    if (context->didOpen) {
                        // Close the connected WebSocket.
                        closeWebSocket = true;
                        // TODO: Timeout closing the WebSocket.
                    } else {
                        // The WebSocket has not opened yet.
                        // Cancel the WebSocket connection attempt.
                        closeWebSocket = true;
                        // Directly close the C4Socket.
                        closeC4Socket = true;
                    }
                }
            }

            if (closeWebSocket) {
                auto messageStr = slice(message).asString();
                emscripten_websocket_close(context->webSocket, status, messageStr.c_str());
            }
            if (closeC4Socket) {
                C4Error error {};
                socket->closed(error);
            }
        };

        dispose = [](C4Socket *socket) {
            auto context = (WebWebSocketContext *)socket->getNativeHandle();
            if (context) {
                delete context;
            }
        };
    }

    WebWebSocketContext::~WebWebSocketContext() {
        emscripten_websocket_delete(webSocket);
    }
}
