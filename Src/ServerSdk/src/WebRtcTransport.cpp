#include "RainStream.hpp"
#include "WebRtcTransport.hpp"
#include "Channel.hpp"
#include "Logger.hpp"

namespace rs
{

	WebRtcTransport::WebRtcTransport(const Json& internal, const Json& data, Channel* channel)
		: EnhancedEventEmitter()
		, logger(new Logger("WebRtcTransport"))
	{
		logger->debug("constructor()");

		// Closed flag.
		this->_closed = false;

		// Internal data.
		// - .routerId
		// - .transportId
		this->_internal = internal;

		// WebRtcTransport data provided by the worker.
		// - .direction
		// - .iceRole
		// - .iceLocalParameters
		//   - .usernameFragment
		//   - .password
		//   - .iceLite
		// - .iceLocalCandidates []
		//   - .foundation
		//   - .priority
		//   - .ip
		//   - .port
		//   - .type
		//   - .protocol
		//   - .tcpType
		// - .iceState
		// - .iceSelectedTuple
		//   - .local
		//     - .ip
		//     - .port
		//     - .protocol
		//   - .remote
		//     - .ip
		//     - .port
		//     - .protocol
		// - .dtlsLocalParameters
		//   - .role
		//   - .fingerprints
		//     - .sha-1
		//     - .sha-224
		//     - .sha-256
		//     - .sha-384
		//     - .sha-512
		// - .dtlsState
		// - .dtlsRemoteCert
		this->_data = data;

		// Channel instance.
		this->_channel = channel;

		// App data.
		this->_appData = undefined;

		// Periodic stats flag.
		// @type {bool}
		this->_statsEnabled = false;

		// Periodic stats interval identifier.
		// @type {bool}
		this->_statsInterval = NULL;

		this->_handleWorkerNotifications();
	}

	uint32_t WebRtcTransport::id()
	{
		return this->_internal["transportId"];
	}

	bool WebRtcTransport::closed()
	{
		return this->_closed;
	}

	Json WebRtcTransport::appData()
	{
		return this->_appData;
	}

	void WebRtcTransport::appData(Json appData)
	{
		this->_appData = appData;
	}

	std::string WebRtcTransport::direction()
	{
		return this->_data["direction"];
	}

	std::string WebRtcTransport::iceRole()
	{
		return this->_data["iceRole"];
	}

	Json WebRtcTransport::iceLocalParameters()
	{
		return this->_data["iceLocalParameters"];
	}

	Json WebRtcTransport::iceLocalCandidates()
	{
		return this->_data["iceLocalCandidates"];
	}

	Json WebRtcTransport::iceState()
	{
		return this->_data["iceState"];
	}

	Json WebRtcTransport::iceSelectedTuple()
	{
		return this->_data["iceSelectedTuple"];
	}

	Json WebRtcTransport::dtlsLocalParameters()
	{
		return this->_data["dtlsLocalParameters"];
	}

	std::string WebRtcTransport::dtlsState()
	{
		return this->_data["dtlsState"].get<std::string>();
	}

	Json WebRtcTransport::dtlsRemoteCert()
	{
		return this->_data["dtlsRemoteCert"];
	}

	/**
	* Close the WebRtcTransport.
	*
	* @param {Any} [appData] - App custom data.
	* @param {bool} [notifyChannel=true] - Private.
	*/
	void WebRtcTransport::close(Json appData, bool notifyChannel/* = true*/)
	{
		logger->debug("close()");

		if (this->_closed)
			return;

		this->_closed = true;

		if (this->_statsEnabled)
		{
			this->_statsEnabled = false;
			clearInterval(this->_statsInterval);
		}

		this->emit("@notify", "transportClosed", Json{ { "id" , this->id() }, appData });

		this->emit("@close");
		this->safeEmit("close", "local", appData);

		this->_destroy(notifyChannel);
	}

