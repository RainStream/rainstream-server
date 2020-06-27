
#include "Consumer.hpp"
#include "Channel.hpp"
#include "Producer.hpp"
#include "RtpParameters.hpp"


Consumer::Consumer(
	json internal,
	json data,
	Channel* channel,
	json appData,
	bool paused,
	bool producerPaused,
	ConsumerScore score = ConsumerScore(),
	ConsumerLayers preferredLayers = ConsumerLayers())
	: EnhancedEventEmitter(),
	logger(new Logger("Consumer"))
{
	logger->debug("constructor()");

	this->_internal = internal;
	this->_data = data;
	this->_channel = channel;
	this->_appData = appData;
	this->_paused = paused;
	this->_producerPaused = producerPaused;
	this->_score = score;
	this->_preferredLayers = preferredLayers;

	this->_handleWorkerNotifications();
}

/**
 * Consumer id.
 */
std::string Consumer::id()
{
	return this->_internal["consumerId"];
}

/**
 * Associated Producer id.
 */
std::string Consumer::producerId()
{
	return this->_internal["producerId"];
}

/**
 * Whether the Consumer is closed.
 */
bool Consumer::closed()
{
	return this->_closed;
}

/**
 * Media kind.
 */
std::string Consumer::kind()
{
	return this->_data["kind"];
}

/**
 * RTP parameters.
 */
json Consumer::rtpParameters()
{
	return this->_data["rtpParameters"];
}

/**
 * Consumer type.
 */
ConsumerType Consumer::type()
{
	return this->_data["type"];
}

/**
 * Whether the Consumer is paused.
 */
bool Consumer::paused()
{
	return this->_paused;
}

/**
 * Whether the associate Producer is paused.
 */
bool Consumer::producerPaused()
{
	return this->_producerPaused;
}

/**
 * Current priority.
 */
int Consumer::priority()
{
	return this->_priority;
}

/**
 * Consumer score.
 */
ConsumerScore Consumer::score()
{
	return this->_score;
}

/**
 * Preferred video layers.
 */
ConsumerLayers Consumer::preferredLayers()
{
	return this->_preferredLayers;
}

/**
 * Current video layers.
 */
ConsumerLayers Consumer::currentLayers()
{
	return this->_currentLayers;
}

/**
 * App custom data.
 */
json Consumer::appData()
{
	return this->_appData;
}

/**
 * Invalid setter.
 */
void Consumer::appData(json appData) // eslint-disable-line no-unused-vars
{
	throw new Error("cannot override appData object");
}

/**
 * Observer.
 *
 * @emits close
 * @emits pause
 * @emits resume
 * @emits score - (score: ConsumerScore)
 * @emits layerschange - (layers: ConsumerLayers | undefined)
 * @emits trace - (trace: ConsumerTraceEventData)
 */
EnhancedEventEmitter* Consumer::observer()
{
	return this->_observer;
}

/**
 * Close the Consumer.
 */
