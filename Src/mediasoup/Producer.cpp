
#include "Producer.hpp"

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
	json appData,
	bool paused
)
	: EnhancedEventEmitter()
	, logger(new Logger("Producer"))
{
	logger->debug("constructor()");

	this->_internal = internal;
	this->_data = data;
	this->_channel = channel;
	this->_appData = appData;
	this->_paused = paused;

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
std::vector<ProducerScore> Producer::score()
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
	throw new Error("cannot override appData object");
}

/**
 * Observer.
 *
 * @emits close
 * @emits pause
 * @emits resume
 * @emits score - (score: ProducerScore[])
 * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
 * @emits trace - (trace: ProducerTraceEventData)
 */
EnhancedEventEmitter* Producer::observer()
{
	return this->_observer;
}

/**
 * Close the Producer.
 */
void Producer::close()
{
	if (this->_closed)
		return;

	logger->debug("close()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["producerId"]);

	try
	{
		this->_channel->request("producer.close", this->_internal);
	}
	catch (const std::exception&)
	{

	}

	this->emit("@close");

	// Emit observer event.
	this->_observer->safeEmit("close");
}

/**
 * Transport was closed.
 *
 * @private
 */
void Producer::transportClosed()
{
	if (this->_closed)
		return;

	logger->debug("transportClosed()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["producerId"]);

	this->safeEmit("transportclose");

	// Emit observer event.
	this->_observer->safeEmit("close");
}

/**
 * Dump Producer.
 */
std::future<json> Producer::dump()
{
	logger->debug("dump()");

	json ret = co_await this->_channel->request("producer.dump", this->_internal);

	co_return ret;
}

/**
 * Get Producer stats.
 */
std::future<json> Producer::getStats()
{
	logger->debug("getStats()");

	json ret = co_await this->_channel->request("producer.getStats", this->_internal);

	co_return ret;
}

	 /**
	  * Pause the Producer.
	  */
std::future<void> Producer::pause()
{
	logger->debug("pause()");

	bool wasPaused = this->_paused;

	co_await this->_channel->request("producer.pause", this->_internal);

	this->_paused = true;

	// Emit observer event.
	if (!wasPaused)
		this->_observer->safeEmit("pause");
}

/**
 * Resume the Producer.
 */
std::future<void> Producer::resume()
{
	logger->debug("resume()");

	bool wasPaused = this->_paused;

	co_await this->_channel->request("producer.resume", this->_internal);

	this->_paused = false;

	// Emit observer event.
	if (wasPaused)
		this->_observer->safeEmit("resume");
}

/**
 * Enable "trace" event.
 */
std::future<void> Producer::enableTraceEvent(std::vector<ProducerTraceEventType> types)
{
	logger->debug("enableTraceEvent()");

	json reqData = { {"types", types } };

	co_await this->_channel->request(
		"producer.enableTraceEvent", this->_internal, reqData);
}


void Producer::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["producerId"].get<std::string>(), [=](std::string event, json& data = json::object())
	{
		if (event == "score")
		{
			// 				const score = data as ProducerScore[];
			// 
			// 				this->_score = score;
			// 
			// 				this->safeEmit("score", score);
			// 
			// 				// Emit observer event.
			// 				this->_observer->safeEmit("score", score);
		}
		else if (event == "videoorientationchange")
		{
			// 				const videoOrientation = data as ProducerVideoOrientation;
			// 
			// 				this->safeEmit("videoorientationchange", videoOrientation);
			// 
			// 				// Emit observer event.
			// 				this->_observer->safeEmit("videoorientationchange", videoOrientation);
		}
		else if (event == "trace")
		{
			// 				const trace = data as ProducerTraceEventData;
			// 
			// 				this->safeEmit("trace", trace);
			// 
			// 				// Emit observer event.
			// 				this->_observer->safeEmit("trace", trace);
		}
		else
		{
			logger->error("ignoring unknown event \"%s\"", event);
		}

	});
}

