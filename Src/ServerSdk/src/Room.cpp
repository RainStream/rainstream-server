#include "RainStream.hpp"
#include "Room.hpp"
#include "Logger.hpp"
#include "Consumer.hpp"
#include "Channel.hpp"
#include "plugins/ActiveSpeakerDetector.hpp"

namespace rs
{

	Room::Room(const Json& internal, const Json& data, Channel* channel)
	{
		DLOG(INFO) << "constructor()";

		// Closed flag.
		this->_closed = false;

		// Internal data.
		// - .routerId
		this->_internal = internal;

		// Room data.
		// - .rtpCapabilities
		this->_data = data;

		// Channel instance.
		this->_channel = channel;

		this->_handleWorkerNotifications();
	}

	uint32_t Room::id()
	{
		return this->_internal["routerId"].get<uint32_t>();
	}

	bool Room::closed()
	{
		return this->_closed;
	}

	Json Room::rtpCapabilities()
	{
		return this->_data["rtpCapabilities"];
	}

	Peers Room::peers()
	{
		return this->_peers;
	}

	/**
	* Close the Room.
	*
	* @param {Any} [appData] - App custom data.
	* @param {bool} [notifyChannel=true] - Private.
	*/
	void Room::close(const Json& appData, bool notifyChannel)
	{
		DLOG(INFO) << "close()";

		if (this->_closed)
			return;

		this->_closed = true;

		this->doEvent("@close");
		this->doEvent("close", appData);

		// Remove notification subscriptions.
		uint32_t routerId = this->_internal["routerId"].get<uint32_t>();
		this->_channel->off(routerId);

		// Close every Peer.
		for (auto peer : this->_peers)
		{
			peer.second->close(undefined, false);
		}

		if (notifyChannel)
		{
			this->_channel->request("router.close", this->_internal)
			.then([=]()
			{
				DLOG(INFO) << "\"router.close\" request succeeded";
			})
			.fail([=](Error error)
			{
				LOG(ERROR) << "\"router.close\" request failed: " << error.ToString();
			});
		}
	}

	Defer Room::dump()
	{
		DLOG(INFO) << "dump()";

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Room closed"));

		// TODO: Should we aggregate Peers info?

		return this->_channel->request("router.dump", this->_internal)
			.then([=](Json data)
		{
			DLOG(INFO) << "\"router.dump\" request succeeded";

			return data;
		})
			.fail([=](Error error)
		{
			LOG(ERROR) << "\"router.dump\" request failed: %s", error.ToString();

			throw error;
		});
	}

	Peer* Room::getPeerByName(std::string name)
	{
		if (this->_peers.count(name))
		{
			return this->_peers[name];
		}
		return nullptr;
	}

	Producer* Room::getProducerById(uint32_t id)
	{
		if (this->_producers.count(id))
		{
			return this->_producers[id];
		}
		return nullptr;
	}

	Defer Room::receiveRequest(const Json& request)
	{
		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Room closed"));
		else if (!request.is_object())
			return promise::reject(TypeError("wrong request Object"));
		else if (request.count("notification"))
			return promise::reject(TypeError("not a request"));
		else if (!request["method"].is_string())
			return promise::reject(TypeError("wrong/missing request method"));

		std::string method = request["method"].get<std::string>();

		DLOG(INFO) << "receiveRequest() [method:" << method << "]";

		return promise::resolve()
			.then([=]()
		{
			if (method == "queryRoom")
			{
				Json response =
				{
					{ "rtpCapabilities", this->rtpCapabilities() },
					{ "mandatoryCodecPayloadTypes" , Json::array() } // TODO
				};

				return response;
			}
			else if (method == "join")
			{
				std::string peerName = request["peerName"].get<std::string>();
				Json rtpCapabilities = request["rtpCapabilities"];
				bool spy = request.value("spy", false);
				Json appData = request["appData"];

				Peer* peer = this->_createPeer(peerName, rtpCapabilities, spy, appData);

				Json response(Json::object());

				Json peers(Json::array());

				for (auto it : this->peers())
				{
					Peer* otherPeer = it.second;
					if (otherPeer != peer && !otherPeer->spy())
					{
						Json consumers(Json::array());
						for (auto itSub : peer->consumers())
						{
							Consumer* consumer = itSub.second;
							if (consumer->peer() == otherPeer)
							{
								consumers.push_back(consumer->toJson());
							}
						}
						peers.push_back(
							Object::assign({ { "consumers" ,consumers } }, otherPeer->toJson())
						);
					}
				}

				response["peers"] = peers;

				return response;
			}
			else
			{
				throw Error("unknown request method " + method);
			}
		})
			.fail([=](Error error)
		{
			LOG(ERROR) << "receiveRequest() failed [method:" << method
				<< ": " << error.ToString() << "]";

			throw error;
		});
	}

