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

	_app = new uWS::SSLApp({
		/* There are example certificates in uWebSockets.js repo */
		.key_file_name = key.c_str(),
		.cert_file_name = cert.c_str(),
		.passphrase = phrase.c_str()
		});

	_app->get("/*", [](auto* res, auto*/*req*/) {

		res->end("Hello world!");

	}).ws<PeerSocketData>("/*", {
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

			std::string query(req->getQuery());

			struct UpgradeData {
				std::string secWebSocketKey;
				std::string secWebSocketProtocol;
				std::string secWebSocketExtensions;
				struct us_socket_context_t* context;
				decltype(res) httpRes;
				bool aborted = false;
			} *upgradeData = new UpgradeData{
				std::string(req->getHeader("sec-websocket-key")),
				std::string(req->getHeader("sec-websocket-protocol")),
				std::string(req->getHeader("sec-websocket-extensions")),
				context,
				res
			};

			auto accept = [=]()->WebSocketClient* {

				WebSocketClient* transport = nullptr;
				if (!upgradeData->aborted) {
					transport = new WebSocketClient(query);

					/* This call will immediately emit .open event */
					upgradeData->httpRes->template upgrade<PeerSocketData>({
						/* We initialize PerSocketData struct here */
						.transport = transport
						}, upgradeData->secWebSocketKey,
						upgradeData->secWebSocketProtocol,
						upgradeData->secWebSocketExtensions,
						upgradeData->context);
				}

				delete upgradeData;

				return transport;
			};

			auto reject = [=](Error error) {
				if (!upgradeData->aborted) {
					res->end(error.ToString(), true);
				}
				
				MSC_ERROR("websocket client error for %s", error.ToString().c_str());

				delete upgradeData;
			};

			res->onAborted([=]() {
				/* We don't implement any kind of cancellation here,
				 * so simply flag us as aborted */
				upgradeData->aborted = true;
			});

			if (_lisenter) {
				_lisenter->OnConnectRequest(query, std::move(accept), std::move(reject));
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
