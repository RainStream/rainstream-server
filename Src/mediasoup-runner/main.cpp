#define MS_CLASS "mediasoup-worker"
// #define MS_LOG_DEV_LEVEL 3

#include "MediaSoupErrors.hpp"
#include "lib.hpp"
#include <cstdlib> // std::_Exit(), std::genenv()
#include <string>

#include <uwebsockets/App.h>
#include <uwebsockets/WebSocket.h>


#define MEDIASOUP_VERSION "3.10.6"

class ChannelNative
{
public:
	static ChannelReadFreeFn channelReadFn(uint8_t** message,
		uint32_t* messageLen,
		size_t* messageCtx,
		// This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is more
		// data to read.
		const void* handle,
		ChannelReadCtx ctx)
	{

		ChannelNative* pThis = (ChannelNative*)ctx;

		//return channelReadFreeFn;
		return pThis->ProduceMessage(message, messageLen, messageCtx, handle) ? nullptr : nullptr;
	}

	static void channelReadFreeFn(uint8_t*, uint32_t, size_t)
	{

	}

	static void channelWriteFn(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx /* ctx */)
	{
		std::string strMsg((const char*)message, messageLen);

		printf("channelWriteFn:%s\n", strMsg.c_str());
	}

	bool ProduceMessage(uint8_t** message, uint32_t* messageLen, size_t* messageCtx, const void* handle)
	{
		
		this->_handle = reinterpret_cast<uv_async_t*>(const_cast<void*>(handle));
		return false;
	}

	void sendMessage(std::string msg)
	{
		if (_handle)
		{
			uv_async_send(_handle);
		}
	}

protected:
	uv_async_t* _handle;
};

class PayloadChannelNative
{
public:
	static PayloadChannelReadFreeFn payloadChannelReadFreeFn(
		uint8_t** message,
		uint32_t* messageLen,
		size_t* messageCtx,
		uint8_t** payload,
		uint32_t* payloadLen,
		size_t* payloadCapacity,
		// This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is more
		// data to read.
		const void* handle,
		PayloadChannelReadCtx ctx)
	{

		//uv_async_send((uv_async_t*)handle);
		//return payloadChannelReadFreeFn;
		return nullptr;
	}

	static void payloadChannelReadFreeFn(uint8_t*, uint32_t, size_t)
	{
		
	}

	static void payloadChannelWriteFn(
		const uint8_t* message,
		uint32_t messageLen,
		const uint8_t* payload,
		uint32_t payloadLen,
		ChannelWriteCtx ctx)
	{
		std::string strMsg((const char*)message, messageLen);
		std::string strPayload((const char*)payload, payloadLen);

		printf("%s---->%s\n", strMsg.c_str(), strPayload.c_str());
	}
};

//const int SSL = 1;