	/**
	* Remote WebRtcTransport was closed.
	* Invoked via remote notification.
	*
	* @private
	*
	* @param {Any} [appData] - App custom data.
	* @param {bool} [notifyChannel=true] - Private.
	*/
	void WebRtcTransport::remoteClose(Json appData, bool notifyChannel/* = true*/)
	{
		logger->debug("remoteClose()");

		if (this->_closed)
			return;

		this->_closed = true;

		this->emit("@close");
		this->safeEmit("close", "remote", appData);

		this->_destroy(notifyChannel);
	}

	void WebRtcTransport::_destroy(bool notifyChannel/* = true*/)
	{
		// Remove notification subscriptions.
		uint32_t transportId = this->_internal["transportId"].get<uint32_t>();
		this->_channel->off(transportId);

		// We won"t receive events anymore, so be nice and manually update some
		// fields.
		this->_data.erase("iceSelectedTuple");
		this->_data["iceState"] = "closed";
		this->_data["dtlsState"] = "closed";

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
	* Dump the WebRtcTransport.
	*
	* @private
	*
	* @return {Promise}
	*/
	Defer WebRtcTransport::dump()
	{
		logger->debug("dump()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("WebRtcTransport closed"));

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
	* Provide the remote DTLS parameters.
	*
	* @private
	*
	* @param {Object} parameters - Remote DTLS parameters.
	*
	* @return {Promise} Resolves to this->
	*/
	Defer WebRtcTransport::setRemoteDtlsParameters(Json parameters)
	{
		logger->debug("setRemoteDtlsParameters()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("WebRtcTransport closed"));

		return this->_channel->request("transport.setRemoteDtlsParameters", this->_internal, parameters)
		.then([=](Json data)
		{
			logger->debug("\"transport.setRemoteDtlsParameters\" request succeeded");

			// Take the .role field of the response data and append it to our
			// this->_data.dtlsLocalParameters.
			this->_data["dtlsLocalParameters"]["role"] = data["role"];

			return this;
		})
		.fail([=](Error error)
		{
			logger->error(
				"\"transport.setRemoteDtlsParameters\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Set maximum bitrate (in bps).
	*
	* @return {Promise} Resolves to this->
	*/
	Defer WebRtcTransport::setMaxBitrate(int bitrate)
	{
		logger->debug("setMaxBitrate() [bitrate:%d]", bitrate);

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("WebRtcTransport closed"));
		else if (this->direction() != "send")
			return promise::reject(Error("invalid WebRtcTransport direction"));

		return this->_channel->request("transport.setMaxBitrate", this->_internal,
			{
				{ "bitrate",bitrate }
			})
		.then([=]()
		{
			logger->debug("\"transport.setMaxBitrate\" request succeeded");

			return this;
		})
		.fail([=](Error error)
		{
			logger->error("\"transport.setMaxBitrate\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Tell the WebRtcTransport to generate new uFrag and password values.
	*
	* @private
	*
	* @return {Promise} Resolves to this->
	*/
	Defer WebRtcTransport::changeUfragPwd()
	{
		logger->debug("changeUfragPwd()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("WebRtcTransport closed"));

		return this->_channel->request("transport.changeUfragPwd", this->_internal)
			.then([=](Json data)
		{
			logger->debug("\"transport.changeUfragPwd\" request succeeded");

			this->_data["iceLocalParameters"] =
			{
				{"usernameFragment" , data["usernameFragment"]},
				{"password"         , data["password"]},
				{"iceLite"          , this->_data["iceLocalParameters"]["iceLite"]}
			};

			return this;
		})
			.fail([=](Error error)
		{
			logger->error("\"transport.changeUfragPwd\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Get the WebRtcTransport stats.
	*
	* @return {Promise}
	*/
	Defer WebRtcTransport::getStats()
	{
		logger->debug("getStats()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("WebRtcTransport closed"));

		return this->_channel->request("transport.getStats", this->_internal)
			.then([=](Json data)
		{
			logger->debug("\"transport.getStats\" request succeeded");

			return data;
		})
			.fail([=](Error error)
		{
			logger->error("\"transport.getStats\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Enables periodic stats retrieval.
	*
	* @private
	*/
	void WebRtcTransport::enableStats(int interval/* = DEFAULT_STATS_INTERVAL*/)
	{
		logger->debug("enableStats()");

		if (this->_closed)
		{
			logger->error("enableStats() | WebRtcTransport closed");

			return;
		}

		if (this->_statsEnabled)
			return;

		this->_statsEnabled = true;

		this->getStats()
			.then([=](Json stats)
		{
			this->emit("@notify", "transportStats", Json{ {"id" , this->id() }, stats });
		})
			.fail([=](Error error)
		{
			logger->error("\"getStats\" request failed: %s", error.ToString().c_str());
		});

		// Set minimum interval to DEFAULT_STATS_INTERVAL.
		if (interval < DEFAULT_STATS_INTERVAL)
			interval = DEFAULT_STATS_INTERVAL;

		this->_statsInterval = setInterval([=]()
		{
			this->getStats()
				.then([=](Json stats)
			{
				this->emit("@notify", "transportStats", Json{ { "id", this->id() }, stats });
			})
				.fail([=](Error error)
			{
				logger->error("\"getStats\" request failed: %s", error.ToString().c_str());
			});
		}, interval);
	}

	/**
	* Disables periodic stats retrieval.
	*
	* @private
	*/
	void WebRtcTransport::disableStats()
	{
		logger->debug("disableStats()");

		if (this->_closed)
		{
			logger->error("disableStats() | WebRtcTransport closed");

			return;
		}

		if (!this->_statsEnabled)
			return;

		this->_statsEnabled = false;
		clearInterval(this->_statsInterval);
	}

	/**
	* Tell the WebRtcTransport to start mirroring incoming data.
	*
	* @return {Promise} Resolves to this->
	*/
	Defer WebRtcTransport::startMirroring(Json options)
	{
		logger->debug("startMirroring()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("WebRtcTransport closed"));

		return this->_channel->request("transport.startMirroring", this->_internal, options)
			.then([=]()
		{
			logger->debug("\"transport.startMirroring\" request succeeded");

			return this;
		})
			.fail([=](Error error)
		{
			logger->error("\"transport.startMirroring\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Tell the WebRtcTransport to stop mirroring incoming data.
	*
	* @return {Promise} Resolves to this->
	*/
	Defer WebRtcTransport::stopMirroring()
	{
		logger->debug("stopMirroring()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("WebRtcTransport closed"));

		return this->_channel->request("transport.stopMirroring", this->_internal)
			.then([=]()
		{
			logger->debug("\"transport.stopMirroring\" request succeeded");

			return this;
		})
			.fail([=](Error error)
		{
			logger->error("\"transport.stopMirroring\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	void WebRtcTransport::_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		uint32_t transportId = this->_internal["transportId"].get<uint32_t>();
		this->_channel->addEventListener(transportId, this);
	}

	void WebRtcTransport::onEvent(std::string event, Json data)
	{
		if (event == "close")
		{
			this->close(undefined, false);
		}
		else if (event == "iceselectedtuplechange")
		{
			this->_data["iceSelectedTuple"] = data["iceSelectedTuple"];

			// Emit it to the app.
			this->safeEmit("iceselectedtuplechange", data["iceSelectedTuple"]);
		}
		else if (event == "icestatechange")
		{
			this->_data["iceState"] = data["iceState"];

			// Emit it to the app.
			this->safeEmit("icestatechange", data["iceState"]);
		}
		else if (event == "dtlsstatechange")
		{
			this->_data["dtlsState"] = data["dtlsState"];

			if (data["dtlsState"].get<std::string>() == "connected")
				this->_data["dtlsRemoteCert"] = data["dtlsRemoteCert"];

			// Emit it to the app.
			this->safeEmit("dtlsstatechange", data["dtlsState"]);
		}
		else
		{
			logger->error("ignoring unknown event \"%s\"", event);
		}
	}

}

