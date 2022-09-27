#pragma once 

#include "EnhancedEventEmitter.hpp"
#include "Logger.hpp"
#include "Channel.hpp"
//#include "PayloadChannel.hpp"
#include "Producer.hpp"

class Producer;

class RtpObserver : public EnhancedEventEmitter
{
	// Internal data.
protected:
	json _internal;
// 	{
// 		std::string routerId;
// 		std::string rtpObserverId;
// 	};

	// Channel instance.
	Channel* _channel;

	// PayloadChannel instance.
	PayloadChannel* _payloadChannel;

	// Closed flag.
	bool _closed = false;

	// Paused flag.
	bool _paused = false;

	// Custom app data.
private:
	json _appData;

	// Method to retrieve a Producer.
protected:
	GetProducerById _getProducerById;

	// Observer instance.
	EnhancedEventEmitter* _observer;
	// Internal data.
private:
	json _internal;
	// 	{
	//		std::string routerId;
	// 		std::string transportId;
	// 		std::string dataProducerId;
	// 		std::string dataConsumerId;
	// 	};

		// DataConsumer data.
	json _data;
	// 	{
	// 		type: DataConsumerType;
	// 		sctpStreamParameters?: SctpStreamParameters;
	// 		std::string label;
	// 		std::string protocol;
	// 	};

		// Channel instance.
	Channel* _channel;

	// PayloadChannel instance.
	PayloadChannel* _payloadChannel;

	// Closed flag.
	bool _closed = false;

	// Custom app data.
	json _appData;

	// Observer instance.
	EnhancedEventEmitter* _observer{ nullptr };

public:
	/**
	 * @private
	 * @interface
	 * @emits routerclose
	 * @emits @close
	 */
	RtpObserver(
		json internal,
		Channel* channel,
		PayloadChannel* payloadChannel,
		json appData,
		GetProducerById getProducerById
	)
	{
		MSC_DEBUG("constructor()");

		this->_internal = internal;
		this->_channel = channel;
		this->_payloadChannel = payloadChannel;
		this->_appData = appData;
		this->_getProducerById = getProducerById;
	}

	/**
	 * RtpObserver id.
	 */
	std::string id()
	{
		return this->_internal["rtpObserverId"];
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
	void appData(json appData) // eslint-disable-line no-unused-vars
	{
		MSC_THROW_ERROR("cannot override appData object");
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
	EnhancedEventEmitter* observer()
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

		MSC_DEBUG("close()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal["rtpObserverId"]);

		try
		{
			this->_channel->request("rtpObserver.close", this->_internal);
		}
		catch (const std::exception&)
		{

		}

		this->emit("@close");

		// Emit observer event.
		this->_observer->safeEmit("close");
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

		MSC_DEBUG("routerClosed()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal["rtpObserverId"]);

		this->safeEmit("routerclose");

		// Emit observer event.
		this->_observer->safeEmit("close");
	}

	/**
	 * Pause the RtpObserver.
	 */
	std::future<void> pause()
	{
		MSC_DEBUG("pause()");

		bool wasPaused = this->_paused;

		co_await this->_channel->request("rtpObserver.pause", this->_internal);

		this->_paused = true;

		// Emit observer event.
		if (!wasPaused)
			this->_observer->safeEmit("pause");
	}

	/**
	 * Resume the RtpObserver.
	 */
	std::future<void> resume()
	{
		MSC_DEBUG("resume()");

		bool wasPaused = this->_paused;

		co_await this->_channel->request("rtpObserver.resume", this->_internal);

		this->_paused = false;

		// Emit observer event.
		if (wasPaused)
			this->_observer->safeEmit("resume");
	}

	/**
	 * Add a Producer to the RtpObserver.
	 */
	std::future<void> addProducer(std::string producerId)
	{
		MSC_DEBUG("addProducer()");

		Producer* producer = this->_getProducerById(producerId);
		json internal = this->_internal;
		internal["producerId"] = producerId;

		co_await this->_channel->request("rtpObserver.addProducer", internal);

		// Emit observer event.
		this->_observer->safeEmit("addproducer", producer);
	}

	/**
	 * Remove a Producer from the RtpObserver.
	 */
	std::future<void> removeProducer(std::string producerId)
	{
		MSC_DEBUG("removeProducer()");

		Producer* producer = this->_getProducerById(producerId);
		json internal = this->_internal;
		internal["producerId"] = producerId;

		co_await this->_channel->request("rtpObserver.removeProducer", internal);

		// Emit observer event.
		this->_observer->safeEmit("removeproducer", producer);
	}
};
