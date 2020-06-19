#pragma once 

// import { v4 as uuidv4 } from "uuid";
// import { AwaitQueue } from "awaitqueue";
#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
#include "ortc.hpp"
#include "errors.hpp"
#include "Channel.hpp"
#include "PayloadChannel.hpp"
#include "Transport.hpp"
#include "WebRtcTransport.hpp"
#include "PlainTransport.hpp"
#include "PipeTransport.hpp"
#include "DirectTransport.hpp"
#include "Producer.hpp"
#include "Consumer.hpp"
#include "DataProducer.hpp"
#include "DataConsumer.hpp"
#include "RtpObserver.hpp"
#include "AudioLevelObserver.hpp"
#include "RtpParameters.hpp"
#include "SctpParameters.hpp"


struct RouterOptions
{
	/**
	 * Router media codecs.
	 */
	std::vector<RtpCodecCapability> mediaCodecs;

	/**
	 * Custom application data.
	 */
	json appData;
};

struct PipeToRouterOptions
{
	/**
	 * The id of the Producer to consume.
	 */
	producerId?: std::string;

	/**
	 * The id of the DataProducer to consume.
	 */
	dataProducerId?: std::string;

	/**
	 * Target Router instance.
	 */
	router: Router;

	/**
	 * IP used in the PipeTransport pair. Default "127.0.0.1".
	 */
	listenIp?: TransportListenIp | std::string;

	/**
	 * Create a SCTP association. Default false.
	 */
	enableSctp?: bool;

	/**
	 * SCTP streams uint32_t.
	 */
	numSctpStreams?: NumSctpStreams;

	/**
	 * Enable RTX and NACK for RTP retransmission.
	 */
	enableRtx?: bool;

	/**
	 * Enable SRTP.
	 */
	enableSrtp?: bool;
};

struct PipeToRouterResult
{
	/**
	 * The Consumer created in the current Router.
	 */
	pipeConsumer?: Consumer;

	/**
	 * The Producer created in the target Router.
	 */
	pipeProducer?: Producer;

	/**
	 * The DataConsumer created in the current Router.
	 */
	pipeDataConsumer?: DataConsumer;

	/**
	 * The DataProducer created in the target Router.
	 */
	pipeDataProducer?: DataProducer;
};

const Logger* logger = new Logger("Router");

class Router : public EnhancedEventEmitter
{
	// Internal data.
	private readonly _internal:
	{
		routerId: std::string;
	};

	// Router data.
	private readonly _data:
	{
		rtpCapabilities: RtpCapabilities;
	}

	// Channel instance.
	private readonly _channel: Channel;

	// PayloadChannel instance.
	private readonly _payloadChannel: PayloadChannel;

	// Closed flag.
	private _closed = false;

	// Custom app data.
	private readonly _appData?: any;

	// Transports map.
	std::map<std::string, Transport*> _transports;

	// Producers map.
	std::map<std::string, Producer*> _producers;

	// RtpObservers map.
	std::map<std::string, RtpObserver*> _rtpObservers;

	// DataProducers map.
	std::map<std::string, DataProducer*> _dataProducers;

	// Router to PipeTransport map.
	std::map<Router*, PipeTransport*> _mapRouterPipeTransports;

	// AwaitQueue instance to make pipeToRouter tasks happen sequentially.
	private readonly _pipeToRouterQueue =
		new AwaitQueue({ ClosedErrorClass: InvalidStateError });

	// Observer instance.
	EnhancedEventEmitter* _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @emits workerclose
	 * @emits @close
	 */
	Router(
		{
			internal,
			data,
			channel,
			payloadChannel,
			appData
		}:
		{
			internal: any;
			data: any;
			channel: Channel;
			payloadChannel: PayloadChannel;
			appData?: any;
		}
	)
	{
		super();

		logger->debug("constructor()");

		this->_internal = internal;
		this->_data = data;
		this->_channel = channel;
		this->_payloadChannel = payloadChannel;
		this->_appData = appData;
	}

	/**
	 * Router id.
	 */
	std::std::string id()
	{
		return this->_internal.routerId;
	}

	/**
	 * Whether the Router is closed.
	 */
	bool closed()
	{
		return this->_closed;
	}

