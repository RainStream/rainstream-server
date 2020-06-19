#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
#include "Channel.hpp"
#include "PayloadChannel.hpp"
#include "Producer.hpp"

const Logger* logger = new Logger("RtpObserver");

class RtpObserver : public EnhancedEventEmitter
{
	// Internal data.
protected:
	json _internal:
	{
		routerId: string;
		rtpObserverId: string;
	};

	// Channel instance.
	Channel* _channel;

	// PayloadChannel instance.
	PayloadChannel* _payloadChannel;

	// Closed flag.
	bool _closed = false;

	// Paused flag.
	bool _paused = false;

	// Custom app data.
	private readonly _appData?: any;

	// Method to retrieve a Producer.
	protected readonly _getProducerById: (producerId: string) => Producer;

	// Observer instance.
	protected readonly _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @interface
	 * @emits routerclose
	 * @emits @close
	 */
	RtpObserver(
		{
			internal,
			channel,
			payloadChannel,
			appData,
			getProducerById
		}:
		{
			internal: any;
			channel: Channel;
			payloadChannel: PayloadChannel;
			json appData;
			getProducerById: (producerId: string) => Producer;
		}
	)
	{
		super();

		logger->debug("constructor()");

		this->_internal = internal;
		this->_channel = channel;
		this->_payloadChannel = payloadChannel;
		this->_appData = appData;
		this->_getProducerById = getProducerById;
	}

	/**
	 * RtpObserver id.
	 */
	get id(): string
	{
		return this->_internal.rtpObserverId;
	}

	/**
	 * Whether the RtpObserver is closed.
	 */
	bool closed()
	{
		return this->_closed;
	}

	/**
	 * Whether the RtpObserver is paused.
	 */
	bool paused()
	{
		return this->_paused;
	}

	/**
	 * App custom data.
	 */
	json appData()
	{
		return this->_appData;
	}

	/**
	 * Invalid setter.
	 */
	set appData(json appData) // eslint-disable-line no-unused-vars
	{
		throw new Error("cannot override appData object");
	}

	/**
	 * Observer.
	 *
	 * @emits close
	 * @emits pause
	 * @emits resume
	 * @emits addproducer - (producer: Producer)
	 * @emits removeproducer - (producer: Producer)
	 */
	get observer(): EnhancedEventEmitter
	{
		return this->_observer;
	}

	/**
	 * Close the RtpObserver.
	 */
	void close()
	{
		if (this->_closed)
			return;

		logger->debug("close()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal.rtpObserverId);

		this->_channel->request("rtpObserver.close", this->_internal)
			.catch(() => {});

		this->emit("@close");

		// Emit observer event.
		this->_observer.safeEmit("close");
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 */
	void routerClosed()
	{
		if (this->_closed)
			return;

		logger->debug("routerClosed()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal.rtpObserverId);

		this->safeEmit("routerclose");

		// Emit observer event.
		this->_observer.safeEmit("close");
	}

	/**
	 * Pause the RtpObserver.
	 */
	async pause(): Promise<void>
	{
		logger->debug("pause()");

		const wasPaused = this->_paused;

		co_await this->_channel->request("rtpObserver.pause", this->_internal);

		this->_paused = true;

		// Emit observer event.
		if (!wasPaused)
			this->_observer.safeEmit("pause");
	}

	/**
	 * Resume the RtpObserver.
	 */
	async resume(): Promise<void>
	{
		logger->debug("resume()");

		const wasPaused = this->_paused;

		co_await this->_channel->request("rtpObserver.resume", this->_internal);

		this->_paused = false;

		// Emit observer event.
		if (wasPaused)
			this->_observer.safeEmit("resume");
	}

	/**
	 * Add a Producer to the RtpObserver.
	 */
	async addProducer({ producerId }: { producerId: string }): Promise<void>
	{
		logger->debug("addProducer()");

		const producer = this->_getProducerById(producerId);
		const internal = { ...this->_internal, producerId };

		co_await this->_channel->request("rtpObserver.addProducer", internal);

		// Emit observer event.
		this->_observer.safeEmit("addproducer", producer);
	}

	/**
	 * Remove a Producer from the RtpObserver.
	 */
	async removeProducer({ producerId }: { producerId: string }): Promise<void>
	{
		logger->debug("removeProducer()");

		const producer = this->_getProducerById(producerId);
		const internal = { ...this->_internal, producerId };

		co_await this->_channel->request("rtpObserver.removeProducer", internal);

		// Emit observer event.
		this->_observer.safeEmit("removeproducer", producer);
	}
}
