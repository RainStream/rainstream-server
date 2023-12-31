#include "RainStream.hpp"
#include "Peer.hpp"
#include "Channel.hpp"
#include "Logger.hpp"
#include <set>

namespace rs
{
	const std::set<std::string> KINDS = { "audio", "video", "depth" };

	Peer::Peer(const Json& internal, const Json& data, Channel* channel, SandBox sandbox)
	{
		DLOG(INFO) << "constructor() [internal:" << internal.dump() << "]";

		// Closed flag.
		this->_closed = false;

		// Internal data.
		// - .routerId
		// - .peerName
		this->_internal = internal;

		// Peer data.
		// - .rtpCapabilities
		this->_data = data;

		// Channel instance.
		this->_channel = channel;

		// Store internal sandbox stuff.
		// - .getProducerRtpParametersMapping()
		this->_sandbox = sandbox;

		// App data.
		this->_appData = undefined;
	}

	std::string Peer::name()
	{
		return this->_internal["peerName"];
	}

	bool Peer::closed()
	{
		return this->_closed;
	}

	Json Peer::appData()
	{
		return this->_appData;
	}

	void Peer::appData(Json appData)
	{
		this->_appData = appData;
	}

	Json Peer::rtpCapabilities()
	{
		return this->_data["rtpCapabilities"];
	}

	bool Peer::spy()
	{
		return this->_data["spy"];
	}

	/**
	* Get an array with all the WebRtcTransports.
	*
	* @return {Array<WebRtcTransport>}
	*/
	WebRtcTransports Peer::transports()
	{
		return this->_transports;
	}

	/**
	* Get an array with all the Producers.
	*
	* @return {Array<Producers>}
	*/
	Producers Peer::producers()
	{
		return this->_producers;
	}

	/**
	* Get an array with all the Consumer.
	*
	* @return {Array<Consumer>}
	*/
	Consumers Peer::consumers()
	{
		return this->_consumers;
	}

	/**
	* Get the Consumer asasociated to the given source (Producer).
	*
	* @return {Consumer}
	*/
	Consumer* Peer::getConsumerForSource(Producer* source)
	{
		for (auto &consumer : this->consumers())
		{
			if (consumer.second->source() == source)
			{
				return consumer.second;
			}
		}

		return nullptr;
	}

	/**
	* Close the Peer.
	*
	* @param {Any} [appData] - App custom data.
	* @param {bool} [notifyChannel=true] - Private.
	*/
	void Peer::close(Json appData, bool notifyChannel/* = true*/)
	{
		DLOG(INFO) << "close()";

		if (this->_closed)
			return;

		// Send a notification.
		this->_sendNotification("closed", appData);

		this->_closed = true;

		this->doEvent("@close", appData);
		this->doEvent("close", appData);

		// Close all the Producers.
		auto producers = this->_producers;
		for (auto it : producers)
		{
			it.second->close(undefined, notifyChannel);

			//delete it.second;
		}

		// Close all the Consumers.
		auto consumers = this->_consumers;
		for (auto it : consumers)
		{
			it.second->close(notifyChannel);

			//delete it.second;
		}

		// Close all the WebRtcTransports.
		auto transports = this->_transports;
		for (auto it : transports)
		{
			it.second->close(undefined, notifyChannel);

			//delete it.second;
		}

		delete this;
	}

	/**
	* The remote Peer left the Room.
	* Invoked via remote notification.
	*
	* @private
	*
	* @param {Any} [appData] - App custom data.
	*/
	void Peer::remoteClose(Json appData)
	{
		DLOG(INFO) << "remoteClose()";

		if (this->closed())
			return;

		this->_closed = true;

		this->doEvent("@close", appData);
		this->doEvent("close", appData);

		// Close all the Producers.
		auto producers = this->_producers;
		for (auto it : producers)
		{
			it.second->remoteClose();

			delete it.second;
		}

		// Close all the Consumers.
		auto consumers = this->_consumers;
		for (auto it : consumers)
		{
			it.second->close();

			delete it.second;
		}

		// Close all the WebRtcTransports.
		auto transports = this->_transports;
		for (auto it : transports)
		{
			it.second->remoteClose();

			delete it.second;
		}

		delete this;
	}

