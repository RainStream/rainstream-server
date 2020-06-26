#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
#include "RtpObserver.hpp"
#include "Producer.hpp"

struct AudioLevelObserverOptions
{
	/**
	 * Maximum uint32_t of entries in the "volumes”" event. Default 1.
	 */
	uint32_t maxEntries;

	/**
	 * Minimum average volume (in dBvo from -127 to 0) for entries in the
	 * "volumes" event.	Default -80.
	 */
	uint32_t threshold;

	/**
	 * Interval in ms for checking audio volumes. Default 1000.
	 */
	uint32_t interval;

	/**
	 * Custom application data.
	 */
	json appData;
};

struct AudioLevelObserverVolume
{
	/**
	 * The audio producer instance.
	 */
	Producer* producer;

	/**
	 * The average volume (in dBvo from -127 to 0) of the audio producer in the
	 * last interval.
	 */
	uint32_t volume;
};

class AudioLevelObserver : public RtpObserver
{
	Logger* logger;
public:
	/**
	 * @private
	 * @emits volumes - (volumes: AudioLevelObserverVolume[])
	 * @emits silence
	 */
	AudioLevelObserver(json params /*= json()*/)
		: RtpObserver(params)
		, logger(new Logger("AudioLevelObserver"))
	{
		this->_handleWorkerNotifications();
	}

	/**
	 * Observer.
	 *
	 * @emits close
	 * @emits pause
	 * @emits resume
	 * @emits addproducer - (producer: Producer)
	 * @emits removeproducer - (producer: Producer)
	 * @emits volumes - (volumes: AudioLevelObserverVolume[])
	 * @emits silence
	 */
	EnhancedEventEmitter* observer()
	{
		return this->_observer;
	}

private:
	void _handleWorkerNotifications()
	{
		this->_channel->on(this->_internal.value("rtpObserverId",""), [=](std::string event, json data)
		{
		
			if(event == "volumes")
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
				logger->error("ignoring unknown event \"%s\"", event);
			}
			
		});
	}
};
