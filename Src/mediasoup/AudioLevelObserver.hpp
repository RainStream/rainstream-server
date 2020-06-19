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
	maxEntries?: uint32_t;

	/**
	 * Minimum average volume (in dBvo from -127 to 0) for entries in the
	 * "volumes" event.	Default -80.
	 */
	threshold?: uint32_t;

	/**
	 * Interval in ms for checking audio volumes. Default 1000.
	 */
	interval?: uint32_t;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

struct AudioLevelObserverVolume
{
	/**
	 * The audio producer instance.
	 */
	producer: Producer;

	/**
	 * The average volume (in dBvo from -127 to 0) of the audio producer in the
	 * last interval.
	 */
	volume: uint32_t;
}

const Logger* logger = new Logger("AudioLevelObserver");

class AudioLevelObserver : public RtpObserver
{
	/**
	 * @private
	 * @emits volumes - (volumes: AudioLevelObserverVolume[])
	 * @emits silence
	 */
	AudioLevelObserver(json params: any)
		: RtpObserver(params)
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
		this->_channel->on(this->_internal.value("rtpObserverId",""), (event: string, data?: any) =>
		{
			switch (event)
			{
				case "volumes":
				{
					// Get the corresponding Producer instance and remove entries with
					// no Producer (it may have been closed in the meanwhile).
					const volumes: AudioLevelObserverVolume[] = data
						.map(({ producerId, volume }: { producerId: string; volume: uint32_t }) => (
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
						this->_observer.safeEmit("volumes", volumes);
					}

					break;
				}

				case "silence":
				{
					this->safeEmit("silence");

					// Emit observer event.
					this->_observer.safeEmit("silence");

					break;
				}

				default:
				{
					logger->error("ignoring unknown event \"%s\"", event);
				}
			}
		});
	}
}
