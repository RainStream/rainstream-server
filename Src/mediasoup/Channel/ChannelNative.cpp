#define MSC_CLASS "ChannelNative"

#include "common.h"
#include "errors.h"
#include "ChannelNative.h"


namespace mediasoup {

const int PayloadMaxLen = 1024 * 1024 * 4;
const int MessageMaxLen = PayloadMaxLen + sizeof(int);

inline static void onAsync(uv_handle_t* handle)
{
	while (static_cast<ChannelNative*>(handle->data)->CallbackWrite())
	{
		// Read while there are new messages.
	}
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

ChannelNative::ChannelNative()
	: Channel()
{
	this->_uvWriteHandle = new uv_async_t;
	this->_uvWriteHandle->data = static_cast<void*>(this);

	int err =
		uv_async_init(uv_default_loop(), this->_uvWriteHandle, reinterpret_cast<uv_async_cb>(onAsync));

	if (err != 0)
	{
		delete this->_uvWriteHandle;
		this->_uvWriteHandle = nullptr;

		MSC_THROW_ERROR("uv_async_init() failed: %s", uv_strerror(err));
	}

	/*err = uv_async_send(this->_uvWriteHandle);

	if (err != 0)
	{
		delete this->_uvWriteHandle;
		this->_uvWriteHandle = nullptr;

		MSC_THROW_ERROR("uv_async_send() failed: %s", uv_strerror(err));
	}*/
}

void ChannelNative::subClose()
{
	if (this->_uvWriteHandle)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->_uvWriteHandle), static_cast<uv_close_cb>(onClose));
	}
}

ChannelReadFreeFn ChannelNative::channelReadFn(uint8_t** message,
	uint32_t* messageLen,
	size_t* messageCtx,
	// This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is more
	// data to read.
	const void* handle,
	ChannelReadCtx ctx)
{

	ChannelNative* pThis = (ChannelNative*)ctx;

	//return channelReadFreeFn;
	return pThis->ProduceMessage(message, messageLen, messageCtx, handle) ? channelReadFreeFn : nullptr;
}

void ChannelNative::channelReadFreeFn(uint8_t* message, uint32_t messageLen, size_t messageCtx)
{
	RequestMessage* data = reinterpret_cast<RequestMessage*>(messageCtx);

	delete data;
}

void ChannelNative::channelWriteFn(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx ctx)
{
	ChannelNative* pThis = (ChannelNative*)ctx;

	std::string strPayload((const char*)message, messageLen);

	pThis->ReceiveMessage(strPayload);
}

bool ChannelNative::ProduceMessage(uint8_t** message, uint32_t* messageLen, size_t* messageCtx, const void* handle)
{
	this->_uvReadHandle = reinterpret_cast<uv_async_t*>(const_cast<void*>(handle));

	if (_requestMessageQueue.empty())
	{
		return false;
	}
	
	RequestMessage* data = _requestMessageQueue.front();

	*message = (uint8_t*)data->request.data();
	*messageLen = data->request.size();
	*messageCtx = (size_t)data;

	_requestMessageQueue.pop();

	return true;
}

void ChannelNative::SendRequestMessage(uint32_t id)
{
	if (!_uvReadHandle)
	{
		return;
	}

	try
	{
		if (uv_async_send(_uvReadHandle) != 0)
		{
			this->_sents[id].setException(
				std::make_exception_ptr(Error(" SendRequestMessage uv_async_send error")));
		}
	}
	catch (const std::exception& ex)
	{
		this->_sents[id].setException(
			std::make_exception_ptr(ex));
	}
}

void ChannelNative::ReceiveMessage(const std::string& message)
{
	_receiveMessageQueue.push(message);

	int err = uv_async_send(this->_uvWriteHandle);

	if (err != 0)
	{
		delete this->_uvWriteHandle;
		this->_uvWriteHandle = nullptr;

		MSC_THROW_ERROR("uv_async_send() failed: %s", uv_strerror(err));
	}
}

bool ChannelNative::CallbackWrite()
{
	if (this->_closed)
		return false;

	if (!_receiveMessageQueue.size())
	{
		return false;
	}

	std::string strPayload = _receiveMessageQueue.front();

	_processMessage(json::parse(strPayload));

	_receiveMessageQueue.pop();

	return true;
}

async_simple::coro::Lazy<json> ChannelNative::request(std::string method, std::optional<std::string> handlerId, const json& data/* = json()*/)
{
	constexpr auto max_value = std::numeric_limits<uint32_t>::max(); //4294967295

	this->_nextId < max_value ? ++this->_nextId : (this->_nextId = 1);

	uint32_t id = this->_nextId;

	MSC_DEBUG("request() [method \"%s\", id: \"%d\"]", method.c_str(), id);

	if (!handlerId.has_value())
	{
		handlerId = "undefined";
	}

	std::string payload = data.is_null() ? "undefined" : data.dump();

	std::string request = Utils::Printf("%u:%s:%s:%s", id, method.c_str(), handlerId.value().c_str(), payload.c_str());

	if (request.length() > MessageMaxLen)
		MSC_THROW_ERROR("Channel request too big");

	_requestMessageQueue.push(new RequestMessage{ id, request });

	async_simple::Promise<json> t_promise;

	this->_sents.insert(std::make_pair(id, std::move(t_promise)));

	SendRequestMessage(id);

	auto value = co_await this->_sents[id].getFuture();

	co_return value;
}
}
