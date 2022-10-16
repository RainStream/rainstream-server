#define MSC_CLASS "ActiveSpeakerObserver"

#include "common.hpp"
#include "Logger.hpp"
#include "Channel.hpp"
#include "ActiveSpeakerObserver.hpp"


ActiveSpeakerObserver::ActiveSpeakerObserver(json internal,
	Channel* channel,
	PayloadChannel* payloadChannel,
	json appData,
	GetProducerById getProducerById)
	: RtpObserver(internal, channel, payloadChannel, appData, getProducerById)
{
	this->handleWorkerNotifications();
}

EnhancedEventEmitter* ActiveSpeakerObserver::observer()
{
	return this->_observer;
}

void ActiveSpeakerObserver::handleWorkerNotifications()
{
	this->_channel->on(this->_internal["rtpObserverId"], [=](std::string event, const json& data)
		{

			if (event == "dominantspeaker")
			{
				Producer* producer = this->_getProducerById(data["producerId"]);

				if (!producer)
					return;

				std::vector<Producer*> dominantSpeaker = { producer };

				this->safeEmit("dominantspeaker", dominantSpeaker);
				this->_observer->safeEmit("dominantspeaker", dominantSpeaker);
			}
			else
			{
				MSC_ERROR("ignoring unknown event \"%s\"", event.c_str());
			}
		});
}
