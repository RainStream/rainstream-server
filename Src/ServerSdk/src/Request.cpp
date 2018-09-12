#include "RainStream.hpp"
#include "Request.hpp"
#include "errors.hpp"
#include "utils.hpp"

namespace rs
{
	/* Class variables. */

	// clang-format off
	std::unordered_map<std::string, Request::MethodId> Request::string2MethodId =
	{
		{ "worker.dump",                       Request::MethodId::WORKER_DUMP                          },
		{ "worker.updateSettings",             Request::MethodId::WORKER_UPDATE_SETTINGS               },
		{ "worker.createRouter",               Request::MethodId::WORKER_CREATE_ROUTER                 },
		{ "router.close",                      Request::MethodId::ROUTER_CLOSE                         },
		{ "router.dump",                       Request::MethodId::ROUTER_DUMP                          },
		{ "router.createWebRtcTransport",      Request::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT       },
		{ "router.createPlainRtpTransport",    Request::MethodId::ROUTER_CREATE_PLAIN_RTP_TRANSPORT    },
		{ "router.createProducer",             Request::MethodId::ROUTER_CREATE_PRODUCER               },
		{ "router.createConsumer",             Request::MethodId::ROUTER_CREATE_CONSUMER               },
		{ "router.setAudioLevelsEvent",        Request::MethodId::ROUTER_SET_AUDIO_LEVELS_EVENT        },
		{ "transport.close",                   Request::MethodId::TRANSPORT_CLOSE                      },
		{ "transport.dump",                    Request::MethodId::TRANSPORT_DUMP                       },
		{ "transport.getStats",                Request::MethodId::TRANSPORT_GET_STATS                  },
		{ "transport.setRemoteDtlsParameters", Request::MethodId::TRANSPORT_SET_REMOTE_DTLS_PARAMETERS },
		{ "transport.setMaxBitrate",           Request::MethodId::TRANSPORT_SET_MAX_BITRATE            },
		{ "transport.changeUfragPwd",          Request::MethodId::TRANSPORT_CHANGE_UFRAG_PWD           },
		{ "transport.startMirroring",          Request::MethodId::TRANSPORT_START_MIRRORING            },
		{ "transport.stopMirroring",           Request::MethodId::TRANSPORT_STOP_MIRRORING             },
		{ "producer.close",                    Request::MethodId::PRODUCER_CLOSE                       },
		{ "producer.dump",                     Request::MethodId::PRODUCER_DUMP                        },
		{ "producer.getStats",                 Request::MethodId::PRODUCER_GET_STATS                   },
		{ "producer.updateRtpParameters",      Request::MethodId::PRODUCER_UPDATE_RTP_PARAMETERS       },
		{ "producer.pause",                    Request::MethodId::PRODUCER_PAUSE                       },
		{ "producer.resume" ,                  Request::MethodId::PRODUCER_RESUME                      },
		{ "producer.setPreferredProfile",      Request::MethodId::PRODUCER_SET_PREFERRED_PROFILE       },
		{ "consumer.close",                    Request::MethodId::CONSUMER_CLOSE                       },
		{ "consumer.dump",                     Request::MethodId::CONSUMER_DUMP                        },
		{ "consumer.getStats",                 Request::MethodId::CONSUMER_GET_STATS                   },
		{ "consumer.enable",                   Request::MethodId::CONSUMER_ENABLE                      },
		{ "consumer.pause",                    Request::MethodId::CONSUMER_PAUSE                       },
		{ "consumer.resume",                   Request::MethodId::CONSUMER_RESUME                      },
		{ "consumer.setPreferredProfile",      Request::MethodId::CONSUMER_SET_PREFERRED_PROFILE       },
		{ "consumer.setEncodingPreferences",   Request::MethodId::CONSUMER_SET_ENCODING_PREFERENCES    },
		{ "consumer.requestKeyFrame",          Request::MethodId::CONSUMER_REQUEST_KEY_FRAME           }
	};
	// clang-format on

	/* Instance methods. */

	Request::Request(std::string method, Json& internal, Json& data/* = Json::object()*/)
		: method(method)
		, internal(internal)
		, data(data)
		, id(utils::randomNumber())
	{
		static const std::string JsonStringId{ "id" };
		static const std::string JsonStringMethod{ "method" };
		static const std::string JsonStringInternal{ "internal" };
		static const std::string JsonStringData{ "data" };

		auto it = Request::string2MethodId.find(this->method);

		if (it != Request::string2MethodId.end())
		{
			this->methodId = it->second;
		}
		else
		{
			throw Error(utils::Printf("unknown request.method '%s'", this->method.c_str()));
		}
	}

	Request::~Request()
	{
		
	}

	void Request::Accept()
	{
		static Json emptyData(Json::object());

		Accept(emptyData);
	}

	void Request::Accept(Json& data)
	{
		static Json emptyData(Json::object());
		static const std::string JsonStringId{ "id" };
		static const std::string JsonStringAccepted{ "accepted" };
		static const std::string JsonStringData{ "data" };

		//MS_ASSERT(!this->replied, "Request already replied");

		this->replied = true;

		Json json(Json::object());

		json[JsonStringId] = this->id;
		json[JsonStringAccepted] = true;

		if (data.is_object() || data.is_array())
			json[JsonStringData] = data;
		else
			json[JsonStringData] = emptyData;

		//this->channel->Send(json);
	}

	void Request::Reject(std::string& reason)
	{
		Reject(reason.c_str());
	}

	/**
	 * Reject the Request.
	 * @param reason  Description string.
	 */
	void Request::Reject(const char* reason)
	{

		static const std::string JsonStringId{ "id" };
		static const std::string JsonStringRejected{ "rejected" };
		static const std::string JsonStringReason{ "reason" };

		//MS_ASSERT(!this->replied, "Request already replied");

		this->replied = true;

		Json json(Json::object());

		json[JsonStringId] = this->id;
		json[JsonStringRejected] = true;

		if (reason != nullptr)
			json[JsonStringReason] = reason;

		//this->channel->Send(json);
	}

	Json Request::toJson()
	{
		Json request = {
			   { "id", id },
			   { "method", method },
			   { "internal", internal },
			   { "data", data }
		};

		return request;
	}

}

