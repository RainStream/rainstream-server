#define MSC_CLASS "ChannelNative"

#include "common.h"
#include "ChannelNative.h"

namespace mediasoup {
	ChannelNative::ChannelNative()
		: Channel()
	{
	}

	void ChannelNative::subClose()
	{

	}

	std::future<json> ChannelNative::request(std::string method, std::optional<std::string> handlerId, const json& data/* = json()*/)
	{
		this->_nextId < 4294967295 ? ++this->_nextId : (this->_nextId = 1);

		uint32_t id = this->_nextId;

		std::promise<json> t_promise;

		this->_sents.insert(std::make_pair(id, std::move(t_promise)));

		return this->_sents[id].get_future();
	}
}
