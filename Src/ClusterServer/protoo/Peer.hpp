#ifndef PROTOO_PEER_HPP
#define PROTOO_PEER_HPP

#include "common.hpp"
#include "protoo/WebSocketTransport.hpp"

namespace protoo
{
	class Request;
	class WebSocketTransport;

	class Peer : public WebSocketTransport::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnPeerClose(Peer* peer) = 0;
			virtual void OnPeerRequest(Peer* peer, Request* request) = 0;
			virtual void OnNotification(Peer* peer, Json notification) = 0;
		};
	public:
		explicit Peer(std::string peerName, protoo::WebSocketTransport* transport, Listener* listener);
		virtual ~Peer();

	public:
		std::string id();
		void close();
		rs::Peer* mediaPeer();
		void setMediaPeer(rs::Peer* peer);
		void Send(Json data);
		Defer send(std::string method, Json data);
		Defer notify(std::string method, Json data);

	protected:
		virtual void onMessage(const std::string& message);
		virtual void onDisconnection(int code, const std::string& message);

	protected:
		void _handleTransport();
		void _handleRequest(Json request);
		void _handleResponse(Json response);
		void _handleNotification(Json notification);

	private:
		std::string _peerName;
		rs::Peer* _peer{ nullptr };
		Listener* listener{ nullptr };
		WebSocketTransport* _transport{ nullptr };

		// Closed flag.
		bool _closed = false;

		std::unordered_map<uint32_t, Defer> _requestHandlers;
	};
}

#endif