	/**
	 * RTC capabilities of the Router.
	 */
	RtpCapabilities rtpCapabilities()
	{
		return this->_data.rtpCapabilities;
	}

	/**
	 * App custom data.
	 */
	json appData()
	{
		return this->_appData;
	}

	/**
	 * Invalid setter.
	 */
	set appData(json appData) // eslint-disable-line no-unused-vars
	{
		throw new Error("cannot override appData object");
	}

	/**
	 * Observer.
	 *
	 * @emits close
	 * @emits newtransport - (transport: Transport)
	 * @emits newrtpobserver - (rtpObserver: RtpObserver)
	 */
	EnhancedEventEmitter* observer()
	{
		return this->_observer;
	}

	/**
	 * Close the Router.
	 */
	void close()
	{
		if (this->_closed)
			return;

		logger->debug("close()");

		this->_closed = true;

		this->_channel->request("router.close", this->_internal)
			.catch(() => {});

		// Close every Transport.
		for (const transport : this->_transports.values())
		{
			transport.routerClosed();
		}
		this->_transports.clear();

		// Clear the Producers map.
		this->_producers.clear();

		// Close every RtpObserver.
		for (const rtpObserver : this->_rtpObservers.values())
		{
			rtpObserver.routerClosed();
		}
		this->_rtpObservers.clear();

		// Clear the DataProducers map.
		this->_dataProducers.clear();

		// Clear map of Router/PipeTransports.
		this->_mapRouterPipeTransports.clear();

		// Close the pipeToRouter AwaitQueue instance.
		this->_pipeToRouterQueue.close();

		this->emit("@close");

		// Emit observer event.
		this->_observer.safeEmit("close");
	}

	/**
	 * Worker was closed.
	 *
	 * @private
	 */
	void workerClosed()
	{
		if (this->_closed)
			return;

		logger->debug("workerClosed()");

		this->_closed = true;

		// Close every Transport.
		for (const transport of this->_transports.values())
		{
			transport.routerClosed();
		}
		this->_transports.clear();

		// Clear the Producers map.
		this->_producers.clear();

		// Close every RtpObserver.
		for (const rtpObserver of this->_rtpObservers.values())
		{
			rtpObserver.routerClosed();
		}
		this->_rtpObservers.clear();

		// Clear the DataProducers map.
		this->_dataProducers.clear();

		// Clear map of Router/PipeTransports.
		this->_mapRouterPipeTransports.clear();

		this->safeEmit("workerclose");

		// Emit observer event.
		this->_observer.safeEmit("close");
	}

	/**
	 * Dump Router.
	 */
	async dump(): Promise<any>
	{
		logger->debug("dump()");

		return this->_channel->request("router.dump", this->_internal);
	}

	/**
	 * Create a WebRtcTransport.
	 */
	async createWebRtcTransport(
		{
			listenIps,
			enableUdp = true,
			enableTcp = false,
			preferUdp = false,
			preferTcp = false,
			initialAvailableOutgoingBitrate = 600000,
			enableSctp = false,
			numSctpStreams = { OS: 1024, MIS: 1024 },
			maxSctpMessageSize = 262144,
			appData = {}
		}: WebRtcTransportOptions
	): Promise<WebRtcTransport>
	{
		logger->debug("createWebRtcTransport()");

		if (!Array.isArray(listenIps))
			throw new TypeError("missing listenIps");
		else if (appData && typeof appData !== "object")
			throw new TypeError("if given, appData must be an object");

		listenIps = listenIps.map((listenIp) =>
		{
			if (typeof listenIp === "std::string" && listenIp)
			{
				return { ip: listenIp };
			}
			else if (typeof listenIp === "object")
			{
				return {
					ip          : listenIp.ip,
					announcedIp : listenIp.announcedIp || undefined
				};
			}
			else
			{
				throw new TypeError("wrong listenIp");
			}
		});

		const internal = { ...this->_internal, transportId: uuidv4() };
		const reqData = {
			listenIps,
			enableUdp,
			enableTcp,
			preferUdp,
			preferTcp,
			initialAvailableOutgoingBitrate,
			enableSctp,
			numSctpStreams,
			maxSctpMessageSize,
			isDataChannel : true
		};

		const data =
			co_await this->_channel->request("router.createWebRtcTransport", internal, reqData);

		const transport = new WebRtcTransport(
			{
				internal,
				data,
				channel                  : this->_channel,
				payloadChannel           : this->_payloadChannel,
				appData,
				getRouterRtpCapabilities : (): RtpCapabilities => this->_data.rtpCapabilities,
				getProducerById          : (producerId: std::string): Producer | undefined => (
					this->_producers.get(producerId)
				),
				getDataProducerById : (dataProducerId: std::string): DataProducer | undefined => (
					this->_dataProducers.get(dataProducerId)
				)
			});

		this->_transports.set(transport.id, transport);
		transport.on("@close", () => this->_transports.delete(transport.id));
		transport.on("@newproducer", (producer: Producer) => this->_producers.set(producer.id, producer));
		transport.on("@producerclose", (producer: Producer) => this->_producers.delete(producer.id));
		transport.on("@newdataproducer", (dataProducer: DataProducer) => (
			this->_dataProducers.set(dataProducer.id, dataProducer)
		));
		transport.on("@dataproducerclose", (dataProducer: DataProducer) => (
			this->_dataProducers.delete(dataProducer.id)
		));

		// Emit observer event.
		this->_observer.safeEmit("newtransport", transport);

		return transport;
	}

