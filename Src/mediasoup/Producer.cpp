#define MSC_CLASS "Producer"

#include "common.hpp"
#include "Producer.hpp"
#include "Logger.hpp"
#include "errors.hpp"
#include "Channel.hpp"
#include "PayloadChannel.hpp"

/**
 * @private
 * @emits transportclose
 * @emits score - (score: ProducerScore[])
 * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
 * @emits trace - (trace: ProducerTraceEventData)
 * @emits @close
 */
Producer::Producer(json internal,
	json data,
	Channel* channel,
	PayloadChannel* payloadChannel,
	json appData,
	bool paused)
	: _observer(new EnhancedEventEmitter())
	, _internal(internal)
	, _data(data)
	, _channel(channel)
	, _payloadChannel(payloadChannel)
	, _appData(appData)
	, _paused(paused)
{
	MSC_DEBUG("constructor()");

	this->_handleWorkerNotifications();
}

/**
 * Producer id.
 */
std::string Producer::id()
{
	return this->_internal["producerId"];
}

/**
 * Whether the Producer is closed.
 */
bool Producer::closed()
{
	return this->_closed;
}

/**
 * Media kind.
 */
std::string Producer::kind()
{
	return this->_data["kind"];
}

/**
 * RTP parameters.
 */
json Producer::rtpParameters()
{
	return this->_data["rtpParameters"];
}

/**
 * Producer type.
 */
ProducerType Producer::type()
{
	return this->_data["type"];
}

/**
 * Consumable RTP parameters.
 *
 * @private
 */
json Producer::consumableRtpParameters()
{
	return this->_data["consumableRtpParameters"];
}

/**
 * Whether the Producer is paused.
 */
bool Producer::paused()
{
	return this->_paused;
}

/**
 * Producer score list.
 */
json Producer::score()
{
	return this->_score;
}

/**
 * App custom data.
 */
json Producer::appData()
{
	return this->_appData;
}

/**
 * Invalid setter.
 */
void Producer::appData(json appData) // eslint-disable-line no-unused-vars
{
	MSC_THROW_ERROR("cannot override appData object");
}

EnhancedEventEmitter* Producer::observer()
{
	return this->_observer;
}

Channel* Producer::channelForTesting()
{
	return this->_channel;
}

/**
 * Close the Producer.
 */
void Producer::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["producerId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["producerId"]);

	try
	{
		json reqData = { { "producerId", this->_internal["producerId"] } };
		this->_channel->request("transport.closeProducer", this->_internal["transportId"], reqData);
	}
	catch (const std::exception& error)
	{
	}

	this->emit("@close");
	// Emit observer event.
	this->_observer->safeEmit("close");

	delete this;
}

void Producer::transportClosed()
{
	if (this->_closed)
		return;

	MSC_DEBUG("transportClosed()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["producerId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["producerId"]);

	this->safeEmit("transportclose");
	// Emit observer event.
	this->_observer->safeEmit("close");

	delete this;
}

std::future<json> Producer::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("producer.dump", this->_internal["producerId"]);

	co_return ret;
}

/**
 * Get Producer stats.
 */
std::future<json> Producer::getStats()
{
	MSC_DEBUG("getStats()");

	json ret = co_await this->_channel->request("producer.getStats", this->_internal["producerId"]);

	co_return ret;
}

	 /**
	  * Pause the Producer.
	  */
std::future<void> Producer::pause()
{
	MSC_DEBUG("pause()");

	bool wasPaused = this->_paused;

	co_await this->_channel->request("producer.pause", this->_internal["producerId"]);

	this->_paused = true;

	// Emit observer event.
	if (!wasPaused)
		this->_observer->safeEmit("pause");
}

std::future<void> Producer::resume()
{
	MSC_DEBUG("resume()");

	bool wasPaused = this->_paused;

	co_await this->_channel->request("producer.resume", this->_internal["producerId"]);

	this->_paused = false;

	// Emit observer event.
	if (wasPaused)
		this->_observer->safeEmit("resume");
}

std::future<void> Producer::enableTraceEvent(std::vector<ProducerTraceEventType> types)
{
	MSC_DEBUG("enableTraceEvent()");

	json reqData = { {"types", types } };

	co_await this->_channel->request(
		"producer.enableTraceEvent", this->_internal["producerId"], reqData);
}

//send(rtpPacket) {
//	if (!Buffer.isBuffer(rtpPacket)) {
//		throw new TypeError('rtpPacket must be a Buffer');
//	}
//	this.#payloadChannel.notify('producer.send', this.#internal.producerId, undefined, rtpPacket);
//}

void Producer::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["producerId"].get<std::string>(), [=](std::string event, json& data)
	{
		if (event == "score")
		{
			const json& score = data;

			this->_score = score;

			this->safeEmit("score", score);
			// Emit observer event.
			this->_observer->safeEmit("score", score);
		}
		else if (event == "videoorientationchange")
		{
			const json& videoOrientation = data;

			this->safeEmit("videoorientationchange", videoOrientation);
			// Emit observer event.
			this->_observer->safeEmit("videoorientationchange", videoOrientation);
		}
		else if (event == "trace")
		{
			const json& trace = data;

			this->safeEmit("trace", trace);
			// Emit observer event.
			this->_observer->safeEmit("trace", trace);
		}
		else
		{
			MSC_ERROR("ignoring unknown event \"%s\"", event.c_str());
		}

	});
}


