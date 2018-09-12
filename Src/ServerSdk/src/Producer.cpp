#include "RainStream.hpp"
#include "Producer.hpp"
#include "Channel.hpp"
#include "Peer.hpp"
#include "Logger.hpp"
#include <set>

namespace rs
{
	const std::set<std::string> PROFILES = { "default", "low", "medium", "high" };

	Producer::Producer(Peer* peer, WebRtcTransport*transport, const Json& internal, const Json& data, Channel* channel, const Json& options)
		: EnhancedEventEmitter()
		, logger(new Logger("Producer"))
	{

		logger->debug("constructor()");

		// Closed flag.
		this->_closed = false;

		// Internal data.
		// - .routerId
		// - .producerId
		// - .transportId
		this->_internal = internal;

		// Producer data.
		// - .kind
		// - .peer
		// - .transport
		// - .rtpParameters
		// - .consumableRtpParameters
		this->_data = data;

		// Channel instance.
		this->_channel = channel;

		this->_peer = peer;

		this->_transport = transport;

		// App data.
		this->_appData = undefined;

		// Locally paused flag.
		// @type {bool}
		this->_locallyPaused = false;

		// Remotely paused flag.
		// @type {bool}
		this->_remotelyPaused = options["remotelyPaused"].get<bool>();

		// Periodic stats flag.
		// @type {bool}
		this->_statsEnabled = false;

		// Periodic stats interval identifier.
		// @type {bool}
		this->_statsInterval = NULL;

		// Preferred profile.
		// @type {String}
		this->_preferredProfile = "default";

		this->_handleTransportEvents();

		this->_handleWorkerNotifications();
	}

	uint32_t Producer::id()
	{
		return this->_internal["producerId"];
	}

	bool Producer::closed()
	{
		return this->_closed;
	}

	Json Producer::appData()
	{
		return this->_appData;
	}

	void Producer::appData(Json appData)
	{
		this->_appData = appData;
	}

	std::string Producer::kind()
	{
		return this->_data["kind"];
	}

	Peer* Producer::peer()
	{
		return this->_peer;
	}

	WebRtcTransport* Producer::transport()
	{
		return this->_transport;
	}

	Json Producer::rtpParameters()
	{
		return this->_data["rtpParameters"];
	}

	Json Producer::consumableRtpParameters()
	{
		return this->_data["consumableRtpParameters"];
	}

	/**
	* Whether the Producer is locally paused.
	*
	* @return {bool}
	*/
	bool Producer::locallyPaused()
	{
		return this->_locallyPaused;
	}

	/**
	* Whether the Producer is remotely paused.
	*
	* @return {bool}
	*/
	bool Producer::remotelyPaused()
	{
		return this->_remotelyPaused;
	}

	/**
	* Whether the Producer is paused.
	*
	* @return {bool}
	*/
	bool Producer::paused()
	{
		return this->_locallyPaused || this->_remotelyPaused;
	}

	/**
	* The preferred profile.
	*
	* @type {String}
	*/
	std::string Producer::preferredProfile()
	{
		return this->_preferredProfile;
	}

	/**
	* Close the Producer.
	*
	* @param {Any} [appData] - App custom data.
	* @param {bool} [notifyChannel=true] - Private.
	*/
	void Producer::close(Json appData, bool notifyChannel/* = true*/)
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

		// If our Transport is still alive, it means that the app really wants to close
		// the client-side Producer. Otherwise, if the Transport is closed the client-side
		// Producer will get "unhandled", so don"t close it from here.
		if (!this->transport()->closed())
		{
			Json data =
			{
				{ "id", this->id() },
				{ "peerName" , this->peer()->name() },
				{ "appData", appData }
			};

			this->emit("@notify", "producerClosed", data);
		}

		this->emit("@close");
		this->safeEmit("close", "local", appData);

