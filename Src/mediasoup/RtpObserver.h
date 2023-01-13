#pragma once 

#include "EnhancedEventEmitter.h"

namespace mediasoup {

class Channel;
class Producer;
class PayloadChannel;

using GetProducerById = std::function<Producer*(std::string)>;

class MS_EXPORT RtpObserver : public EnhancedEventEmitter
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
	async_simple::coro::Lazy<void> pause();
	/**
	 * Resume the RtpObserver.
	 */
	async_simple::coro::Lazy<void> resume();
	/**
	 * Add a Producer to the RtpObserver.
	 */
	async_simple::coro::Lazy<void> addProducer(std::string producerId);
	/**
	 * Remove a Producer from the RtpObserver.
	 */
	async_simple::coro::Lazy<void> removeProducer(std::string producerId);

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

}
