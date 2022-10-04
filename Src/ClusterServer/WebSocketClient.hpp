#ifndef WEBSOCKET_TRANSPORT_HPP
#define WEBSOCKET_TRANSPORT_HPP

#include "common.hpp"
#include "DepLibUV.hpp"

namespace protoo{

class WebSocketClient;

struct PeerSocketData {
	WebSocketClient* transport{ nullptr };
};

class WebSocketClient
{
	friend class WebSocketServer;
public:
	class Listener
	{
	public:
		virtual void onMessage(const std::string& message) = 0;
		virtual void onClosed(int code, const std::string& message) = 0;
	};
public:
	explicit WebSocketClient(std::string url);
	virtual ~WebSocketClient();

	void setListener(Listener* listener);
	std::string url() const;
	std::string addresss() const;

public:
	void Close(int code = 1000, std::string message = std::string());
	bool closed();
	void send(const json& data);
	void OnUvWrite();

protected:
	void setUserData(void* userData);
	void onMessage(const std::string& message);
	void onClosed(int code, const std::string& message);



private:
	void* userData{ nullptr };

	Listener* _listener{ nullptr };
	std::string _url;
	std::string _address;
	// Closed flag.
	bool _closed = false;

	uv_async_t* uvWriteHandle{ nullptr };

	std::mutex write_mutex;


	std::list<std::string> _buffers;

};
}

#endif

