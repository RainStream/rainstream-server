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
	{
		DLOG(INFO) << "constructor()";

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
		return this->_data["kind"].get<std::string>();
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
		DLOG(INFO) << "close()";

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
				{ "id", this->id() },
				{ "peerName" , this->peer()->name() }
			};

			// Don"t notify "consumerClosed" if the Peer is already closed.
			if (!this->peer()->closed())
				this->doNotify("consumerClosed", data);
		}

		this->doEvent("@close");
		this->doEvent("close");

		// Remove notification subscriptions.
		uint32_t consumerId = this->_internal["consumerId"].get<uint32_t>();
		this->_channel->off(consumerId);

		if (notifyChannel)
		{
			this->_channel->request("consumer.close", this->_internal)
			.then([=]()
			{
				DLOG(INFO) << "\"consumer.close\" request succeeded";
			})
			.fail([=](Error error)
			{
				LOG(ERROR) << "\"consumer.close\" request failed:" << error.ToString();
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
		DLOG(INFO) << "dump()";

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Consumer closed"));

		return this->_channel->request("consumer.dump", this->_internal)
		.then([=](Json data)
		{
			DLOG(INFO) << "\"consumer.dump\" request succeeded";

			return data;
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << "\"consumer.dump\" request failed: " << error.ToString();

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
		DLOG(INFO) << "enable()";

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
			{
				{ "rtpParameters" , this->rtpParameters() }
			})
		.then([=]()
		{
			DLOG(INFO) << "\"consumer.enable\" request succeeded";

			this->_internal = internal;
			this->_transport = transport;
			this->_data = data;

			transport->addEventListener("@close", [=](Json data)
			{
				this->_internal.erase("transportId");
				this->_transport = undefined;

				this->doEvent("unhandled");
			});

			return;
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << "\"consumer.enable\" request failed: " << error.ToString();

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
		DLOG(INFO) << "pause()";

		if (this->_closed)
		{
			LOG(ERROR) << "pause() | Consumer closed";

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

				this->doNotify("consumerPaused", data);
			}
		}

		this->_channel->request("consumer.pause", this->_internal)
		.then([=]()
		{
			DLOG(INFO) << "\"consumer.pause\" request succeeded";
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << "\"consumer.pause\" request failed: " << error.ToString();
		});

		this->doEvent("pause", { { "local", "local" }, { "appData" , appData } });

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
		DLOG(INFO) << "remotePause()";

		if (this->_closed || this->_remotelyPaused)
			return;

		this->_remotelyPaused = true;

		this->_channel->request("consumer.pause", this->_internal)
		.then([=]()
		{
			DLOG(INFO) << "\"consumer.pause\" request succeeded";
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << "\"consumer.pause\" request failed: " << error.ToString();
		});

		this->doEvent("pause", { { "local", "remote" }, { "appData" , appData } });
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
		DLOG(INFO) << "resume()";

		if (this->_closed)
		{
			LOG(ERROR) << "resume() | Consumer closed";

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

				this->doNotify("consumerResumed", data);
			}
		}

		if (!this->_remotelyPaused)
		{
			this->_channel->request("consumer.resume", this->_internal)
			.then([=]()
			{
				DLOG(INFO) << "\"consumer.resume\" request succeeded";
			})
			.fail([=](Error error)
			{
				LOG(ERROR) << "\"consumer.resume\" request failed: " << error.ToString();
			});
		}

		this->doEvent("resume", { { "local", "local" }, { "appData" , appData } });

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
		DLOG(INFO) << "remoteResume()";

		if (this->_closed || !this->_remotelyPaused)
			return;

		this->_remotelyPaused = false;

		if (!this->_locallyPaused)
		{
			this->_channel->request("consumer.resume", this->_internal)
			.then([=]()
			{
				DLOG(INFO) << "\"consumer.resume\" request succeeded";
			})
			.fail([=](Error error)
			{
				LOG(ERROR) << "\"consumer.resume\" request failed: " << error.ToString();
			});
		}

		this->doEvent("resume", { { "local", "remote" }, { "appData" , appData } });
	}

	/**
	* Sets the preferred RTP profile.
	*
	* @param {String} profile
	*/
	void Consumer::setPreferredProfile(std::string profile)
	{
		DLOG(INFO) << "setPreferredProfile() [profile:" << profile << "]";

		if (this->_closed)
		{
			LOG(ERROR) << "setPreferredProfile() | Consumer closed";

			return;
		}
		else if (profile == this->_preferredProfile)
		{
			return;
		}
		else if (!PROFILES.count(profile))
		{
			LOG(ERROR) << "setPreferredProfile() | invalid profile " << profile;

			return;
		}

		this->_channel->request("consumer.setPreferredProfile", this->_internal,
			{
				{ "profile" , profile }
			})
		.then([=]()
		{
			DLOG(INFO) << "\"consumer.setPreferredProfile\" request succeeded";

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

				this->doNotify("consumerPreferredProfileSet", data);
			}
		})
		.fail([=](Error error)
		{
			LOG(ERROR) <<
				"\"consumer.setPreferredProfile\" request failed: " << error.ToString();
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
		DLOG(INFO) << "remoteSetPreferredProfile() [profile:" << profile << "]";

		if (this->_closed || profile == this->_preferredProfile)
			return;

		this->_channel->request("consumer.setPreferredProfile", this->_internal,
			{
				{ "profile" , profile }
			})
		.then([=]()
		{
			DLOG(INFO) << "\"consumer.setPreferredProfile\" request succeeded";

			this->_preferredProfile = profile;
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << 
				"\"consumer.setPreferredProfile\" request failed:" << error.ToString();
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
		DLOG(INFO) << "setEncodingPreferences() [preferences:" << preferences << "]";

		if (this->_closed)
		{
			LOG(ERROR) << "setEncodingPreferences() | Consumer closed";

			return;
		}

		this->_channel->request(
			"consumer.setEncodingPreferences", this->_internal, 
			{
				{ "preferences" , preferences }
			})
		.then([=]()
		{
			DLOG(INFO) << "\"consumer.setEncodingPreferences\" request succeeded";
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << 
				"\"consumer.setEncodingPreferences\" request failed:" << error.ToString();
		});
	}

	/**
	* Request a key frame to the source.
	*/
	void Consumer::requestKeyFrame()
	{
		DLOG(INFO) << "requestKeyFrame()";

		if (this->_closed || !this->enabled() || this->paused())
			return;

		if (this->kind() != "video")
			return;

		this->_channel->request("consumer.requestKeyFrame", this->_internal)
		.then([=]()
		{
			DLOG(INFO) << "\"consumer.requestKeyFrame\" request succeeded";
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << "\"consumer.requestKeyFrame\" request failed:" << error.ToString();
		});
	}

	/**
	* Get the Consumer stats.
	*
	* @return {Promise}
	*/
	Defer Consumer::getStats()
	{
		DLOG(INFO) << "getStats()";

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Consumer closed"));

		return this->_channel->request("consumer.getStats", this->_internal)
			.then([=](Json data)
		{
			DLOG(INFO) << "\"consumer.getStats\" request succeeded";

			return data;
		})
			.fail([=](Error error)
		{
			LOG(ERROR) << "\"consumer.getStats\" request failed:" << error.ToString();

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
		DLOG(INFO) << "enableStats()";

		if (this->_closed)
		{
			LOG(ERROR) << "enableStats() | Consumer closed";

			return;
		}

		if (!this->hasPeer())
		{
			LOG(ERROR) << 
				"enableStats() | cannot enable stats on a Consumer without Peer";

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

				this->doNotify("consumerStats", data);
			})
				.fail([=](Error error)
			{
				LOG(ERROR) << "\"getStats\" request failed:" << error.ToString();
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

				this->doNotify("consumerStats", data);
			})
				.fail([=](Error error)
			{
				LOG(ERROR) << "\"getStats\" request failed:" << error.ToString();
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
		DLOG(INFO) << "disableStats()";

		if (this->_closed)
		{
			LOG(ERROR) << "disableStats() | Consumer closed";

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

					this->doNotify("consumerPaused", data2);
				}
			}

			this->doEvent("pause", data);
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

					this->doNotify("consumerResumed", data2);
				}
			}

			this->doEvent("resume", data);
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

					this->doNotify("consumerEffectiveProfileChanged", data2);
				}
			}

			this->doEvent("effectiveprofilechange", data);
		}
		else
		{
			LOG(ERROR) << "ignoring unknown event" << event;
		}
	}

}
