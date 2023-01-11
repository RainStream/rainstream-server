#define MSC_CLASS "ChannelNative"

#include "common.h"
#include "errors.h"
#include "ChannelNative.h"


namespace mediasoup {
const int PayloadMaxLen = 1024 * 1024 * 4;
const int MessageMaxLen = PayloadMaxLen + sizeof(int);

ChannelNative::ChannelNative()
	: Channel()
{
}

void ChannelNative::subClose()
{

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
	uint32_t id = messageCtx;

	/*if (_releaseMessageQueue)
	{

	}*/
}

void ChannelNative::channelWriteFn(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx ctx)
{
	ChannelNative* pThis = (ChannelNative*)ctx;

	std::string strPayload((const char*)message, messageLen);

	pThis->_processMessage(json::parse(strPayload));
}

bool ChannelNative::ProduceMessage(uint8_t** message, uint32_t* messageLen, size_t* messageCtx, const void* handle)
{
	this->_handle = reinterpret_cast<uv_async_t*>(const_cast<void*>(handle));

	if (_requestMessageQueue.empty())
	{
		return false;
	}
	
	RequestMessage* data = _requestMessageQueue.front();

	*message = (uint8_t*)data->request.data();
	*messageLen = data->request.size();
	*messageCtx = data->id;

	_requestMessageQueue.pop();

	_releaseMessageQueue.insert(std::pair(data->id, data));

	return true;
}

void ChannelNative::SendRequestMessage(uint32_t id)
{
	if (!_handle)
	{
		return;
	}

	try
	{
		if (uv_async_send(_handle) != 0)
		{
			this->_sents[id].set_exception(
				std::make_exception_ptr(Error(" SendRequestMessage uv_async_send error")));
		}
	}
	catch (const std::exception& ex)
	{
		this->_sents[id].set_exception(
			std::make_exception_ptr(ex));
	}
}

std::future<json> ChannelNative::request(std::string method, std::optional<std::string> handlerId, const json& data/* = json()*/)
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

	std::promise<json> t_promise;

	this->_sents.insert(std::make_pair(id, std::move(t_promise)));

	SendRequestMessage(id);

	return this->_sents[id].get_future();
}
}
