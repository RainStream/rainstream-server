#include "RainStream.hpp"
#include "PlainRtpTransport.hpp"
#include "Channel.hpp"
#include "Logger.hpp"

namespace rs
{

	PlainRtpTransport::PlainRtpTransport(const Json& internal, const Json& data, Channel* channel)
	{
		DLOG(INFO) << "constructor()";

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
		DLOG(INFO) << "close()";

		if (this->_closed)
			return;

		this->_closed = true;

		this->doEvent("@close");

		// Remove notification subscriptions.
		uint32_t transportId = this->_internal["transportId"].get<uint32_t>();
		this->_channel->off(transportId);

		if (notifyChannel)
		{
			this->_channel->request("transport.close", this->_internal)
				.then([=]()
			{
				DLOG(INFO) << "\"transport.close\" request succeeded";
			})
				.fail([=](Error error)
			{
				LOG(ERROR) << "\"transport.close\" request failed:"<< error.ToString();
			});
		}

		delete this;
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
		DLOG(INFO) << "dump()";

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("PlainRtpTransport closed"));

		return this->_channel->request("transport.dump", this->_internal)
			.then([=](Json data)
		{
			DLOG(INFO) << "\"transport.dump\" request succeeded";

			return data;
		})
			.fail([=](Error error)
		{
			LOG(ERROR) << "\"transport.dump\" request failed:" << error.ToString();

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
			LOG(ERROR) << "ignoring unknown event:" << event;
		}
	}

}