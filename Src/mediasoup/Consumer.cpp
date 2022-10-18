#define MSC_CLASS "Consumer"

#include "common.hpp"
#include "Consumer.hpp"
#include "Channel.hpp"
#include "PayloadChannel.hpp"
#include "Logger.hpp"
#include "errors.hpp"


Consumer::Consumer(
	json internal,
	json data,
	Channel* channel,
	PayloadChannel* payloadChannel,
	json appData,
	bool paused,
	bool producerPaused,
	json score/* = json()*/,
	ConsumerLayers preferredLayers/* = ConsumerLayers()*/)
	: _observer(new EnhancedEventEmitter())
	, _internal(internal)
	, _data(data)
	, _channel(channel)
	, _payloadChannel(payloadChannel)
	, _appData(appData)
	, _paused(paused)
	, _producerPaused(producerPaused)
	, _score(score)
	, _preferredLayers(preferredLayers)
{
	MSC_DEBUG("constructor()");

	this->_handleWorkerNotifications();
}

Consumer::~Consumer()
{
	delete _observer;
	_observer = nullptr;
}

std::string Consumer::id()
{
	return this->_internal["consumerId"];
}

std::string Consumer::producerId()
{
	return this->_internal["producerId"];
}

bool Consumer::closed()
{
	return this->_closed;
}

std::string Consumer::kind()
{
	return this->_data["kind"];
}

json Consumer::rtpParameters()
{
	return this->_data["rtpParameters"];
}

ConsumerType Consumer::type()
{
	return this->_data["type"];
}

bool Consumer::paused()
{
	return this->_paused;
}

bool Consumer::producerPaused()
{
	return this->_producerPaused;
}

int Consumer::priority()
{
	return this->_priority;
}

json Consumer::score()
{
	return this->_score;
}

ConsumerLayers Consumer::preferredLayers()
{
	return this->_preferredLayers;
}

ConsumerLayers Consumer::currentLayers()
{
	return this->_currentLayers;
}

json Consumer::appData()
{
	return this->_appData;
}

void Consumer::appData(json appData) // eslint-disable-line no-unused-vars
{
	MSC_THROW_ERROR("cannot override appData object");
}

EnhancedEventEmitter* Consumer::observer()
{
	return this->_observer;
}

Channel* Consumer::channelForTesting()
{
	return this->_channel;
}

void Consumer::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["consumerId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["consumerId"]);

	try
	{
		json reqData = { 
			{"consumerId", this->_internal["consumerId"] }
		};
		this->_channel->request("transport.closeConsumer", this->_internal["transportId"], reqData);
	}
	catch (const std::exception&)
	{

	}

	this->emit("@close");
	// Emit observer event.
	this->_observer->safeEmit("close");
}

void Consumer::transportClosed()
{
	if (this->_closed)
		return;

	MSC_DEBUG("transportClosed()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["consumerId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["consumerId"]);

	this->safeEmit("transportclose");
	// Emit observer event.
	this->_observer->safeEmit("close");
}

task_t<json> Consumer::dump()
{
	MSC_DEBUG("dump()");

	json ret = co_await this->_channel->request("consumer.dump", this->_internal["consumerId"]);

	co_return ret;
}

task_t<json> Consumer::getStats()
{
	MSC_DEBUG("getStats()");

	json ret = co_await  this->_channel->request("consumer.getStats", this->_internal["consumerId"]);

	co_return ret;
}

task_t<void> Consumer::pause()
{
	MSC_DEBUG("pause()");

	bool wasPaused = this->_paused || this->_producerPaused;

	co_await this->_channel->request("consumer.pause", this->_internal["consumerId"]);

	this->_paused = true;

	// Emit observer event.
	if (!wasPaused)
		this->_observer->safeEmit("pause");
}

