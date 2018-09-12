#include "RainStream.hpp"
#include "Consumer.hpp"
#include "Producer.hpp"
#include "Channel.hpp"
#include "WebRtcTransport.hpp"
#include "Peer.hpp"
#include "Logger.hpp"
#include <set>

namespace rs
{

	const std::set<std::string> PROFILES = { "default", "low", "medium", "high" };

	Consumer::Consumer(Peer* peer, Producer* source, const Json& internal, const Json& data, Channel* channel)
		: EnhancedEventEmitter()
		, logger(new Logger("Consumer"))
	{
		logger->debug("constructor()");

		// Closed flag.
		this->_closed = false;

		this->_internal = internal;

		this->_data = data;

		// Channel instance.
		this->_channel = channel;

		this->_peer = peer;

		this->_source = source;

		// App data.
		this->_appData = undefined;

		// Locally paused flag.
		// @type {bool}
		this->_locallyPaused = false;

		// Remotely paused flag.
		// @type {bool}
		this->_remotelyPaused = false;

		// Source paused flag.
		// @type {bool}
		this->_sourcePaused = false;

		// Periodic stats flag.
		// @type {bool}
		this->_statsEnabled = false;

		// Periodic stats interval identifier.
		// @type {bool}
		this->_statsInterval = NULL;

		// Preferred profile.
		// @type {String}
		this->_preferredProfile = "default";

		// Effective profile.
		// @type {String}
		this->_effectiveProfile = "";

		this->_handleWorkerNotifications();
	}

	uint32_t Consumer::id()
	{
		return this->_internal["consumerId"].get<uint32_t>();
	}

	bool Consumer::closed()
	{
		return this->_closed;
	}

	Json Consumer::appData()
	{
		return this->_appData;
	}

	void Consumer::appData(const Json& appData)
	{
		this->_appData = appData;
	}

	std::string Consumer::kind()
	{
		return this->_data["kind"];
	}

	Peer* Consumer::peer()
	{
		return this->_peer;
	}

	WebRtcTransport* Consumer::transport()
	{
		return this->_transport;
	}

	Json Consumer::rtpParameters()
	{
		return this->_data["rtpParameters"];
	}

	Producer* Consumer::source()
	{
		return this->_source;
	}

	bool Consumer::enabled()
	{
		return this->_transport;
	}

	/**
	* Whether the Consumer is locally paused.
	*
	* @return {bool}
	*/
	bool Consumer::locallyPaused()
	{
		return this->_locallyPaused;
	}

	/**
	* Whether the Consumer is remotely paused.
	*
	* @return {bool}
	*/
	bool Consumer::remotelyPaused()
	{
		return this->_remotelyPaused;
	}

	/**
	* Whether the source (Producer) is paused.
	*
	* @return {bool}
	*/
	bool Consumer::sourcePaused()
	{
		return this->_sourcePaused;
	}

	/**
	* Whether the Consumer is paused.
	*
	* @return {bool}
	*/
	bool Consumer::paused()
	{
		return this->_locallyPaused || this->_remotelyPaused || this->_sourcePaused;
	}

	/**
	* The preferred profile.
	*
	* @type {String}
	*/
	std::string Consumer::preferredProfile()
	{
		return this->_preferredProfile;
	}

	/**
	* The effective profile.
	*
	* @type {String}
	*/
	std::string Consumer::effectiveProfile()
	{
		return this->_effectiveProfile;
	}

	/**
	* Close the Consumer.
	*
	* @private
	*
	* @param {bool} [notifyChannel=true] - Private.
	*/
	void Consumer::close(bool notifyChannel/* = true*/)
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

		// Notify if it has a Peer.
		if (this->hasPeer())
		{
			Json data =
			{
				{"id", this->id() },
				{"peerName" , this->peer()->name() }
			};

			// Don"t notify "consumerClosed" if the Peer is already closed.
			if (!this->peer()->closed())
				this->emit("@notify", "consumerClosed", data);
		}

		this->emit("@close");
		this->safeEmit("close");

		// Remove notification subscriptions.
		uint32_t consumerId = this->_internal["consumerId"].get<uint32_t>();
		this->_channel->off(consumerId);

