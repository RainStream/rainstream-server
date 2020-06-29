#ifndef PROTOO_PEER_HPP
#define PROTOO_PEER_HPP

#include "common.hpp"
#include "WebSocketClient.hpp"

namespace protoo
{
	class Request;
	class WebSocketClient;

	class Peer : public WebSocketClient::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnPeerClose(Peer* peer) = 0;
			virtual void OnPeerRequest(Peer* peer, json& request) = 0;
			virtual void OnPeerNotify(Peer* peer, json& notification) = 0;
		};
	public:
		explicit Peer(std::string peerName, protoo::WebSocketClient* transport, Listener* listener);
		virtual ~Peer();

	public:
		std::string id();
		void close();

		void Accept(uint32_t id, json& data);
		void Reject(uint32_t id, uint32_t code, const std::string& errorReason);

// 		Defer send(std::string method, json data);
// 		Defer notify(std::string method, json data);

	protected:
		virtual void onMessage(const std::string& message);
		virtual void onClosed(int code, const std::string& message);

	protected:
		void _handleTransport();
		void _handleRequest(json request);
		void _handleResponse(json response);
		void _handleNotification(json notification);

	private:
		std::string _peerName;

		Listener* listener{ nullptr };
		WebSocketClient* _transport{ nullptr };

		// Closed flag.
		bool _closed = false;

//		std::unordered_map<uint32_t, Defer> _requestHandlers;
		std::unordered_map<uint32_t, json> _requests;
	};
}

#endif