		this->_destroy(notifyChannel);
	}

	/**
	* The remote Producer was closed.
	* Invoked via remote notification.
	*
	* @private
	*
	* @param {Any} [appData] - App custom data.
	* @param {bool} [notifyChannel=true] - Private.
	*/
	void Producer::remoteClose(Json appData /*= Json::object()*/, bool notifyChannel/* = true*/)
	{
		logger->debug("remoteClose()");

		if (this->closed())
			return;

		this->_closed = true;

		this->emit("@close");
		this->safeEmit("close", "remote", appData);

		this->_destroy(notifyChannel);
	}

	void Producer::_destroy(bool notifyChannel /*= true*/)
	{
		uint32_t producerId = this->_internal["producerId"].get<uint32_t>();
		// Remove notification subscriptions.
		this->_channel->off(producerId);

		if (notifyChannel)
		{
			this->_channel->request("producer.close", this->_internal)
				.then([=]()
			{
				logger->debug("\"producer.close\" request succeeded");
			})
				.fail([=](Error error)
			{
				logger->error("\"producer.close\" request failed: %s", error.ToString().c_str());
			});
		}
	}

	/**
	* Dump the Producer.
	*
	* @private
	*
	* @return {Promise}
	*/
	Defer Producer::dump()
	{
		logger->debug("dump()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Producer closed"));

		return this->_channel->request("producer.dump", this->_internal)
			.then([=](Json data)
		{
			logger->debug("\"producer.dump\" request succeeded");

			return data;
		})
			.fail([=](Error error)
		{
			logger->error("\"producer.dump\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Pauses receiving media.
	*
	* @param {Any} [appData] - App custom data.
	*
	* @return {bool} true if paused.
	*/
	bool Producer::pause(Json appData)
	{
		logger->debug("pause()");

		if (this->_closed)
		{
			logger->error("pause() | Producer closed");

			return false;
		}
		else if (this->_locallyPaused)
		{
			return true;
		}

		this->_locallyPaused = true;

		Json data =
		{
			{ "id", this->id() },
			{ "peerName" , this->peer()->name() },
			{ "appData", appData }
		};

		this->emit("@notify", "producerPaused", data);

		this->_channel->request("producer.pause", this->_internal)
			.then([=]()
		{
			logger->debug("\"producer.pause\" request succeeded");
		})
			.fail([=](Error error)
		{
			logger->error("\"producer.pause\" request failed: %s", error.ToString().c_str());
		});

		this->safeEmit("pause", "local", appData);

		// Return true if really paused.
		return this->paused();
	}

	/**
	* The remote Producer was paused.
	* Invoked via remote notification.
	*
	* @private
	*
	* @param {Any} [appData] - App custom data.
	*/
	void Producer::remotePause(Json appData)
	{
		logger->debug("remotePause()");

		if (this->_closed || this->_remotelyPaused)
			return;

		this->_remotelyPaused = true;

		this->_channel->request("producer.pause", this->_internal)
			.then([=]()
		{
			logger->debug("\"producer.pause\" request succeeded");
		})
			.fail([=](Error error)
		{
			logger->error("\"producer.pause\" request failed: %s", error.ToString().c_str());
		});

		this->safeEmit("pause", "remote", appData);
	}

	/**
	* Resumes receiving media.
	*
	* @param {Any} [appData] - App custom data.
	*
	* @return {bool} true if not paused.
	*/
	bool Producer::resume(Json appData)
	{
		logger->debug("resume()");

		if (this->_closed)
		{
			logger->error("resume() | Producer closed");

			return false;
		}
		else if (!this->_locallyPaused)
		{
			return true;
		}

		this->_locallyPaused = false;

		Json data =
		{
			{ "id", this->id() },
			{ "peerName" , this->peer()->name() },
			{ "appData", appData }
		};

		this->emit("@notify", "producerResumed", data);

		if (!this->_remotelyPaused)
		{
			this->_channel->request("producer.resume", this->_internal)
				.then([=]()
			{
				logger->debug("\"producer.resume\" request succeeded");
			})
				.fail([=](Error error)
			{
				logger->error("\"producer.resume\" request failed: %s", error.ToString().c_str());
			});
		}

		this->safeEmit("resume", "local", appData);

		// Return true if not paused.
		return !this->paused();
	}

	/**
	* The remote Producer was resumed.
	* Invoked via remote notification.
	*
	* @private
	*
	* @param {Any} [appData] - App custom data.
	*/
	void Producer::remoteResume(Json appData)
	{
		logger->debug("remoteResume()");

		if (this->_closed || !this->_remotelyPaused)
			return;

		this->_remotelyPaused = false;

		if (!this->_locallyPaused)
		{
			this->_channel->request("producer.resume", this->_internal)
				.then([=]()
			{
				logger->debug("\"producer.resume\" request succeeded");
			})
				.fail([=](Error error)
			{
				logger->error("\"producer.resume\" request failed: %s", error.ToString().c_str());
			});
		}

		this->safeEmit("resume", "remote", appData);
	}

	/**
	* Sets the preferred RTP profile.
	*
	* @param {String} profile
	*/
	void Producer::setPreferredProfile(std::string profile)
	{
		logger->debug("setPreferredProfile() [profile:%s]", profile);

		if (this->_closed)
		{
			logger->error("setPreferredProfile() | Producer closed");

			return;
		}
		else if (profile == this->_preferredProfile)
		{
			return;
		}
		else if (!PROFILES.count(profile))
		{
			logger->error("setPreferredProfile() | invalid profile \"%s\"", profile.c_str());

			return;
		}

		this->_channel->request("producer.setPreferredProfile", this->_internal,
			{
				profile
			})
			.then([=]()
		{
			logger->debug("\"producer.setPreferredProfile\" request succeeded");

			this->_preferredProfile = profile;
		})
			.fail([=](Error error)
		{
			logger->error(
				"\"producer.setPreferredProfile\" request failed: %s", error.ToString().c_str());
		});
	}

	/**
	* Change receiving RTP parameters.
	*
	* @private
	*
	* @param {RTCRtpParameters} rtpParameters - New RTP parameters.
	*
	* @return {Promise}
	*/
	Defer Producer::updateRtpParameters(Json rtpParameters)
	{
		logger->debug("updateRtpParameters()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Producer closed"));

		return this->_channel->request("producer.updateRtpParameters", this->_internal,
			{
				rtpParameters
			})
			.then([=]()
		{
			logger->debug("\"producer.updateRtpParameters\" request succeeded");

			this->_data["rtpParameters"] = rtpParameters;
		})
			.fail([=](Error error)
		{
			logger->error(
				"\"producer.updateRtpParameters\" request failed: %s", error.ToString().c_str());
		});
	}

	/**
	* Get the Producer stats.
	*
	* @return {Promise}
	*/
	Defer Producer::getStats()
	{
		logger->debug("getStats()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Producer closed"));

		return this->_channel->request("producer.getStats", this->_internal)
			.then([=](Json data)
		{
			logger->debug("\"producer.getStats\" request succeeded");

			return data;
		})
			.fail([=](Error error)
		{
			logger->error("\"producer.getStats\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Enables periodic stats retrieval.
	*
	* @private
	*/
	void Producer::enableStats(int interval /*= DEFAULT_STATS_INTERVAL*/)
	{
		logger->debug("enableStats()");

		if (this->_closed)
		{
			logger->error("enableStats() | Producer closed");

			return;
		}

		if (this->_statsEnabled)
			return;

		this->_statsEnabled = true;

		if (!this->paused())
		{
			this->getStats()
				.then([=](Json stats)
			{
				this->emit("@notify", "producerStats", Json{ {"id" , this->id() }, stats });
			})
				.fail([=](Error error)
			{
				logger->error("\"getStats\" failed: %s", error.ToString().c_str());
			});
		}

		// Set minimum interval to DEFAULT_STATS_INTERVAL.
		if (interval < DEFAULT_STATS_INTERVAL)
			interval = DEFAULT_STATS_INTERVAL;

		this->_statsInterval = setInterval([=]()
		{
			if (this->paused())
				return;

			this->getStats()
				.then([=](Json stats)
			{
				this->emit("@notify", "producerStats", Json{ { "id", this->id() }, stats });
			})
				.fail([=](Error error)
			{
				logger->error("\"getStats\" failed: %s", error.ToString().c_str());
			});
		}, interval);
	}

	/**
	* Disables periodic stats retrieval.
	*
	* @private
	*/
	void Producer::disableStats()
	{
		logger->debug("disableStats()");

		if (this->_closed)
		{
			logger->error("disableStats() | Producer closed");

			return;
		}

		if (!this->_statsEnabled)
			return;

		this->_statsEnabled = false;
		clearInterval(this->_statsInterval);
	}

	void Producer::_handleTransportEvents()
	{
		// On closure, the worker Transport closes all its Producers and the client
		// side gets producer.on("unhandled").
		this->transport()->on("@close", [=]()
		{
			if (!this->_closed)
				this->close(undefined, false);
		});
	}

	void Producer::_handleWorkerNotifications()
	{
		uint32_t producerId = this->_internal["producerId"].get<uint32_t>();
		// Subscribe to notifications.
		this->_channel->addEventListener(producerId, this);
	}

	void Producer::onEvent(std::string event, Json data)
	{
		if (event == "close")
		{
			this->close(undefined, false);
		}
		else
		{
			logger->error("ignoring unknown event \"%s\"", event);
		}
	}

}
