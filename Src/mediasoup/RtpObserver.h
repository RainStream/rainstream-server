#pragma once 

#include "EnhancedEventEmitter.h"

class Channel;
class Producer;
class PayloadChannel;

using GetProducerById = std::function<Producer*(std::string)>;

class RtpObserver : public EnhancedEventEmitter
{
public:
	/**
	 * @private
	 * @interface
	 */
	RtpObserver(
		json internal,
		Channel* channel,
		PayloadChannel* payloadChannel,
		json appData,
		GetProducerById getProducerById
	);

	virtual ~RtpObserver();

	/**
	 * RtpObserver id.
	 */
	std::string id();
	/**
	 * Whether the RtpObserver is closed.
	 */
	bool closed();
	/**
	 * Whether the RtpObserver is paused.
	 */
	bool paused();
	/**
	 * App custom data.
	 */
	json appData();
	/**
	 * Invalid setter.
	 */
	void appData(json appData);
	/**
	 * Observer.
	 */
	EnhancedEventEmitter* observer();
	/**
	 * Close the RtpObserver.
	 */
	void close();
	/**
	 * Router was closed.
	 *
	 * @private
	 */
	void routerClosed();
	/**
	 * Pause the RtpObserver.
	 */
	std::future<void> pause();
	/**
	 * Resume the RtpObserver.
	 */
	std::future<void> resume();
	/**
	 * Add a Producer to the RtpObserver.
	 */
	std::future<void> addProducer(std::string producerId);
	/**
	 * Remove a Producer from the RtpObserver.
	 */
	std::future<void> removeProducer(std::string producerId);

protected:
	// Internal data.
	json _internal;
	// Channel instance.
	Channel* _channel;
	// PayloadChannel instance.
	PayloadChannel* _payloadChannel;
	// Closed flag.
	bool _closed = false;
	// Paused flag.
	bool _paused = false;
	// Custom app data.
	json _appData;
	// Method to retrieve a Producer.
	GetProducerById _getProducerById;
	// Observer instance.
	EnhancedEventEmitter* _observer{ nullptr };
};
