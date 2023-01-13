#define MSC_CLASS "Channel"

#include "common.h"
#include "Channel.h"
#include "Logger.h"
#include "utils.h"
#include "errors.h"


namespace mediasoup {


Channel::Channel()
{
	MSC_DEBUG("constructor()");
}

void Channel::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	// Close every pending sent.
	for (auto& [key, sent] : this->_sents)
	{
		sent.setException(std::make_exception_ptr(Error("Channel closed")));
	}

	this->subClose();
}

void Channel::_processMessage(const json& msg)
{
	// If a Response, retrieve its associated Request.
	if (msg.count("id"))
	{
		uint32_t id = msg["id"].get<uint32_t>();

		if (!this->_sents.count(id))
		{
			MSC_ERROR("received response does not match any sent request [id:%d]", id);
			return;
		}

		async_simple::Promise<json> sent = std::move(this->_sents[id]);
		this->_sents.erase(id);

		if (msg.count("accepted") && msg["accepted"].get<bool>())
		{
			json data = msg.value("data", json::object());
			MSC_DEBUG("request succeeded [id:%d]", id);
			sent.setValue(std::move(data));
		}
		else if (msg.count("error"))
		{
			std::string error = msg["error"];
			std::string reason = msg["reason"];

			MSC_WARN("request failed [id:\"%d\"] reason:%s]", id, reason.c_str());

			if (error == "TypeError")
			{
				sent.setException(std::make_exception_ptr(TypeError(reason)));
			}
			else
			{
				sent.setException(std::make_exception_ptr(Error(reason)));
			}
		}
		else
		{
			MSC_ERROR("received response is not accepted nor rejected[method:, id:%d]",  id);
		}
	}
	// If a Notification emit it to the corresponding entity.
	else if (msg.count("targetId") && msg.count("event"))
	{
		std::string targetId;
		if (msg["targetId"].is_number())
		{
			targetId = std::to_string(msg["targetId"].get<int32_t>());
		}
		else if (msg["targetId"].is_string())
		{
			targetId = msg["targetId"].get<std::string>();
		}

		setImmediate([=]() {
			this->emit(targetId, msg["event"].get<std::string>(), msg.value("data", json()));
		});
	}
	// Otherwise unexpected message.
	else
	{
		MSC_ERROR("received message is not a Response nor a Notification");
	}
}

}