	ActiveSpeakerDetector* Room::createActiveSpeakerDetector()
	{
		DLOG(INFO) << "createActiveSpeakerDetector()";

		ActiveSpeakerDetector* activeSpeakerDetector = new ActiveSpeakerDetector(this);

		return activeSpeakerDetector;
	}

	Defer Room::createRtpStreamer(Producer* producer, const Json& options)
	{
		DLOG(INFO) << "createRtpStreamer()";

		if (!this->_producers.count(producer->id()))
			return promise::reject(Error("Producer not found"));

		PlainRtpTransport* plainRtpTransport = nullptr;
		Consumer* consumer = nullptr;

		return promise::resolve()
			// Create a PlainRtpTransport in the worker.
			.then([=, &plainRtpTransport]()
		{
			return this->_createPlainRtpTransport(options)
				.then([=, &plainRtpTransport](PlainRtpTransport* transport)
			{
				plainRtpTransport = transport;
			});
		})
			// Create a Consumer.
			.then([=]()
		{
			Json internal =
			{
				{ "routerId", this->_internal["routerId"]},
				{ "consumerId" , utils::randomNumber()},
				{ "producerId", producer->id()},
				{ "transportId" , undefined }
			};

			// TODO: The app should provide his own RTP capabilities.
			Json consumerRtpParameters =
				ortc::getConsumerRtpParameters(
					producer->consumableRtpParameters(), this->rtpCapabilities());

			Json data =
			{
				{"kind" , producer->kind()},
				//{"transport" , undefined},
				{"rtpParameters" , consumerRtpParameters}
				//{"source" , producer}
			};

			// Create a Consumer instance.
			Consumer* consumer = new Consumer(nullptr, producer, internal, data, this->_channel);

			return this->_channel->request("router.createConsumer", internal,
				{
					{"kind",  consumer->kind()}
				})
				.then([=]()
			{
				DLOG(INFO) << "\"router.createConsumer\" request succeeded";
			})
				.fail([=](Error error)
			{
				LOG(ERROR) << "\"router.createConsumer\" request failed: " << error.ToString();

				throw error;
			});
		})
			// 	.then([=]()
			// 	{
			// 		return consumer->enable(plainRtpTransport);
			// 	})
			// 	.then([=]()
			// 	{
			// 		RtpStreamer* rtpStreamer = new plugins.RtpStreamer(consumer, plainRtpTransport);
			// 
			// 		return rtpStreamer;
			// 	})
			.fail([=]()
		{
			if (plainRtpTransport)
				plainRtpTransport->close();

			if (consumer)
				consumer->close();
		});
	}

	Peer* Room::_createPeer(std::string peerName, const Json& rtpCapabilities, bool spy, const Json& appData)
	{
		DLOG(INFO) << "_createPeer() [peerName:" << peerName << "]";

		if (peerName.empty())
			throw TypeError("peerName must be a string");
		else if (this->_peers.count(peerName))
			throw Error(utils::Printf("duplicated Peer[peerName:\"%s\"]", peerName.c_str()));

		// Ensure that the given capabilties are a subset of the Room capabilities.
		// NOTE: This may throw.
		ortc::assertCapabilitiesSubset(rtpCapabilities, this->rtpCapabilities());

		Json internal =
		{
			{ "routerId" , this->_internal["routerId"] },
			{ "peerName" , peerName }
		};

		Json data =
		{
			{ "rtpCapabilities" , rtpCapabilities },
			{ "spy" , spy },
		};

		SandBox sandbox =
		{
			// NOTE: This may throw.
			//getProducerRtpParametersMapping
			[=](Json producerRtpParameters)->Json
			{
				return ortc::getProducerRtpParametersMapping(
					producerRtpParameters, this->rtpCapabilities());
			},
			// NOTE: This may throw.
			//	getConsumableRtpParameters 
			[=](Json producerRtpParameters,Json rtpMapping)->Json
			{
				return ortc::getConsumableRtpParameters(
					producerRtpParameters, this->rtpCapabilities(), rtpMapping);
			}
		};

		// Create a Peer.
		Peer* peer = new Peer(internal, data, this->_channel, sandbox);

		peer->appData(appData);

		// Store the Peer and remove it when closed. Also notify allthe other Peers.
		// (unless it's a spy peer).
		this->_peers[peer->name()] = peer;
		peer->addEventListener("@close", [=](Json appData2)
		{
			this->_peers.erase(peer->name());

			if (!spy)
			{
				for (auto it : this->_peers)
				{
					Peer* otherPeer = it.second;
					if (otherPeer == peer)
						continue;

					otherPeer->handlePeerClosed(peer, appData2);
				}
			}
		});

		// Create a Consumer for this Peer associated to each Producer in other
		// Peers.
		for (auto it : this->_peers)
		{
			Peer* otherPeer = it.second;
			if (otherPeer == peer)
				continue;

			for (auto producer : otherPeer->producers())
			{
				peer->addConsumerForProducer(producer.second, false);
			}
		}

		// Listen for new Producers so we can create new Consumers for other Peers.
		peer->addEventListener("@newproducer", [=](Json data)
		{
			Producer* producer = peer->getProducerById(data.get<uint32_t>());
			if (producer)
			{
				this->_handleProducer(producer);
			}
		});

		// Tell all the Peers (but us) about the new Peer (unless this is a spy peer).
		if (!spy)
		{
			// Tell all the Peers (but us) about the new Peer.
			for (auto it : this->_peers)
			{
				Peer* otherPeer = it.second;
				if (otherPeer == peer)
					continue;

				otherPeer->handleNewPeer(peer);
			}
		}

		this->doEvent("newpeer", peer->name());

		return peer;
	}