task_t<void> Consumer::resume()
{
	MSC_DEBUG("resume()");

	bool wasPaused = this->_paused || this->_producerPaused;

	co_await this->_channel->request("consumer.resume", this->_internal["consumerId"]);

	this->_paused = false;

	// Emit observer event.
	if (wasPaused && !this->_producerPaused)
		this->_observer->safeEmit("resume");
}

task_t<void> Consumer::setPreferredLayers(
	int spatialLayer,
	int temporalLayer
)
{
	MSC_DEBUG("setPreferredLayers()");

	json reqData = {
		{ "spatialLayer",spatialLayer },
		{"temporalLayer", temporalLayer}
	};

	json data = co_await this->_channel->request(
		"consumer.setPreferredLayers", this->_internal["consumerId"], reqData);

	this->_preferredLayers = data /*|| undefined*/;
}

task_t<void> Consumer::setPriority(int priority)
{
	MSC_DEBUG("setPriority()");

	json reqData = { { "priority" , priority } };

	json data = co_await this->_channel->request(
		"consumer.setPriority", this->_internal["consumerId"], reqData);

	this->_priority = data["priority"];
}

task_t<void> Consumer::unsetPriority()
{
	MSC_DEBUG("unsetPriority()");

	json reqData = { { "priority" , 1 } };

	json data = co_await this->_channel->request(
		"consumer.setPriority", this->_internal["consumerId"], reqData);

	this->_priority = data["priority"];
}

task_t<void> Consumer::requestKeyFrame()
{
	MSC_DEBUG("requestKeyFrame()");

	co_await this->_channel->request("consumer.requestKeyFrame", this->_internal["consumerId"]);
}

task_t<void> Consumer::enableTraceEvent(std::vector<ConsumerTraceEventType> types)
{
	MSC_DEBUG("enableTraceEvent()");

	json reqData = { types };

	co_await this->_channel->request(
		"consumer.enableTraceEvent", this->_internal["consumerId"], reqData);
}

void Consumer::_handleWorkerNotifications()
{
	this->_channel->on(this->_internal["consumerId"], [=](std::string event, const json& data)
	{
		if (event == "producerclose")
		{
			if (this->_closed)
				return;

			this->_closed = true;

			// Remove notification subscriptions.
			this->_channel->removeAllListeners(this->_internal["consumerId"]);
			this->_payloadChannel->removeAllListeners(this->_internal["consumerId"]);

			this->emit("@producerclose");
			this->safeEmit("producerclose");

			// Emit observer event.
			this->_observer->safeEmit("close");

			delete this;
		}
		else if (event == "producerpause")
		{
			if (this->_producerPaused)
				return;

			bool wasPaused = (this->_paused || this->_producerPaused);

			this->_producerPaused = true;

			this->safeEmit("producerpause");

			// Emit observer event.
			if (!wasPaused)
				this->_observer->safeEmit("pause");
		}
		else if (event == "producerresume")
		{
			if (!this->_producerPaused)
				return;

			bool wasPaused = (this->_paused || this->_producerPaused);

			this->_producerPaused = false;

			this->safeEmit("producerresume");

			// Emit observer event.
			if (wasPaused && !this->_paused)
				this->_observer->safeEmit("resume");
		}
		else if (event == "score")
		{
			this->_score = data;

			this->safeEmit("score", this->_score);

			// Emit observer event.
			this->_observer->safeEmit("score", this->_score);
		}
		else if (event == "layerschange")
		{
			const json& layers = data;

			this->_currentLayers = layers;

			this->safeEmit("layerschange", layers);

			// Emit observer event.
			this->_observer->safeEmit("layerschange", layers);
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

	this->_payloadChannel->on(this->_internal["consumerId"], [=](std::string event, const json& data, std::string payload) {
		if (event == "rtp")
		{
			if (this->_closed)
				return;
			/*const packet = payload;
			this.safeEmit('rtp', packet);*/
		}
		else
		{
			MSC_ERROR("ignoring unknown event \"%s\"", event.c_str());
		}
	});
}

