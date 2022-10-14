#define MSC_CLASS "AudioLevelObserver"

#include "common.hpp"
#include "Logger.hpp"
#include "Channel.hpp"
#include "AudioLevelObserver.hpp"


AudioLevelObserver::AudioLevelObserver(json internal,
	Channel* channel,
	PayloadChannel* payloadChannel,
	json appData,
	GetProducerById getProducerById)
	: RtpObserver(internal, channel, payloadChannel, appData, getProducerById)
{
	this->_handleWorkerNotifications();
}

EnhancedEventEmitter* AudioLevelObserver::observer()
{
	return this->_observer;
}

void AudioLevelObserver::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["rtpObserverId"], [=](std::string event, const json& data)
		{
			if (event == "volumes")
			{
				/*
				// Get the corresponding Producer instance and remove entries with
				// no Producer (it may have been closed in the meanwhile).
				const volumes: AudioLevelObserverVolume[] = data
					.map(({ producerId, volume }: { producerId; volume }) => (
						{
							producer : this->_getProducerById(producerId),
							volume
						}
					))
					.filter(({ producer }: { producer: Producer }) => producer);

				if (volumes.length > 0)
				{
					this->safeEmit("volumes", volumes);

					// Emit observer event.
					this->_observer->safeEmit("volumes", volumes);
				}
				*/
			}
			else if (event == "silence")
			{
				this->safeEmit("silence");

				// Emit observer event.
				this->_observer->safeEmit("silence");
			}
			else
			{
				MSC_ERROR("ignoring unknown event \"%s\"", event);
			}
		});
}