	void Room::_handleProducer(Producer* producer)
	{
		// Store the Producer and remove it when closed.
		this->_producers[producer->id()] = producer;
		producer->addEventListener("@close", [=](Json data)
		{
			this->_producers.erase(producer->id());
		});

		// Tell all the Peers (but us) about the new Producer.
		for (auto it : this->_peers)
		{
			Peer* otherPeer = it.second;
			if (otherPeer == producer->peer())
				continue;

			otherPeer->addConsumerForProducer(producer);
		}
	}

	Defer Room::_createPlainRtpTransport(const Json& options)
	{
		DLOG(INFO) << "_createPlainRtpTransport()";

		Json internal =
		{
			{"routerId" , this->_internal["routerId"]},
			{"transportId" , utils::randomNumber()}
		};

		return this->_channel->request("router.createPlainRtpTransport", internal, options)
			.then([=](Json data)
		{
			DLOG(INFO) << "\"router.createPlainRtpTransport\" request succeeded";

			// Create a PlainRtpTransport.
			PlainRtpTransport* transport = new PlainRtpTransport(internal, data, this->_channel);

			return transport;
		})
			.fail([=](Error error)
		{
			LOG(ERROR) << "\"router.createPlainRtpTransport\" request failed:" << error.ToString();

			throw error;
		});
	}

	void Room::_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		uint32_t routerId = this->_internal["routerId"].get<uint32_t>();
		this->_channel->addEventListener(routerId, this);

		// Subscribe to new events.
		this->addEventListener("newListener", [=](Json data)
		{
			std::string event = data.get<std::string>();
			if (event == "audiolevels")
			{
				// Ignore if there are listeners already.
				if (this->listeners("audiolevels"))
					return;

				this->_channel->request(
					"router.setAudioLevelsEvent", this->_internal, { { "enabled", true } })
					.then([=]()
				{
					DLOG(INFO) << "\"router.setAudioLevelsEvent\" request succeeded";
				})
					.fail([=](Error error)
				{
					LOG(ERROR) << 
						"\"router.setAudioLevelsEvent\" request failed: " << error.ToString();
				});
			}
		});

		// Subscribe to events removal.
		this->addEventListener("removeListener", [=](Json data)
		{
			std::string event = data.get<std::string>();
			if (event == "audiolevels")
			{
				// Ignore if there are other remaining listeners.
				if (this->listeners("audiolevels"))
					return;

				this->_channel->request(
					"router.setAudioLevelsEvent", this->_internal, { {"enabled", false } })
					.then([=]()
				{
					DLOG(INFO) << "\"router.setAudioLevelsEvent\" request succeeded";
				})
					.fail([=](Error error)
				{
					LOG(ERROR) << 
						"\"router.setAudioLevelsEvent\" request failed: " << error.ToString();
				});
			}
		});
	}

	void Room::onEvent(std::string event, Json data)
	{
		if (event == "audiolevels")
		{
			struct Level
			{
				uint32_t producerId = 0;
				int32_t audioLevel = 0;
			};
			const Json& entries = data["entries"];
			std::vector<Level> levels;
			for (auto &entry : entries)
			{
				Level level;
				level.producerId = entry[0].get<uint32_t>();
				level.audioLevel = entry[1].get<int32_t>();
				levels.push_back(level);
			}

			std::sort(levels.begin(), levels.end(), [](const Level& a, const Level& b) {
				return a.audioLevel < b.audioLevel;
			});

			Json jLevels(Json::array());
			for(auto &level : levels)
			{
				Json jLevel(Json::object());
				jLevel["producerId"] = level.producerId;
				jLevel["audioLevel"] = level.audioLevel;
				jLevels.push_back(jLevel);
			}

			this->doEvent("audiolevels", jLevels);
		}
		else
		{
			LOG(ERROR) << "ignoring unknown event: " << event;
		}
	}

}