int main(int argc, char* argv[])
{
	/* ws->getUserData returns one of these */
	struct PerSocketData {
		/* Define your user data */
		int something;
	};

	/* Keep in mind that uWS::SSLApp({options}) is the same as uWS::App() when compiled without SSL support.
	 * You may swap to using uWS:App() if you don't need SSL */
	uWS::SSLApp({
		/* There are example certificates in uWebSockets.js repo */
		.key_file_name = "certs/privkey.pem",
		.cert_file_name = "certs/fullchain.pem"
		}).ws<PerSocketData>("/*", {
			/* Settings */
			.compression = uWS::SHARED_COMPRESSOR,
			.maxPayloadLength = 16 * 1024,
			.idleTimeout = 10,
			.maxBackpressure = 1 * 1024 * 1024,
			/* Handlers */
			.upgrade = [](auto* res, auto* req, auto* context) {

				/* HttpRequest (req) is only valid in this very callback, so we must COPY the headers
				 * we need later on while upgrading to WebSocket. You must not access req after first return.
				 * Here we create a heap allocated struct holding everything we will need later on. */

				struct UpgradeData {
					std::string secWebSocketKey;
					std::string secWebSocketProtocol;
					std::string secWebSocketExtensions;
					struct us_socket_context_t* context;
					decltype(res) httpRes;
					bool aborted = false;
				} *upgradeData = new UpgradeData {
					std::string(req->getHeader("sec-websocket-key")),
					std::string(req->getHeader("sec-websocket-protocol")),
					std::string(req->getHeader("sec-websocket-extensions")),
					context,
					res
				};

				/* We have to attach an abort handler for us to be aware
				 * of disconnections while we perform async tasks */
				res->onAborted([=]() {
					/* We don't implement any kind of cancellation here,
					 * so simply flag us as aborted */
					upgradeData->aborted = true;
					std::cout << "HTTP socket was closed before we upgraded it!" << std::endl;
				});

				/* Simulate checking auth for 5 seconds. This looks like crap, never write
				 * code that utilize us_timer_t like this; they are high-cost and should
				 * not be created and destroyed more than rarely!
				 *
				 * Also note that the code would be a lot simpler with capturing lambdas, maybe your
				 * database client has such a nice interface? Either way, here we go!*/
				struct us_loop_t* loop = (struct us_loop_t*)uWS::Loop::get();
				struct us_timer_t* delayTimer = us_create_timer(loop, 0, sizeof(UpgradeData*));
				memcpy(us_timer_ext(delayTimer), &upgradeData, sizeof(UpgradeData*));
				us_timer_set(delayTimer, [](struct us_timer_t* t) {
					/* We wrote the upgradeData pointer to the timer's extension */
					UpgradeData* upgradeData;
					memcpy(&upgradeData, us_timer_ext(t), sizeof(UpgradeData*));

					/* Were'nt we aborted before our async task finished? Okay, upgrade then! */
					if (!upgradeData->aborted) {
						std::cout << "Async task done, upgrading to WebSocket now!" << std::endl;

						/* If you don't want to upgrade you can instead respond with custom HTTP here,
						* such as res->writeStatus(...)->writeHeader(...)->end(...); or similar.*/

						/* This call will immediately emit .open event */
						upgradeData->httpRes->template upgrade<PerSocketData>({
							/* We initialize PerSocketData struct here */
							.something = 13
						}, upgradeData->secWebSocketKey,
							upgradeData->secWebSocketProtocol,
							upgradeData->secWebSocketExtensions,
							upgradeData->context);
					}
					else {
						std::cout << "Async task done, but the HTTP socket was closed. Skipping upgrade to WebSocket!" << std::endl;
					}

					delete upgradeData;

					us_timer_close(t);
					}, 5000, 0);

				},
				.open = [](auto* ws) {
					/* Open event here, you may access ws->getUserData() which points to a PerSocketData struct.
					 * Here we simply validate that indeed, something == 13 as set in upgrade handler. */
					std::cout << "Something is: " << static_cast<PerSocketData*>(ws->getUserData())->something << std::endl;
				},
				.message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
					/* We simply echo whatever data we get */
					ws->send(message, opCode);
				},
				.drain = [](auto*/*ws*/) {
					/* Check ws->getBufferedAmount() here */
				},
				.ping = [](auto*/*ws*/, std::string_view) {
					/* You don't need to handle this one, we automatically respond to pings as per standard */
				},
				.pong = [](auto*/*ws*/, std::string_view) {
					/* You don't need to handle this one either */
				},
				.close = [](auto*/*ws*/, int /*code*/, std::string_view /*message*/) {
					/* You may access ws->getUserData() here, but sending or
					 * doing any kind of I/O with the socket is not valid. */
				}
			}).listen(9001, [](auto* listen_socket) {
					if (listen_socket) {
						std::cout << "Listening on port " << 9001 << std::endl;
					}
				}).run();

	std::string version = MEDIASOUP_VERSION;

	ChannelNative* channelNative = new ChannelNative;
	PayloadChannelNative* payloadChannelNative = new PayloadChannelNative;

	std::thread t([=]() {
		auto statusCode = mediasoup_worker_run(
			argc,
			argv,
			version.c_str(),
			0,
			0,
			0,
			0,
			ChannelNative::channelReadFn,
			channelNative,
			ChannelNative::channelWriteFn,
			channelNative,
			PayloadChannelNative::payloadChannelReadFreeFn,
			payloadChannelNative,
			PayloadChannelNative::payloadChannelWriteFn,
			payloadChannelNative);

		switch (statusCode)
		{
		case 0:
			std::_Exit(EXIT_SUCCESS);
		case 1:
			std::_Exit(EXIT_FAILURE);
		case 42:
			std::_Exit(42);
		}

		});

	t.join();
}
