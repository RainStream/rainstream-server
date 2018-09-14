#include "RainStream.hpp"
#include "Room.hpp"
#include "Logger.hpp"
#include "Consumer.hpp"
#include "Channel.hpp"

namespace rs
{

	Room::Room(const Json& internal, const Json& data, Channel* channel)
	{
		LOG(INFO) << "constructor()";

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
		LOG(INFO) << "close()";

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
				LOG(INFO) << "\"router.close\" request succeeded";
			})
			.fail([=](Error error)
			{
				LOG(ERROR) << "\"router.close\" request failed: " << error.ToString();
			});
		}
	}

	Defer Room::dump()
	{
		LOG(INFO) << "dump()";

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Room closed"));

		// TODO: Should we aggregate Peers info?

		return this->_channel->request("router.dump", this->_internal)
			.then([=](Json data)
		{
			LOG(INFO) << "\"router.dump\" request succeeded";

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

	Defer Room::receiveRequest(const Json& request)
	{
		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Room closed"));
		else if (!request.is_object())
			return promise::reject(new TypeError("wrong request Object"));
		else if (request.count("notification"))
			return promise::reject(new TypeError("not a request"));
		else if (!request["method"].is_string())
			return promise::reject(new TypeError("wrong/missing request method"));

		std::string method = request["method"].get<std::string>();

		LOG(INFO) << "receiveRequest() [method:" << method << "]";

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
				Json appData = request["appData"];

				Peer* peer = this->_createPeer(peerName, rtpCapabilities, appData);

				Json response(Json::object());

				Json peers(Json::array());

				for (auto it : this->peers())
				{
					Peer* otherPeer = it.second;
					if (otherPeer != peer)
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
		LOG(INFO) << "createActiveSpeakerDetector()";

		// 	ActiveSpeakerDetector* activeSpeakerDetector = new plugins.ActiveSpeakerDetector(this);
		// 
		// 	return activeSpeakerDetector;

		return nullptr;
	}

	Defer Room::createRtpStreamer(Producer* producer, const Json& options)
	{
		LOG(INFO) << "createRtpStreamer()";

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
				{"routerId", this->_internal["routerId"]},
				{"consumerId" , utils::randomNumber()},
				{"producerId", producer->id()},
				{"transportId" , undefined }
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
				Json{
					{"kind",  consumer->kind()}
				})
				.then([=]()
			{
				LOG(INFO) << "\"router.createConsumer\" request succeeded";
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

	Peer* Room::_createPeer(std::string peerName, const Json& rtpCapabilities, const Json& appData)
	{
		LOG(INFO) << "_createPeer() [peerName:" << peerName << "]";

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
			{ "rtpCapabilities" , rtpCapabilities }
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
		this->_peers[peer->name()] = peer;
		peer->addEventListener("@close", [=](Json appData2)
		{
			this->_peers.erase(peer->name());

			for (auto it : this->_peers)
			{
				Peer* otherPeer = it.second;
				if (otherPeer == peer)
					continue;

				otherPeer->handlePeerClosed(peer, appData2);
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

		// Tell all the Peers (but us) about the new Peer.
		for (auto it : this->_peers)
		{
			Peer* otherPeer = it.second;
			if (otherPeer == peer)
				continue;

			otherPeer->handleNewPeer(peer);
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
		LOG(INFO) << "_createPlainRtpTransport()";

		Json internal =
		{
			{"routerId" , this->_internal["routerId"]},
			{"transportId" , utils::randomNumber()}
		};

		return this->_channel->request("router.createPlainRtpTransport", internal, options)
			.then([=](Json data)
		{
			LOG(INFO) << "\"router.createPlainRtpTransport\" request succeeded";

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
					"router.setAudioLevelsEvent", this->_internal, Json{ {"enabled", true} })
					.then([=]()
				{
					LOG(INFO) << "\"router.setAudioLevelsEvent\" request succeeded";
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
					"router.setAudioLevelsEvent", this->_internal, Json{ {"enabled", false} })
					.then([=]()
				{
					LOG(INFO) << "\"router.setAudioLevelsEvent\" request succeeded";
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
			// 			const Json& entries = data["entries"];
			// 			for (auto &levels : entries)
			// 			{
			// 
			// 			}
			// 			const levels = entries
			// 				.map((entry) = >
			// 			{
			// 				return {
			// 				producer: this->_producers.get(entry[0]),
			// 						  audioLevel : entry[1]
			// 				};
			// 			})
			// 				.filter((entry) = > bool(entry.producer))
			// 				.sort((a, b) = >
			// 			{
			// 				if (a.audioLevel > b.audioLevel)
			// 					return -1;
			// 				else if (a.audioLevel < b.audioLevel)
			// 					return 1;
			// 				else
			// 					return 0;
			// 			});
			// 
			// 			this->safeEmit("audiolevels", levels);
		}
		else
		{
			LOG(ERROR) << "ignoring unknown event: " << event;
		}
	}

}