	/**
	 * Create a PlainTransport.
	 */
	async createPlainTransport(
		{
			listenIp,
			rtcpMux = true,
			comedia = false,
			enableSctp = false,
			numSctpStreams = { OS: 1024, MIS: 1024 },
			maxSctpMessageSize = 262144,
			enableSrtp = false,
			srtpCryptoSuite = "AES_CM_128_HMAC_SHA1_80",
			appData = {}
		}: PlainTransportOptions
	): Promise<PlainTransport>
	{
		logger->debug("createPlainTransport()");

		if (!listenIp)
			throw new TypeError("missing listenIp");
		else if (appData && typeof appData !== "object")
			throw new TypeError("if given, appData must be an object");

		if (typeof listenIp === "std::string" && listenIp)
		{
			listenIp = { ip: listenIp };
		}
		else if (typeof listenIp === "object")
		{
			listenIp =
			{
				ip          : listenIp.ip,
				announcedIp : listenIp.announcedIp || undefined
			};
		}
		else
		{
			throw new TypeError("wrong listenIp");
		}

		const internal = { ...this->_internal, transportId: uuidv4() };
		const reqData = {
			listenIp,
			rtcpMux,
			comedia,
			enableSctp,
			numSctpStreams,
			maxSctpMessageSize,
			isDataChannel : false,
			enableSrtp,
			srtpCryptoSuite
		};

		const data =
			co_await this->_channel->request("router.createPlainTransport", internal, reqData);

		const transport = new PlainTransport(
			{
				internal,
				data,
				channel                  : this->_channel,
				payloadChannel           : this->_payloadChannel,
				appData,
				getRouterRtpCapabilities : (): RtpCapabilities => this->_data.rtpCapabilities,
				getProducerById          : (producerId: std::string): Producer | undefined => (
					this->_producers.get(producerId)
				),
				getDataProducerById : (dataProducerId: std::string): DataProducer | undefined => (
					this->_dataProducers.get(dataProducerId)
				)
			});

		this->_transports.set(transport.id, transport);
		transport.on("@close", () => this->_transports.delete(transport.id));
		transport.on("@newproducer", (producer: Producer) => this->_producers.set(producer.id, producer));
		transport.on("@producerclose", (producer: Producer) => this->_producers.delete(producer.id));
		transport.on("@newdataproducer", (dataProducer: DataProducer) => (
			this->_dataProducers.set(dataProducer.id, dataProducer)
		));
		transport.on("@dataproducerclose", (dataProducer: DataProducer) => (
			this->_dataProducers.delete(dataProducer.id)
		));

		// Emit observer event.
		this->_observer.safeEmit("newtransport", transport);

		return transport;
	}

	/**
	 * DEPRECATED: Use createPlainTransport().
	 */
	async createPlainRtpTransport(
		options: PlainTransportOptions
	): Promise<PlainTransport>
	{
		logger->warn(
			"createPlainRtpTransport() is DEPRECATED, use createPlainTransport()");

		return this->createPlainTransport(options);
	}