	/**
	* Get WebRtcTransport by id.
	*
	* @param {Number} id
	*
	* @return {WebRtcTransport}
	*/
	WebRtcTransport* Peer::getTransportById(uint32_t id)
	{
		if (this->_transports.count(id))
		{
			return this->_transports[id];
		}
		return nullptr;
	}

	/**
	* Get Producer by id.
	*
	* @param {Number} id
	*
	* @return {Producer}
	*/
	Producer* Peer::getProducerById(uint32_t id)
	{
		if (this->_producers.count(id))
		{
			return this->_producers[id];
		}
		return nullptr;
	}

	/**
	* Get Consumer by id.
	*
	* @param {Number} id
	*
	* @return {Consumer}
	*/
	Consumer* Peer::getConsumerById(uint32_t id)
	{
		if (this->_consumers.count(id))
		{
			return this->_consumers[id];
		}
		return nullptr;
	}

	/**
	* Dump the Peer.
	*
	* @private
	*
	* @return {Promise}
	*/
	Defer Peer::dump()
	{
		DLOG(INFO) << "dump()";

		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Peer closed"));

		Json peerData =
		{
			{ "peerName", this->name() },
			{ "transports" , Json::array() },
			{ "producers"  , Json::array() },
			{ "consumers"  , Json::array() }
		};

		std::vector<Defer> promises;

		for (auto transport : this->_transports)
		{
			auto promise = transport.second->dump()
				.then([&](Json data)
			{
				peerData["transports"].push_back(data);
			});

			promises.push_back(promise);
		}

		for (auto it : this->_producers)
		{
			auto producer = it.second;
			auto promise = producer->dump()
				.then([&](Json data)
			{
				peerData["producers"].push_back(data);
			});

			promises.push_back(promise);
		}

		for (auto it : this->_consumers)
		{
			auto consumer = it.second;
			auto promise = consumer->dump()
				.then([&](Json data)
			{
				peerData["consumers"].push_back(data);
			});

			promises.push_back(promise);
		}

		return promise::all(promises)
		.then([=]()
		{
			DLOG(INFO) << "dump() | succeeded";

			return peerData;
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << "dump() | failed: " << error.ToString();

			throw error;
		});
	}

	Json Peer::toJson()
	{
		return Json{
			{ "name"    , this->name() },
			{ "appData" , this->appData() }
		};
	}

	Defer Peer::receiveRequest(Json request)
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
			if (method == "createTransport")
			{
				uint32_t id = request["id"].get<uint32_t>();
				std::string direction = request["direction"].get<std::string>();
				Json options = request["options"];
				Json dtlsParameters = request.value("dtlsParameters",Json());
				Json appData = request["appData"];

				return this->_createWebRtcTransport(id, direction, options, appData)
				.then([=](WebRtcTransport* transport)
				{
					// If no client"s DTLS parameters are given, we are done.
					if (dtlsParameters.is_null())
					{
						return promise::resolve()
						.then([=]()
						{
							return transport;
						});
					}
					else
					{
						return transport->setRemoteDtlsParameters(dtlsParameters)
						.then([=]()
						{
							return transport;
						})
						.fail([=](Error error)
						{
							transport->remoteClose();

							throw error;
						});
					}
				})
				.then([=](WebRtcTransport* transport)
				{
					Json response =
					{
						{ "iceParameters"  , transport->iceLocalParameters() },
						{ "iceCandidates"  , transport->iceLocalCandidates() },
						{ "dtlsParameters" , transport->dtlsLocalParameters() }
					};

					return response;
				});
			}
			else if (method == "restartTransport")
			{
				uint32_t id = request["id"].get<uint32_t>();
				WebRtcTransport* transport = this->_transports[id];

				if (!transport)
					throw Error("Transport not found");

				return transport->changeUfragPwd()
					.then([=]()
				{
					Json response =
					{
						{"iceParameters" , transport->iceLocalParameters()}
					};

					return response;
				});
			}
			else if (method == "createProducer")
			{
				uint32_t id = request["id"].get<uint32_t>();
				std::string kind = request["kind"].get<std::string>();
				uint32_t transportId = request["transportId"].get<uint32_t>();
				Json rtpParameters = request["rtpParameters"];
				bool paused = request["paused"].get<bool>();
				Json appData = request["appData"];

				bool remotelyPaused = paused;

				return this->_createProducer(id, kind, transportId,
					rtpParameters, remotelyPaused, appData);
			}

