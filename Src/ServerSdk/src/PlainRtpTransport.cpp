#include "RainStream.hpp"
#include "PlainRtpTransport.hpp"
#include "Channel.hpp"
#include "Logger.hpp"

namespace rs
{

	PlainRtpTransport::PlainRtpTransport(const Json& internal, const Json& data, Channel* channel)
		: EnhancedEventEmitter()
		, logger(new Logger("PlainRtpTransport"))
	{
		logger->debug("constructor()");

		// Closed flag.
		this->_closed = false;

		// Internal data.
		// - .routerId
		// - .transportId
		this->_internal = internal;

		// PlainRtpTransport data provided by the worker.
		// - .tuple
		//   - .local
		//     - .ip
		//     - .port
		//     - .protocol
		//   - .remote
		//     - .ip
		//     - .port
		//     - .protocol
		this->_data = data;

		// Channel instance.
		this->_channel = channel;

		this->_handleWorkerNotifications();
	}

	uint32_t PlainRtpTransport::id()
	{
		return this->_internal["transportId"];
	}

	bool PlainRtpTransport::closed()
	{
		return this->_closed;
	}

	Json PlainRtpTransport::tuple()
	{
		return this->_data["tuple"];
	}

	/**
	* Close the PlainRtpTransport.
	*
	* @param {bool} [notifyChannel=true] - Private.
	*/
	void PlainRtpTransport::close(bool notifyChannel /*= true*/)
	{
		logger->debug("close()");

		if (this->_closed)
			return;

		this->_closed = true;

		this->emit("@close");

		// Remove notification subscriptions.
		uint32_t transportId = this->_internal["transportId"].get<uint32_t>();
		this->_channel->off(transportId);

		if (notifyChannel)
		{
			this->_channel->request("transport.close", this->_internal)
				.then([=]()
			{
				logger->debug("\"transport.close\" request succeeded");
			})
				.fail([=](Error error)
			{
				logger->error("\"transport.close\" request failed: %s", error.ToString().c_str());
			});
		}
	}

	/**
	* Dump the PlainRtpTransport.
	*
	* @private
	*
	* @return {Promise}
	*/
	Defer PlainRtpTransport::dump()
	{
		logger->debug("dump()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("PlainRtpTransport closed"));

		return this->_channel->request("transport.dump", this->_internal)
			.then([=](Json data)
		{
			logger->debug("\"transport.dump\" request succeeded");

			return data;
		})
			.fail([=](Error error)
		{
			logger->error("\"transport.dump\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Enables periodic stats retrieval.
	*
	* Not implemented.
	*
	*/
	void PlainRtpTransport::enableStats()
	{
	}

	/**
	* Disables periodic stats retrieval.
	*
	* Not implemented.
	*
	*/
	void PlainRtpTransport::disableStats()
	{
	}

	void PlainRtpTransport::_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		uint32_t transportId = this->_internal["transportId"].get<uint32_t>();
		this->_channel->addEventListener(transportId, this);
	}

	void PlainRtpTransport::onEvent(std::string event, Json data)
	{
		if (event == "close")
		{
			this->close(false);
		}
		else
		{
			logger->error("ignoring unknown event \"%s\"", event);
		}
	}

}