	/**
	 * Create a PipeTransport.
	 */
	async createPipeTransport(
		{
			listenIp,
			enableSctp = false,
			numSctpStreams = { OS: 1024, MIS: 1024 },
			maxSctpMessageSize = 1073741823,
			enableRtx = false,
			enableSrtp = false,
			appData = {}
		}: PipeTransportOptions
	): Promise<PipeTransport>
	{
		logger->debug("createPipeTransport()");

		if (!listenIp)
			throw new TypeError("missing listenIp");
		else if (appData && typeof appData !== "object")
			throw new TypeError("if given, appData must be an object");

		if (typeof listenIp === "std::string" && listenIp)
		{
			listenIp = { ip: listenIp };
		}
		else if (typeof listenIp === "object")
		{
			listenIp =
			{
				ip          : listenIp.ip,
				announcedIp : listenIp.announcedIp || undefined
			};
		}
		else
		{
			throw new TypeError("wrong listenIp");
		}

		const internal = { ...this->_internal, transportId: uuidv4() };
		const reqData = {
			listenIp,
			enableSctp,
			numSctpStreams,
			maxSctpMessageSize,
			isDataChannel : false,
			enableRtx,
			enableSrtp
		};

		json data =
			co_await this->_channel->request("router.createPipeTransport", internal, reqData);

		PipeTransport* transport = new PipeTransport(
			{
				internal,
				data,
				channel                  : this->_channel,
				payloadChannel           : this->_payloadChannel,
				appData,
				getRouterRtpCapabilities : (): RtpCapabilities => this->_data.rtpCapabilities,
				getProducerById          : (producerId: std::string): Producer | undefined => (
					this->_producers.get(producerId)
				),
				getDataProducerById : (dataProducerId: std::string): DataProducer | undefined => (
					this->_dataProducers.get(dataProducerId)
				)
			});

		this->_transports.set(transport.id, transport);
		transport.on("@close", () => this->_transports.delete(transport.id));
		transport.on("@newproducer", (producer: Producer) => this->_producers.set(producer.id, producer));
		transport.on("@producerclose", (producer: Producer) => this->_producers.delete(producer.id));
		transport.on("@newdataproducer", (dataProducer: DataProducer) => (
			this->_dataProducers.set(dataProducer.id, dataProducer)
		));
		transport.on("@dataproducerclose", (dataProducer: DataProducer) => (
			this->_dataProducers.delete(dataProducer.id)
		));

		// Emit observer event.
		this->_observer.safeEmit("newtransport", transport);

		return transport;
	}

	/**
	 * Create a DirectTransport.
	 */
	async createDirectTransport(
		{
			maxMessageSize = 262144,
			appData = {}
		}: DirectTransportOptions =
		{
			maxMessageSize : 262144
		}
	): Promise<DirectTransport>
	{
		logger->debug("createDirectTransport()");

		const internal = { ...this->_internal, transportId: uuidv4() };
		const reqData = { direct: true, maxMessageSize };

		const data =
			co_await this->_channel->request("router.createDirectTransport", internal, reqData);

		const transport = new DirectTransport(
			{
				internal,
				data,
				channel                  : this->_channel,
				payloadChannel           : this->_payloadChannel,
				appData,
				getRouterRtpCapabilities : (): RtpCapabilities => this->_data.rtpCapabilities,
				getProducerById          : (producerId: std::string): Producer | undefined => (
					this->_producers.get(producerId)
				),
				getDataProducerById : (dataProducerId: std::string): DataProducer | undefined => (
					this->_dataProducers.get(dataProducerId)
				)
			});

		this->_transports.set(transport.id, transport);
		transport.on("@close", () => this->_transports.delete(transport.id));
		transport.on("@newproducer", (producer: Producer) => this->_producers.set(producer.id, producer));
		transport.on("@producerclose", (producer: Producer) => this->_producers.delete(producer.id));
		transport.on("@newdataproducer", (dataProducer: DataProducer) => (
			this->_dataProducers.set(dataProducer.id, dataProducer)
		));
		transport.on("@dataproducerclose", (dataProducer: DataProducer) => (
			this->_dataProducers.delete(dataProducer.id)
		));

		// Emit observer event.
		this->_observer.safeEmit("newtransport", transport);

		return transport;
	}

