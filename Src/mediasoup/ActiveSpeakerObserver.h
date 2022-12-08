#pragma once

#include "RtpObserver.h"

namespace mediasoup {

class Producer;

struct ActiveSpeakerObserverOptions
{
	int interval;

	/**
	 * Custom application data.
	 */
	json appData;
};

struct ActiveSpeakerObserverActivity
{
	/**
	 * The producer instance.
	 */
	Producer* producer;
};
//
//export type ActiveSpeakerObserverEvents = RtpObserverEvents&
//{
//	dominantspeaker: [{ producer: Producer }] ;
//};
//
//export type ActiveSpeakerObserverObserverEvents = RtpObserverObserverEvents&
//{
//	dominantspeaker: [{ producer: Producer }] ;
//};
//
//type RtpObserverObserverConstructorOptions = RtpObserverConstructorOptions;


class ActiveSpeakerObserver : public RtpObserver
{
public:
	/**
	 * @private
	 */
	ActiveSpeakerObserver(json internal,
		Channel* channel,
		PayloadChannel* payloadChannel,
		json appData,
		GetProducerById getProducerById);

	/**
	 * Observer.
	 */
	EnhancedEventEmitter* observer();

private:
	void handleWorkerNotifications();
};

}
