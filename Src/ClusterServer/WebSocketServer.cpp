#define MSC_CLASS "WebSocketServer"

#include "WebSocketServer.hpp"
#include "WebSocketClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "Utility.hpp"


namespace protoo {

//const int SSL = 1;

#define SEC_WEBSOCKET_PROTOCOL "protoo"


WebSocketServer::WebSocketServer(json tls, Lisenter* lisenter)
	: _lisenter(lisenter)
	, _tls(tls)
{
	uv_loop_t* uv_loop = DepLibUV::GetLoop();
	uWS::Loop* loop = uWS::Loop::get(uv_loop);

	std::string cert, key, phrase;

	if (_tls.is_object()) {
		cert = _tls.value("cert", "");
		key = _tls.value("key", "");
		phrase = _tls.value("phrase", "");
	}

	_app = new uWS::App(/*{
		.key_file_name = key.c_str(),
		.cert_file_name = cert.c_str(),
		.passphrase = phrase.c_str()
	}*/);

	_app->ws<PeerSocketData>("/*", {
		/* Settings */
		.compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
		.maxPayloadLength = 100 * 1024 * 1024,
		.idleTimeout = 16,
		.maxBackpressure = 100 * 1024 * 1024,
		.closeOnBackpressureLimit = false,
		.resetIdleTimeoutOnSend = false,
		.sendPingsAutomatically = true,
		/* Handlers */
		.upgrade = [=](auto* res, auto* req, auto* context) {

			std::string roomId(req->getQuery("roomId"));
			std::string peerId(req->getQuery("peerId"));
			std::string protocol(req->getHeader("sec-websocket-protocol"));
			std::string query(req->getQuery());

			WebSocketClient* transport = new WebSocketClient(query);

			res->template upgrade<PeerSocketData>({
				.peerId = roomId,
				.roomId = peerId,
				.protocol = protocol,
				.transport = transport
			}, req->getHeader("sec-websocket-key"),
				req->getHeader("sec-websocket-protocol"),
				req->getHeader("sec-websocket-extensions"),
				context);

			if (_lisenter) {
				_lisenter->OnConnectRequest(transport);
			}
		},
		.open = [=](auto* ws) {
			PeerSocketData* peerData = ws->getUserData();
			WebSocketClient* transport = peerData->transport;

			if (transport) {
				transport->setUserData(ws);
			}
		},
		.message = [=](auto* ws, std::string_view message, uWS::OpCode opCode) {
			PeerSocketData* peerData = ws->getUserData();
			WebSocketClient* transport = peerData->transport;

			if (transport) {
				transport->onMessage(std::string(message));
			}
		},
		.close = [=](auto* ws, int code, std::string_view message) {
			PeerSocketData* peerData = ws->getUserData();
			WebSocketClient* transport = peerData->transport;

			if (transport) {
				if (_lisenter) {
					_lisenter->OnConnectClosed(transport);
				}

				transport->onClosed(code, std::string(message));
				delete transport;
			}
		}
	});
}

WebSocketServer::~WebSocketServer()
{
	delete _app;
}

bool WebSocketServer::Setup(const char* host, uint16_t port)
{
	_app->listen(port, [=](auto* listen_socket) {
		if (!listen_socket) {
			MSC_ABORT("websocket listening on port %d error", port);
		}
		});

	return true;
}

}