	/**
	 * Pipes the given Producer or DataProducer into another Router in same host.
	 */
	async pipeToRouter(
		{
			producerId,
			dataProducerId,
			router,
			listenIp = "127.0.0.1",
			enableSctp = true,
			numSctpStreams = { OS: 1024, MIS: 1024 },
			enableRtx = false,
			enableSrtp = false
		}: PipeToRouterOptions
	): Promise<PipeToRouterResult>
	{
		if (!producerId && !dataProducerId)
			throw new TypeError("missing producerId or dataProducerId");
		else if (producerId && dataProducerId)
			throw new TypeError("just producerId or dataProducerId can be given");
		else if (!router)
			throw new TypeError("Router not found");
		else if (router === this)
			throw new TypeError("cannot use this Router as destination");

		let producer: Producer | undefined;
		let dataProducer: DataProducer | undefined;

		if (producerId)
		{
			producer = this->_producers.get(producerId);

			if (!producer)
				throw new TypeError("Producer not found");
		}
		else if (dataProducerId)
		{
			dataProducer = this->_dataProducers.get(dataProducerId);

			if (!dataProducer)
				throw new TypeError("DataProducer not found");
		}

		// Here we may have to create a new PipeTransport pair to connect source and
		// destination Routers. We just want to keep a PipeTransport pair for each
		// pair of Routers. Since this operation is async, it may happen that two
		// simultaneous calls to router1.pipeToRouter({ producerId: xxx, router: router2 })
		// would end up generating two pairs of PipeTranports. To prevent that, let"s
		// use an async queue.

		let localPipeTransport: PipeTransport | undefined;
		let remotePipeTransport: PipeTransport | undefined;

		co_await this->_pipeToRouterQueue.push(async () =>
		{
			let pipeTransportPair = this->_mapRouterPipeTransports.get(router);

			if (pipeTransportPair)
			{
				localPipeTransport = pipeTransportPair[0];
				remotePipeTransport = pipeTransportPair[1];
			}
			else
			{
				try
				{
					pipeTransportPair = co_await Promise.all(
						[
							this->createPipeTransport(
								{ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp }),

							router.createPipeTransport(
								{ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp })
						]);

					localPipeTransport = pipeTransportPair[0];
					remotePipeTransport = pipeTransportPair[1];

					co_await Promise.all(
						[
							localPipeTransport.connect(
								{
									ip             : remotePipeTransport.tuple.localIp,
									port           : remotePipeTransport.tuple.localPort,
									srtpParameters : remotePipeTransport.srtpParameters
								}),

							remotePipeTransport.connect(
								{
									ip             : localPipeTransport.tuple.localIp,
									port           : localPipeTransport.tuple.localPort,
									srtpParameters : localPipeTransport.srtpParameters
								})
						]);

					localPipeTransport.observer.on("close", () =>
					{
						remotePipeTransport!.close();
						this->_mapRouterPipeTransports.delete(router);
					});

					remotePipeTransport.observer.on("close", () =>
					{
						localPipeTransport!.close();
						this->_mapRouterPipeTransports.delete(router);
					});

					this->_mapRouterPipeTransports.set(
						router, [ localPipeTransport, remotePipeTransport ]);
				}
				catch (error)
				{
					logger->error(
						"pipeToRouter() | error creating PipeTransport pair:%o",
						error);

					if (localPipeTransport)
						localPipeTransport.close();

					if (remotePipeTransport)
						remotePipeTransport.close();

					throw error;
				}
			}
		});

		if (producer)
		{
			let pipeConsumer: Consumer | undefined;
			let pipeProducer: Producer | undefined;

			try
			{
				pipeConsumer = co_await localPipeTransport!.consume(
					{
						producerId : producerId!
					});

				pipeProducer = co_await remotePipeTransport!.produce(
					{
						id            : producer.id,
						kind          : pipeConsumer!.kind,
						rtpParameters : pipeConsumer!.rtpParameters,
						paused        : pipeConsumer!.producerPaused,
						appData       : producer.appData
					});

				// Pipe events from the pipe Consumer to the pipe Producer.
				pipeConsumer!.observer.on("close", () => pipeProducer!.close());
				pipeConsumer!.observer.on("pause", () => pipeProducer!.pause());
				pipeConsumer!.observer.on("resume", () => pipeProducer!.resume());

				// Pipe events from the pipe Producer to the pipe Consumer.
				pipeProducer.observer.on("close", () => pipeConsumer!.close());

				return { pipeConsumer, pipeProducer };
			}
			catch (error)
			{
				logger->error(
					"pipeToRouter() | error creating pipe Consumer/Producer pair:%o",
					error);

				if (pipeConsumer)
					pipeConsumer.close();

				if (pipeProducer)
					pipeProducer.close();

				throw error;
			}
		}
		else if (dataProducer)
		{
			let pipeDataConsumer: DataConsumer | undefined;
			let pipeDataProducer: DataProducer | undefined;

			try
			{
				pipeDataConsumer = co_await localPipeTransport!.consumeData(
					{
						dataProducerId : dataProducerId!
					});

				pipeDataProducer = co_await remotePipeTransport!.produceData(
					{
						id                   : dataProducer.id,
						sctpStreamParameters : pipeDataConsumer!.sctpStreamParameters,
						label                : pipeDataConsumer!.label,
						protocol             : pipeDataConsumer!.protocol,
						appData              : dataProducer.appData
					});

				// Pipe events from the pipe DataConsumer to the pipe DataProducer.
				pipeDataConsumer!.observer.on("close", () => pipeDataProducer!.close());

				// Pipe events from the pipe DataProducer to the pipe DataConsumer.
				pipeDataProducer.observer.on("close", () => pipeDataConsumer!.close());

				return { pipeDataConsumer, pipeDataProducer };
			}
			catch (error)
			{
				logger->error(
					"pipeToRouter() | error creating pipe DataConsumer/DataProducer pair:%o",
					error);

				if (pipeDataConsumer)
					pipeDataConsumer.close();

				if (pipeDataProducer)
					pipeDataProducer.close();

				throw error;
			}
		}
		else
		{
			throw new Error("internal error");
		}
	}