void Consumer::close()
{
	if (this->_closed)
		return;

	logger->debug("close()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["consumerId"]);

	try
	{
		this->_channel->request("consumer.close", this->_internal);
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
void Consumer::transportClosed()
{
	if (this->_closed)
		return;

	logger->debug("transportClosed()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["consumerId"]);

	this->safeEmit("transportclose");

	// Emit observer event.
	this->_observer->safeEmit("close");
}

/**
 * Dump Consumer.
 */
std::future<json> Consumer::dump()
{
	logger->debug("dump()");

	json ret = co_await this->_channel->request("consumer.dump", this->_internal);

	co_return ret;
}

/**
 * Get Consumer stats.
 */
std::future<json> Consumer::getStats()
{
	logger->debug("getStats()");

	json ret = co_await  this->_channel->request("consumer.getStats", this->_internal);

	co_return ret;
}

/**
 * Pause the Consumer.
 */
std::future<void> Consumer::pause()
{
	logger->debug("pause()");

	bool wasPaused = this->_paused || this->_producerPaused;

	co_await this->_channel->request("consumer.pause", this->_internal);

	this->_paused = true;

	// Emit observer event.
	if (!wasPaused)
		this->_observer->safeEmit("pause");
}

/**
 * Resume the Consumer.
 */
std::future<void> Consumer::resume()
{
	logger->debug("resume()");

	bool wasPaused = this->_paused || this->_producerPaused;

	co_await this->_channel->request("consumer.resume", this->_internal);

	this->_paused = false;

	// Emit observer event.
	if (wasPaused && !this->_producerPaused)
		this->_observer->safeEmit("resume");
}

/**
 * Set preferred video layers.
 */
std::future<void> Consumer::setPreferredLayers(
	uint32_t spatialLayer,
	uint32_t temporalLayer
)
{
	logger->debug("setPreferredLayers()");

	json reqData = {
		{ "spatialLayer",spatialLayer },
		{"temporalLayer", temporalLayer}
	};

	json data = co_await this->_channel->request(
		"consumer.setPreferredLayers", this->_internal, reqData);

	this->_preferredLayers = data /*|| undefined*/;
}

/**
 * Set priority.
 */
std::future<void> Consumer::setPriority(int priority)
{
	logger->debug("setPriority()");

	json reqData = { { "priority" , priority } };

	json data = co_await this->_channel->request(
		"consumer.setPriority", this->_internal, reqData);

	this->_priority = data["priority"];
}

/**
 * Unset priority.
 */
std::future<void> Consumer::unsetPriority()
{
	logger->debug("unsetPriority()");

	json reqData = { { "priority" , 1 } };

	json data = co_await this->_channel->request(
		"consumer.setPriority", this->_internal, reqData);

	this->_priority = data["priority"];
}

/**
 * Request a key frame to the Producer.
 */
std::future<void> Consumer::requestKeyFrame()
{
	logger->debug("requestKeyFrame()");

	co_await this->_channel->request("consumer.requestKeyFrame", this->_internal);
}

/**
 * Enable "trace" event.
 */
std::future<void> Consumer::enableTraceEvent(std::vector<ConsumerTraceEventType> types)
{
	logger->debug("enableTraceEvent()");

	json reqData = { types };

	co_await this->_channel->request(
		"consumer.enableTraceEvent", this->_internal, reqData);
}

void Consumer::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["consumerId"], [=](std::string event, json data)
	{
		if (event == "producerclose")
		{
			// 				if (this->_closed)
			// 					return;
			// 
			// 				this->_closed = true;
			// 
			// 				// Remove notification subscriptions.
			// 				this->_channel->removeAllListeners(this->_internal["consumerId"]);
			// 
			// 				this->emit("@producerclose");
			// 				this->safeEmit("producerclose");
			// 
			// 				// Emit observer event.
			// 				this->_observer->safeEmit("close");
		}
		else if (event == "producerpause")
		{
			// 				if (this->_producerPaused)
			// 					break;
			// 
			// 				bool wasPaused = this->_paused || this->_producerPaused;
			// 
			// 				this->_producerPaused = true;
			// 
			// 				this->safeEmit("producerpause");
			// 
			// 				// Emit observer event.
			// 				if (!wasPaused)
			// 					this->_observer->safeEmit("pause");
		}
		else if (event == "producerresume")
		{
			// 				if (!this->_producerPaused)
			// 					break;
			// 
			// 				bool wasPaused = this->_paused || this->_producerPaused;
			// 
			// 				this->_producerPaused = false;
			// 
			// 				this->safeEmit("producerresume");
			// 
			// 				// Emit observer event.
			// 				if (wasPaused && !this->_paused)
			// 					this->_observer->safeEmit("resume");
		}
		else if (event == "score")
		{
			// 				const score = data as ConsumerScore;
			// 
			// 				this->_score = score;
			// 
			// 				this->safeEmit("score", score);
			// 
			// 				// Emit observer event.
			// 				this->_observer->safeEmit("score", score);
		}
		else if (event == "layerschange")
		{
			// 				const layers = data as ConsumerLayers | undefined;
			// 
			// 				this->_currentLayers = layers;
			// 
			// 				this->safeEmit("layerschange", layers);
			// 
			// 				// Emit observer event.
			// 				this->_observer->safeEmit("layerschange", layers);
		}
		else if (event == "trace")
		{
			// 				const trace = data as ConsumerTraceEventData;
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
};