		if (notifyChannel)
		{
			this->_channel->request("consumer.close", this->_internal)
				.then([=]()
			{
				logger->debug("\"consumer.close\" request succeeded");
			})
				.fail([=](Error error)
			{
				logger->error("\"consumer.close\" request failed: %s", error.ToString().c_str());
			});
		}
	}

	/**
	* Dump the Consumer.
	*
	* @private
	*
	* @return {Promise}
	*/
	Defer Consumer::Consumer::dump()
	{
		logger->debug("dump()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Consumer closed"));

		return this->_channel->request("consumer.dump", this->_internal)
			.then([=](Json data)
		{
			logger->debug("\"consumer.dump\" request succeeded");

			return data;
		})
			.fail([=](Error error)
		{
			logger->error("\"consumer.dump\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	Json Consumer::toJson()
	{
		Json json =
		{
			{ "id", this->id() },
			{ "kind", this->kind() },
			{ "rtpParameters", this->rtpParameters() },
			{ "paused", this->paused() },
			{ "preferredProfile", this->preferredProfile() },
			{ "effectiveProfile", this->effectiveProfile() },
			{ "appData", this->appData() }
		};

		if (this->hasPeer())
			json["peerName"] = this->peer()->name();

		return json;
	}

	/**
	* Whether this Consumer belongs to a Peer.
	*
	* @private
	*
	* @return {bool}
	*/
	bool Consumer::hasPeer()
	{
		return bool(this->peer());
	}

	/**
	* Enable the Consumer for sending media.
	*
	* @private
	*
	* @param {Transport} transport
	* @param {bool} remotelyPaused
	* @param {String} preferredProfile
	*/
	Defer Consumer::enable(WebRtcTransport* transport, bool remotelyPaused, std::string preferredProfile)
	{
		logger->debug("enable()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Consumer closed"));
		else if (this->enabled())
			return promise::reject(errors::InvalidStateError("Consumer already enabled"));

		if (remotelyPaused)
			this->remotePause();

		if (!preferredProfile.empty())
			this->remoteSetPreferredProfile(preferredProfile);

		Json internal = this->_internal;
		internal = Object::assign(internal,
		{
			{ "transportId" , transport->id() }
		});

		Json data = this->_data;

		return this->_channel->request("consumer.enable", internal,
			Json{
				{ "rtpParameters" , this->rtpParameters() }
			})
			.then([=]()
		{
			logger->debug("\"consumer.enable\" request succeeded");

			this->_internal = internal;
			this->_transport = transport;
			this->_data = data;

			transport->on("@close", [=]()
			{
				this->_internal["transportId"] = undefined;
				this->_transport = undefined;
			});

			return;
		})
			.fail([=](Error error)
		{
			logger->error("\"consumer.enable\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Pauses sending media.
	*
	* @param {Any} [appData] - App custom data.
	*
	* @return {bool} true if paused.
	*/
	bool Consumer::pause(Json appData)
	{
		logger->debug("pause()");

		if (this->_closed)
		{
			logger->error("pause() | Consumer closed");

			return false;
		}
		else if (this->_locallyPaused)
		{
			return true;
		}

		this->_locallyPaused = true;

		// Notify if it has a Peer.
		if (this->hasPeer())
		{
			if (this->enabled() && !this->_sourcePaused)
			{
				Json data =
				{
					{ "id", this->id() },
					{ "peerName" , this->peer()->name() },
					{ "appData", appData}
				};

				this->emit("@notify", "consumerPaused", data);
			}
		}

		this->_channel->request("consumer.pause", this->_internal)
			.then([=]()
		{
			logger->debug("\"consumer.pause\" request succeeded");
		})
			.fail([=](Error error)
		{
			logger->error("\"consumer.pause\" request failed: %s", error.ToString().c_str());
		});

		this->safeEmit("pause", "local", appData);

		// Return true if really paused.
		return this->paused();
	}

	/**
	* The remote Consumer was paused.
	* Invoked via remote notification.
	*
	* @private
	*
	* @param {Any} [appData] - App custom data.
	*/
	void Consumer::remotePause(const Json& appData/* = Json::object()*/)
	{
		logger->debug("remotePause()");

		if (this->_closed || this->_remotelyPaused)
			return;

		this->_remotelyPaused = true;

		this->_channel->request("consumer.pause", this->_internal)
			.then([=]()
		{
			logger->debug("\"consumer.pause\" request succeeded");
		})
			.fail([=](Error error)
		{
			logger->error("\"consumer.pause\" request failed: %s", error.ToString().c_str());
		});

		this->safeEmit("pause", "remote", appData);
	}

	/**
	* Resumes sending media.
	*
	* @param {Any} [appData] - App custom data.
	*
	* @return {bool} true if not paused.
	*/
	bool Consumer::resume(const Json& appData)
	{
		logger->debug("resume()");

		if (this->_closed)
		{
			logger->error("resume() | Consumer closed");

			return false;
		}
		else if (!this->_locallyPaused)
		{
			return true;
		}

		this->_locallyPaused = false;

		// Notify if it has a Peer.
		if (this->hasPeer())
		{
			if (this->enabled() && !this->_sourcePaused)
			{
				Json data =
				{
					{ "id", this->id() },
					{ "peerName" , this->peer()->name() },
					{ "appData", appData }
				};

				this->emit("@notify", "consumerResumed", data);
			}
		}

		if (!this->_remotelyPaused)
		{
			this->_channel->request("consumer.resume", this->_internal)
				.then([=]()
			{
				logger->debug("\"consumer.resume\" request succeeded");
			})
				.fail([=](Error error)
			{
				logger->error("\"consumer.resume\" request failed: %s", error.ToString().c_str());
			});
		}

		this->safeEmit("resume", "local", appData);

		// Return true if not paused.
		return !this->paused();
	}

	/**
	* The remote Consumer was resumed.
	* Invoked via remote notification.
	*
	* @private
	*
	* @param {Any} [appData] - App custom data.
	*/
	void Consumer::remoteResume(const Json& appData)
	{
		logger->debug("remoteResume()");

		if (this->_closed || !this->_remotelyPaused)
			return;

		this->_remotelyPaused = false;

		if (!this->_locallyPaused)
		{
			this->_channel->request("consumer.resume", this->_internal)
				.then([=]()
			{
				logger->debug("\"consumer.resume\" request succeeded");
			})
				.fail([=](Error error)
			{
				logger->error("\"consumer.resume\" request failed: %s", error.ToString().c_str());
			});
		}

		this->safeEmit("resume", "remote", appData);
	}

	/**
	* Sets the preferred RTP profile.
	*
	* @param {String} profile
	*/
	void Consumer::setPreferredProfile(std::string profile)
	{
		logger->debug("setPreferredProfile() [profile:%s]", profile.c_str());

		if (this->_closed)
		{
			logger->error("setPreferredProfile() | Consumer closed");

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

		this->_channel->request("consumer.setPreferredProfile", this->_internal,
			{
				{ "profile" , profile }
			})
			.then([=]()
		{
			logger->debug("\"consumer.setPreferredProfile\" request succeeded");

			this->_preferredProfile = profile;

			// Notify if it has a Peer.
			if (this->hasPeer() && !this->_closed)
			{
				Json data =
				{
					{ "id" , this->id() },
					{ "peerName" , this->peer()->name()},
					{ "profile" , profile}
				};

				this->emit("@notify", "consumerPreferredProfileSet", data);
			}
		})
			.fail([=](Error error)
		{
			logger->error(
				"\"consumer.setPreferredProfile\" request failed: %s", error.ToString().c_str());
		});
	}

	/**
	* Preferred receiving profile was set on my remote Consumer.
	*
	* @private
	*
	* @param {String} profile
	*/
	void Consumer::remoteSetPreferredProfile(std::string profile)
	{
		logger->debug("remoteSetPreferredProfile() [profile:%s]", profile.c_str());

		if (this->_closed || profile == this->_preferredProfile)
			return;

		this->_channel->request("consumer.setPreferredProfile", this->_internal,
			{
				{ "profile" , profile }
			})
			.then([=]()
		{
			logger->debug("\"consumer.setPreferredProfile\" request succeeded");

			this->_preferredProfile = profile;
		})
			.fail([=](Error error)
		{
			logger->error(
				"\"consumer.setPreferredProfile\" request failed: %s", error.ToString().c_str());
		});
	}

	/**
	* Sets the encoding preferences.
	* Only for testing.
	*
	* @private
	*
	* @param {String} profile
	*/
	void Consumer::setEncodingPreferences(std::string preferences)
	{
		logger->debug("setEncodingPreferences() [preferences:%s]", preferences.c_str());

		if (this->_closed)
		{
			logger->error("setEncodingPreferences() | Consumer closed");

			return;
		}

		this->_channel->request(
			"consumer.setEncodingPreferences", this->_internal, preferences)
			.then([=]()
		{
			logger->debug("\"consumer.setEncodingPreferences\" request succeeded");
		})
			.fail([=](Error error)
		{
			logger->error(
				"\"consumer.setEncodingPreferences\" request failed: %s", error.ToString().c_str());
		});
	}

	/**
	* Request a key frame to the source.
	*/
	void Consumer::requestKeyFrame()
	{
		logger->debug("requestKeyFrame()");

		if (this->_closed || !this->enabled() || this->paused())
			return;

		if (this->kind() != "video")
			return;

		this->_channel->request("consumer.requestKeyFrame", this->_internal)
			.then([=]()
		{
			logger->debug("\"consumer.requestKeyFrame\" request succeeded");
		})
			.fail([=](Error error)
		{
			logger->error("\"consumer.requestKeyFrame\" request failed: %s", error.ToString().c_str());
		});
	}

	/**
	* Get the Consumer stats.
	*
	* @return {Promise}
	*/
	Defer Consumer::getStats()
	{
		logger->debug("getStats()");

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Consumer closed"));

		return this->_channel->request("consumer.getStats", this->_internal)
			.then([=](Json data)
		{
			logger->debug("\"consumer.getStats\" request succeeded");

			return data;
		})
			.fail([=](Error error)
		{
			logger->error("\"consumer.getStats\" request failed: %s", error.ToString().c_str());

			throw error;
		});
	}

	/**
	* Enables periodic stats retrieval.
	*
	* @private
	*/
	void Consumer::enableStats(int interval/* = DEFAULT_STATS_INTERVAL*/)
	{
		logger->debug("enableStats()");

		if (this->_closed)
		{
			logger->error("enableStats() | Consumer closed");

			return;
		}

		if (!this->hasPeer())
		{
			logger->error(
				"enableStats() | cannot enable stats on a Consumer without Peer");

			return;
		}

		if (this->_statsEnabled)
			return;

		this->_statsEnabled = true;

		if (this->enabled() && !this->paused())
		{
			this->getStats()
				.then([=](Json stats)
			{
				Json data =
				{
					{ "id" , this->id() },
					{ "peerName" , this->peer()->name() },
					{ stats }
				};

				this->emit("@notify", "consumerStats", data);
			})
				.fail([=](Error error)
			{
				logger->error("\"getStats\" request failed: %s", error.ToString().c_str());
			});
		}

		// Set minimum interval to DEFAULT_STATS_INTERVAL.
		if (interval < DEFAULT_STATS_INTERVAL)
			interval = DEFAULT_STATS_INTERVAL;

		this->_statsInterval = setInterval([=]()
		{
			if (!this->enabled() || this->paused())
				return;

			this->getStats()
				.then([=](Json stats)
			{
				Json data =
				{
				{ "id" , this->id() },
				{ "peerName" , this->peer()->name() },
				{ stats }
				};

				this->emit("@notify", "consumerStats", data);
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
	void Consumer::disableStats()
	{
		logger->debug("disableStats()");

		if (this->_closed)
		{
			logger->error("disableStats() | Consumer closed");

			return;
		}

		if (!this->_statsEnabled)
			return;

		this->_statsEnabled = false;
		clearInterval(this->_statsInterval);
	}

	void Consumer::_handleWorkerNotifications()
	{
		uint32_t consumerId = this->_internal["consumerId"].get<uint32_t>();
		// Subscribe to notifications.
		this->_channel->addEventListener(consumerId, this);
	}

	void Consumer::onEvent(std::string event, Json data)
	{
		if (event == "close")
		{
			this->close(false);
		}
		else if (event == "sourcepaused")
		{
			if (this->_sourcePaused)
				return;

			this->_sourcePaused = true;

			// Notify if it has a Peer.
			if (this->hasPeer())
			{
				if (this->enabled() && !this->_locallyPaused)
				{
					Json data2 =
					{
						{ "id" , this->id() },
						{ "peerName" , this->peer()->name()}
					};

					this->emit("@notify", "consumerPaused", data2);
				}
			}

			this->safeEmit("pause", "source");
		}
		else if (event == "sourceresumed")
		{
			if (!this->_sourcePaused)
				return;

			this->_sourcePaused = false;

			// Notify if it has a Peer.
			if (this->hasPeer())
			{
				if (this->enabled() && !this->_locallyPaused)
				{
					Json data2 =
					{
						{ "id" , this->id() },
						{ "peerName" , this->peer()->name() }
					};

					this->emit("@notify", "consumerResumed", data2);
				}
			}

			this->safeEmit("resume", "source");
		}
		else if (event == "effectiveprofilechange")
		{
			std::string profile = data["profile"].get<std::string>();

			if (this->_effectiveProfile == profile)
				return;

			this->_effectiveProfile = profile;

			// Notify if it has a Peer.
			if (this->hasPeer())
			{
				if (this->enabled())
				{
					Json data2 =
					{
					{ "id" , this->id() },
					{ "peerName" , this->peer()->name() },
					{ "profile" , this->_effectiveProfile }
					};

					//						this->emit("@notify", "consumerEffectiveProfileChanged", data2);
				}
			}

			this->safeEmit("effectiveprofilechange", this->_effectiveProfile);
		}
		else
		{
			logger->error("ignoring unknown event \"%s\"", event);
		}
	}

}