	/**
	 * Create an AudioLevelObserver.
	 */
	async createAudioLevelObserver(
		{
			maxEntries = 1,
			threshold = -80,
			interval = 1000,
			appData = {}
		}: AudioLevelObserverOptions = {}
	): Promise<AudioLevelObserver>
	{
		logger->debug("createAudioLevelObserver()");

		if (appData && typeof appData !== "object")
			throw new TypeError("if given, appData must be an object");

		const internal = { ...this->_internal, rtpObserverId: uuidv4() };
		const reqData = { maxEntries, threshold, interval };

		co_await this->_channel->request("router.createAudioLevelObserver", internal, reqData);

		const audioLevelObserver = new AudioLevelObserver(
			{
				internal,
				channel         : this->_channel,
				payloadChannel  : this->_payloadChannel,
				appData,
				getProducerById : (producerId: std::string): Producer | undefined => (
					this->_producers.get(producerId)
				)
			});

		this->_rtpObservers.set(audioLevelObserver.id, audioLevelObserver);
		audioLevelObserver.on("@close", () =>
		{
			this->_rtpObservers.delete(audioLevelObserver.id);
		});

		// Emit observer event.
		this->_observer.safeEmit("newrtpobserver", audioLevelObserver);

		return audioLevelObserver;
	}

	/**
	 * Check whether the given RTP capabilities can consume the given Producer.
	 */
	bool canConsume(
		{
			producerId,
			rtpCapabilities
		}:
		{
			producerId: std::string;
			rtpCapabilities: RtpCapabilities;
		}
	)
	{
		const producer = this->_producers.get(producerId);

		if (!producer)
		{
			logger->error(
				"canConsume() | Producer with id \"%s\" not found", producerId);

			return false;
		}

		try
		{
			return ortc.canConsume(producer.consumableRtpParameters, rtpCapabilities);
		}
		catch (error)
		{
			logger->error("canConsume() | unexpected error: %s", String(error));

			return false;
		}
	}
};
