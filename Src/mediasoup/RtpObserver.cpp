#define MSC_CLASS "RtpObserver"

#include "common.hpp"
#include "Logger.hpp"
#include "errors.hpp"
#include "Channel.hpp"
#include "PayloadChannel.hpp"
#include "RtpObserver.hpp"



RtpObserver::RtpObserver(
	json internal,
	Channel* channel,
	PayloadChannel* payloadChannel,
	json appData,
	GetProducerById getProducerById
)
	: _observer(new EnhancedEventEmitter())
	, _internal(internal)
	, _channel(channel)
	, _payloadChannel(payloadChannel)
	, _appData(appData)
	, _getProducerById(getProducerById)
{
	MSC_DEBUG("constructor()");
}

RtpObserver::~RtpObserver()
{
	delete _observer;
	_observer = nullptr;
}

std::string RtpObserver::id()
{
	return this->_internal["rtpObserverId"];
}

bool RtpObserver::closed()
{
	return this->_closed;
}

bool RtpObserver::paused()
{
	return this->_paused;
}

json RtpObserver::appData()
{
	return this->_appData;
}

void RtpObserver::appData(json appData) // eslint-disable-line no-unused-vars
{
	MSC_THROW_ERROR("cannot override appData object");
}

EnhancedEventEmitter* RtpObserver::observer()
{
	return this->_observer;
}

void RtpObserver::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["rtpObserverId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["rtpObserverId"]);


	try
	{
		json reqData = { { "rtpObserverId", this->_internal["rtpObserverId"]} };
		this->_channel->request("router.closeRtpObserver", this->_internal["routerId"], reqData);
	}
	catch (const std::exception&)
	{

	}

	this->emit("@close");
	// Emit observer event.
	this->_observer->safeEmit("close");
}

void RtpObserver::routerClosed()
{
	if (this->_closed)
		return;

	MSC_DEBUG("routerClosed()");

	this->_closed = true;

	// Remove notification subscriptions.
	this->_channel->removeAllListeners(this->_internal["rtpObserverId"]);
	this->_payloadChannel->removeAllListeners(this->_internal["rtpObserverId"]);

	this->safeEmit("routerclose");
	// Emit observer event.
	this->_observer->safeEmit("close");
}

std::future<void> RtpObserver::pause()
{
	MSC_DEBUG("pause()");

	bool wasPaused = this->_paused;

	co_await this->_channel->request("rtpObserver.pause", this->_internal["rtpObserverId"]);

	this->_paused = true;

	// Emit observer event.
	if (!wasPaused)
		this->_observer->safeEmit("pause");
}

/**
 * Resume the RtpObserver.
 */
std::future<void> RtpObserver::resume()
{
	MSC_DEBUG("resume()");

	bool wasPaused = this->_paused;

	co_await this->_channel->request("rtpObserver.resume", this->_internal["rtpObserverId"]);

	this->_paused = false;

	// Emit observer event.
	if (wasPaused)
		this->_observer->safeEmit("resume");
}

/**
 * Add a Producer to the RtpObserver.
 */
std::future<void> RtpObserver::addProducer(std::string producerId)
{
	MSC_DEBUG("addProducer()");

	Producer* producer = this->_getProducerById(producerId);
	if (!producer)
		MSC_THROW_ERROR("Producer with id \"%s\" not found", producerId.c_str());

	json reqData = { { "producerId", producerId }};

	co_await this->_channel->request("rtpObserver.addProducer", this->_internal["rtpObserverId"], reqData);

	// Emit observer event.
	this->_observer->safeEmit("addproducer", producer);
}

/**
 * Remove a Producer from the RtpObserver.
 */
std::future<void> RtpObserver::removeProducer(std::string producerId)
{
	MSC_DEBUG("removeProducer()");

	Producer* producer = this->_getProducerById(producerId);
	if (!producer)
		MSC_THROW_ERROR("Producer with id \"%s\" not found", producerId.c_str());

	json reqData = { { "producerId", producerId } };

	co_await this->_channel->request("rtpObserver.removeProducer", this->_internal["rtpObserverId"], reqData);

	// Emit observer event.
	this->_observer->safeEmit("removeproducer", producer);
}
