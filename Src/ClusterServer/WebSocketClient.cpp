#define MSC_CLASS "WebSocketClient"

#include "WebSocketClient.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <errors.hpp>
#include <uwebsockets/App.h>
#include <uwebsockets/WebSocket.h>

namespace protoo {

inline static void onAsyncWrite(uv_handle_t* handle)
{
	static_cast<WebSocketClient*>(handle->data)->OnUvWrite();
}

inline static void onWriteClose(uv_handle_t* handle)
{
	
}

WebSocketClient::WebSocketClient(std::string url)
	: _url(url)
{
	this->uvWriteHandle = new uv_async_t{ 0 };
	this->uvWriteHandle->data = (void*)this;

	int err;

	uv_loop_t* uv_loop = uv_default_loop();

	err = uv_async_init(uv_loop, this->uvWriteHandle, reinterpret_cast<uv_async_cb>(onAsyncWrite));
	if (err != 0)
	{
		delete this->uvWriteHandle;
		this->uvWriteHandle = nullptr;

		MSC_WARN("uv_async_init() failed: %s", uv_strerror(err));
	}
}

WebSocketClient::~WebSocketClient()
{

}

void WebSocketClient::Close(int code, std::string message)
{
	if (_closed)
	{
		return;
	}

	_closed = true;

	if (userData)
	{
		auto ws = static_cast <uWS::WebSocket<true, true, PeerSocketData>*>(userData);
		ws->end(code, message.c_str());
	}

	if (this->uvWriteHandle)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvWriteHandle), static_cast<uv_close_cb>(onWriteClose));
	}
}

void WebSocketClient::send(const json& data)
{
	if (this->_closed)
		MSC_THROW_ERROR("transport closed");

	std::lock_guard<std::mutex> guard(write_mutex);

	_buffers.push_back(data.dump());

	uv_async_send(this->uvWriteHandle);
}

bool WebSocketClient::closed()
{
	return this->_closed;
}

void WebSocketClient::setListener(Listener* listener)
{
	_listener = listener;
}

std::string WebSocketClient::url() const
{
	return _url;
}

std::string WebSocketClient::addresss() const
{
	return _address;
}

void WebSocketClient::setUserData(void* userData)
{
	this->userData = userData;

	auto ws = static_cast <uWS::WebSocket<true, true, PeerSocketData>*>(userData);
	_address = std::string(ws->getRemoteAddress());

	//LOG(INFO) << "WebSocketClient connected with [IP:" << _address << "]";
}

void WebSocketClient::onMessage(const std::string& message)
{
	if (_listener)
	{
		_listener->onMessage(message);
	}
}

void WebSocketClient::onClosed(int code, const std::string& message)
{
	if (this->_listener)
	{
		_listener->onClosed(code, message);
	}
}

inline void WebSocketClient::OnUvWrite()
{
	std::lock_guard<std::mutex> guard(write_mutex);

	for (const std::string& buffer : _buffers)
	{
		try
		{
			auto ws = static_cast<uWS::WebSocket<true, true, PeerSocketData>*>(userData);
			ws->send(buffer, uWS::OpCode::TEXT);
		}
		catch (std::exception& error)
		{
			MSC_ERROR("send() failed:%s", error.what());

			throw error;
		}
	}

	_buffers.clear();
}

}