			else if (method == "enableConsumer")
			{
				uint32_t id = request["id"].get<uint32_t>();
				uint32_t transportId = request["transportId"].get<uint32_t>();
				bool paused = request["paused"].get<bool>();
				Json preferredProfile = request["preferredProfile"];

				Consumer* consumer = this->_consumers[id];
				WebRtcTransport* transport = this->_transports[transportId];

				if (!consumer)
					throw Error("Consumer not found");
				else if (!transport)
					throw TypeError("Transport not found");

				return consumer->enable(transport, paused, preferredProfile)
					.then([=]()
				{
					return Json{
						{ "paused", consumer->sourcePaused() || consumer->locallyPaused() },
						{ "preferredProfile" , consumer->preferredProfile() },
						{ "effectiveProfile" , consumer->effectiveProfile() }
					};
				});
			}
			else {
				throw Error("unknown request method \"${method}\"");
			}
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << "receiveRequest() failed [method:" << method << "]:" << error.ToString();

			throw error;
		});
	}

	Defer Peer::receiveNotification(Json notification)
	{
		if (this->_closed)
			return promise::reject(errors::InvalidStateError("Room closed"));
		else if (!notification.is_object())
			return promise::reject(TypeError("wrong notification Object"));
		else if (notification["notification"].get<bool>() != true)
			return promise::reject(TypeError("not a notification"));
		else if (!notification["method"].is_string())
			return promise::reject(TypeError("wrong/missing notification method"));

		std::string method = notification["method"].get<std::string>();

		DLOG(INFO) << "receiveNotification() [method:" << method << "]";

		return promise::resolve()
			.then([=]()
		{
			if (method == "leave")
			{
				Json appData = notification["appData"];

				this->remoteClose(appData);
			}
			else if (method == "updateTransport")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json dtlsParameters = notification["dtlsParameters"];

				WebRtcTransport* transport = this->_transports[id];

				if (!transport)
					throw Error("Transport not found");

				return transport->setRemoteDtlsParameters(dtlsParameters)
				.fail([=](Error error)
				{
					transport->close();

					throw error;
				});
			}
			else if (method == "closeTransport")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json appData = notification["appData"];

				WebRtcTransport* transport = this->_transports[id];

				if (!transport)
					throw Error("Transport not found");

				transport->remoteClose(appData);

			}
			else if (method == "enableTransportStats")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json interval = notification["interval"];

				WebRtcTransport* transport = this->_transports[id];

				if (!transport)
					throw Error("Transport not found");

				transport->enableStats(interval);
			}
			else if (method == "disableTransportStats")
			{
				uint32_t id = notification["id"].get<uint32_t>();

				WebRtcTransport* transport = this->_transports[id];

				if (!transport)
					throw Error("Transport not found");

				transport->disableStats();
			}
			else if (method == "updateProducer")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json rtpParameters = notification["rtpParameters"];

				Producer* producer = this->_producers[id];

				if (!producer)
					throw Error("Producer not found");

				producer->updateRtpParameters(rtpParameters);
			}
			else if (method == "pauseProducer")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json appData = notification.value("appData",Json::object());

				Producer* producer = this->_producers[id];

				if (!producer)
					throw Error("Producer not found");

				producer->remotePause(appData);

				return promise::resolve();
			}
			else if (method == "resumeProducer")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json appData = notification.value("appData", Json::object());

				Producer* producer = this->_producers[id];

				if (!producer)
					throw Error("Producer not found");

				producer->remoteResume(appData);

				return promise::resolve();
			}
			else if (method == "closeProducer")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json appData = notification.value("appData", Json::object());

				Producer* producer = this->_producers[id];

				// Just ignore it.
				if (producer)
				{
					producer->remoteClose(appData);
				}

				return promise::resolve();
			}
			else if (method == "enableProducerStats")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json interval = notification["interval"];

				Producer* producer = this->_producers[id];

				if (!producer)
					throw Error("Producer not found");

				producer->enableStats(interval);
			}
			else if (method == "disableProducerStats")
			{
				uint32_t id = notification["id"].get<uint32_t>();

				Producer* producer = this->_producers[id];

				if (!producer)
					throw Error("Producer not found");

				producer->disableStats();
			}
			else if (method == "pauseConsumer")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json appData = notification["appData"];

				Consumer* consumer = this->_consumers[id];

				if (!consumer)
					throw Error("Consumer not found");

				consumer->remotePause(appData);
			}
			else if (method == "resumeConsumer")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json appData = notification["appData"];

				Consumer* consumer = this->_consumers[id];

				if (!consumer)
					throw Error("Consumer not found");

				consumer->remoteResume(appData);
			}
			else if (method == "setConsumerPreferredProfile")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json profile = notification["profile"];

				Consumer* consumer = this->_consumers[id];

				if (!consumer)
					throw Error("Consumer not found");

				consumer->remoteSetPreferredProfile(profile);
			}
			else if (method == "enableConsumerStats")
			{
				uint32_t id = notification["id"].get<uint32_t>();
				Json interval = notification["interval"];

				Consumer* consumer = this->_consumers[id];

				if (!consumer)
					throw Error("Consumer not found");

				consumer->enableStats(interval);
			}
			else if (method == "disableConsumerStats")
			{
				uint32_t id = notification["id"].get<uint32_t>();

				Consumer* consumer = this->_consumers[id];

				if (!consumer)
					throw Error("Consumer not found");

				consumer->disableStats();
			}
			else
			{
				throw Error("unknown notification method \"${method}\"");
			}
		})
			.fail([=](Error error)
		{
			LOG(ERROR) << 
				"receiveNotification() failed [method:" << method << "]:" << error.ToString();
		});
	}

	/**
	* @private
	*/
	void Peer::handleNewPeer(Peer* peer)
	{
		Json peerData =
		{
			{ "consumers" , Json::array() },
		};

		peerData = Object::assign(peerData, peer->toJson());

		this->_sendNotification("newPeer", peerData);
	}

	/**
	* @private
	*/
	void Peer::handlePeerClosed(Peer* peer, Json appData)
	{
		this->_sendNotification("peerClosed", { { "name" , peer->name() }, { "appData",appData } });
	}

	/**
	* @private
	*/
	void Peer::addConsumerForProducer(Producer* producer, bool notifyClient/* = true*/)
	{
		Json internal =
		{
			{ "routerId"    , this->_internal["routerId"] },
			{ "consumerId"  , utils::randomNumber() },
			{ "producerId"  , producer->id() },
			{ "transportId" , undefined }
		};

		Json consumerRtpParameters =
			ortc::getConsumerRtpParameters(
				producer->consumableRtpParameters(), this->rtpCapabilities());

		Json data =
		{
			{ "kind"          , producer->kind()},
			//{ "peer"          , producer->peer() },
			//{ "transport"     , undefined },
			{ "rtpParameters" , consumerRtpParameters },
			//{ "source"        , producer }
		};

		// Create a Consumer instance.
		Consumer* consumer = new Consumer(producer->peer(), producer, internal, data, this->_channel);

		consumer->appData(producer->appData());

		// Store the Consumer and remove it when closed.
		this->_consumers[consumer->id()] = consumer;
		consumer->addEventListener("@close", [=](Json data)
		{
			this->_consumers.erase(consumer->id());
		});

		consumer->addNotifyListener([=](std::string method, Json data2)
		{
			this->_sendNotification(method, data2);
		});

		this->_channel->request("router.createConsumer", internal,
			{
				{ "kind", consumer->kind() }
			}
		)
		.then([=]()
		{
			DLOG(INFO) << "\"router.createConsumer\" request succeeded";
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << "\"router.createConsumer\" request failed: "<< error.ToString();

			throw error;
		});

		this->doEvent("newconsumer", consumer->id());

		if (notifyClient)
			this->_sendNotification("newConsumer", consumer->toJson());
	}

	void Peer::_sendNotification(std::string method, Json data)
	{
		// Ignore if closed.
		if (this->_closed)
			return;

		Json notification =
		{
			{ "method" , method },
			{ "target", "peer" },
			{ "notification", true } ,
		};

		notification = Object::assign(notification, data);

		DLOG(INFO) << "_sendNotification() [method:" << method << "]";

		this->doEvent("notify", notification);
	}

	Defer Peer::_createWebRtcTransport(uint32_t id, std::string direction, Json options, Json appData)
	{
		DLOG(INFO) << "_createWebRtcTransport() [id:" << id << " direction:" << direction << "]";

		Json internal =
		{
			{ "routerId" , this->_internal["routerId"] },
			{ "transportId" , id ? id : utils::randomNumber() }
		};

		return this->_channel->request("router.createWebRtcTransport", internal, options)
		.then([=](Json data)
		{
			DLOG(INFO) << "\"router.createWebRtcTransport\" request succeeded";

			data = Object::assign(data, { {"direction",direction } });

			// Create a WebRtcTransport.
			WebRtcTransport* transport = new WebRtcTransport(internal, data, this->_channel);

			transport->appData(appData);

			// Store the WebRtcTransport and remove it when closed.
			this->_transports[transport->id()] = transport;
			transport->addEventListener("@close", [=](Json data) {this->_transports.erase(transport->id()); });

			transport->addNotifyListener([=](std::string method, Json data2)
			{
				this->_sendNotification(method, data2);
			});

			this->doEvent("newtransport", transport->id());

			return transport;
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << 
				"\"router.createWebRtcTransport\" request failed:" << error.ToString();

			throw error;
		});
	}

	Defer Peer::_createProducer(uint32_t id, std::string kind, uint32_t transportId, Json rtpParameters, bool remotelyPaused, Json appData)
	{
		DLOG(INFO) << "_createProducer() [id:" << id << ",kind:" << kind << "]";

		if (this->rtpCapabilities().is_null())
			return promise::reject(errors::InvalidStateError("RTP capabilities unset"));
		else if (!KINDS.count(kind))
			return promise::reject(TypeError(utils::Printf("unsupported kind: %s", kind.c_str())));
		else if (this->spy())
			return promise::reject(Error("spy peers cannot create producers"));

		Json rtpMappingForWorker;
		Json consumableRtpParameters;

		try
		{
			Json rtpMapping =
				this->_sandbox.getProducerRtpParametersMapping(rtpParameters);

			// NOTE: rtpMapping has two Maps, but we need to convert them into arrays
			// of pairs for the worker.
			rtpMappingForWorker =
			{
				{ "codecPayloadTypes"  , rtpMapping["mapCodecPayloadTypes"] },
				{ "headerExtensionIds" , rtpMapping["mapHeaderExtensionIds"] }
			};

			consumableRtpParameters =
				this->_sandbox.getConsumableRtpParameters(rtpParameters, rtpMapping);
		}
		catch (Error error)
		{
			return promise::reject(Error(std::string("wrong RTP parameters : ") + error.ToString().c_str()));
		}

		auto transport = this->getTransportById(transportId);

		if (!transport)
			return promise::reject(TypeError("WebRtcTransport not found"));

		Json internal =
		{
			{ "routerId"    , this->_internal["routerId"] },
			{ "producerId"  , id ? id : utils::randomNumber() },
			{ "transportId" , transport->id() }
		};

		Json data =
		{
			{ "kind"                    , kind },
			//{ "peer"                    , this },
			//{ "transport"               , transport },
			{ "rtpParameters"           , rtpParameters },
			{ "consumableRtpParameters" , consumableRtpParameters }
		};

		Json options =
		{
			{ "remotelyPaused" , remotelyPaused }
		};

		return this->_channel->request("router.createProducer", internal,
			{
				{ "kind", kind },
				{ "rtpParameters" , rtpParameters },
				{ "rtpMapping" , rtpMappingForWorker },
				{ "paused" , remotelyPaused }
			})
		.then([=]()
		{
			DLOG(INFO) << "\"router.createProducer\" request succeeded";

			Producer* producer = new Producer(this, transport, internal, data, this->_channel, options);

			producer->appData(appData);

			// Store the Producer and remove it when closed.
			this->_producers[producer->id()] = producer;
			producer->addEventListener("@close", [=](Json data) { this->_producers.erase(producer->id()); });

			producer->addNotifyListener([=](std::string method, Json data2)
			{
				this->_sendNotification(method, data2);
			});

			this->doEvent("@newproducer", producer->id());
			this->doEvent("newproducer", producer->id());

			// Ensure we return nothing.
			return Json::object();
		})
		.fail([=](Error error)
		{
			LOG(ERROR) << "\"router.createProducer\" request failed:" << error.ToString();

			throw error;
		});
	}

}
