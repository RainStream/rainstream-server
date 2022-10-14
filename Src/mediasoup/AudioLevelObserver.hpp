#pragma once 

#include "EnhancedEventEmitter.hpp"
#include "RtpObserver.hpp"


struct AudioLevelObserverOptions
{
	/**
	 * Maximum uint32_t of entries in the "volumes”" event. Default 1.
	 */
	uint32_t maxEntries = 1;

	/**
	 * Minimum average volume (in dBvo from -127 to 0) for entries in the
	 * "volumes" event.	Default -80.
	 */
	int32_t threshold = -80;

	/**
	 * Interval in ms for checking audio volumes. Default 1000.
	 */
	uint32_t interval = 1000;

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
public:
	/**
	 * @private
	 */
	AudioLevelObserver(json internal,
		Channel* channel,
		PayloadChannel* payloadChannel,
		json appData,
		GetProducerById getProducerById);

	/**
	 * Observer.
	 */
	EnhancedEventEmitter* observer();

private:
	void _handleWorkerNotifications();

};
