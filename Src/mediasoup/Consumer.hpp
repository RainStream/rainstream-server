#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
#include "Channel.hpp"
#include "Producer.hpp"
#include "RtpParameters.hpp"


struct ConsumerOptions
{
	/**
	 * The id of the Producer to consume.
	 */
	std::string producerId;

	/**
	 * RTP capabilities of the consuming endpoint.
	 */
	RtpCapabilities rtpCapabilities;

	/**
	 * Whether the Consumer must start in paused mode. Default false.
	 *
	 * When creating a video Consumer, it"s recommended to set paused to true,
	 * then transmit the Consumer parameters to the consuming endpoint and, once
	 * the consuming endpoint has created its local side Consumer, unpause the
	 * server side Consumer using the resume() method. This is an optimization
	 * to make it possible for the consuming endpoint to render the video as far
	 * as possible. If the server side Consumer was created with paused: false,
	 * mediasoup will immediately request a key frame to the remote Producer and
	 * suych a key frame may reach the consuming endpoint even before it"s ready
	 * to consume it, generating “black” video until the device requests a keyframe
	 * by itself.
	 */
	bool paused;

	/**
	 * Preferred spatial and temporal layer for simulcast or SVC media sources.
	 * If unset, the highest ones are selected.
	 */
	ConsumerLayers preferredLayers;

	/**
	 * Custom application data.
	 */
	json appData;
};

/**
 * Valid types for "trace" event.
 */
struct ConsumerTraceEventType = "rtp" | "keyframe" | "nack" | "pli" | "fir";

/**
 * "trace" event data.
 */
struct ConsumerTraceEventData
{
	/**
	 * Trace type.
	 */
	ConsumerTraceEventType type;

	/**
	 * Event timestamp.
	 */
	uint32_t timestamp;

	/**
	 * Event direction.
	 */
	std::string direction: "in" | "out";

	/**
	 * Per type information.
	 */
	json info;
};

struct ConsumerScore
{
	/**
	 * The score of the RTP stream of the consumer.
	 */
	uint32_t score;

	/**
	 * The score of the currently selected RTP stream of the producer.
	 */
	uint32_t producerScore;

	/**
	 * The scores of all RTP streams in the producer ordered by encoding (just
	 * useful when the producer uses simulcast).
	 */
	std::vector<uint32_t> producerScores;
};

struct ConsumerLayers
{
	/**
	 * The spatial layer index (from 0 to N).
	 */
	uint32_t spatialLayer;

	/**
	 * The temporal layer index (from 0 to N).
	 */
	uint32_t temporalLayer;
};

struct ConsumerStat
{
	// Common to all RtpStreams.
	std::string type;
	uint32_t timestamp;
	uint32_t ssrc;
	uint32_t rtxSsrc;
	std::string kind;
	std::string mimeType;
	uint32_t packetsLost;
	uint32_t fractionLost;
	uint32_t packetsDiscarded;
	uint32_t packetsRetransmitted;
	uint32_t packetsRepaired;
	uint32_t nackCount;
	uint32_t nackPacketCount;
	uint32_t pliCount;
	uint32_t firCount;
	uint32_t score;
	uint32_t packetCount;
	uint32_t byteCount;
	uint32_t bitrate;
	uint32_t roundTripTime;
}

/**
 * Consumer type.
 */
struct ConsumerType = "simple" | "simulcast" | "svc" | "pipe";

const Logger* logger = new Logger("Consumer");

class Consumer : public EnhancedEventEmitter
{
	// Internal data.
	private readonly _internal:
	{
		routerId: string;
		transportId: string;
		consumerId: string;
		producerId: string;
	};

	// Consumer data.
	private readonly _data:
	{
		kind: MediaKind;
		rtpParameters: RtpParameters;
		type: ConsumerType;
	};

	// Channel instance.
	Channel* _channel;

	// Closed flag.
	bool _closed = false;

	// Custom app data.
private:
	json _appData;

	// Paused flag.
	bool _paused = false;

	// Associated Producer paused flag.
	bool _producerPaused = false;

	// Current priority.
	uint32_t _priority = 1;

	// Current score.
	ConsumerScore _score;

	// Preferred layers.
	ConsumerLayers _preferredLayers;

	// Curent layers.
	ConsumerLayers _currentLayers;

	// Observer instance.
	EnhancedEventEmitter* _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @emits transportclose
	 * @emits producerclose
	 * @emits producerpause
	 * @emits producerresume
	 * @emits score - (score: ConsumerScore)
	 * @emits layerschange - (layers: ConsumerLayers | undefined)
	 * @emits trace - (trace: ConsumerTraceEventData)
	 * @emits @close
	 * @emits @producerclose
	 */
	Consumer(
		{
			internal,
			data,
			channel,
			appData,
			paused,
			producerPaused,
			score = { score: 10, producerScore: 10, producerScores: [] },
			preferredLayers
		}:
		{
			internal: any;
			data: any;
			channel: Channel;
			appData?: any;
			paused: bool;
			producerPaused: bool;
			score?: ConsumerScore;
			preferredLayers?: ConsumerLayers;
		})
	{
		super();

		logger->debug("constructor()");

		this->_internal = internal;
		this->_data = data;
		this->_channel = channel;
		this->_appData = appData;
		this->_paused = paused;
		this->_producerPaused = producerPaused;
		this->_score = score;
		this->_preferredLayers = preferredLayers;

		this->_handleWorkerNotifications();
	}

	/**
	 * Consumer id.
	 */
	std::string id()
	{
		return this->_internal.consumerId;
	}

	/**
	 * Associated Producer id.
	 */
	std::string producerId()
	{
		return this->_internal.producerId;
	}

	/**
	 * Whether the Consumer is closed.
	 */
	bool closed()
	{
		return this->_closed;
	}

	/**
	 * Media kind.
	 */
	MediaKind kind()
	{
		return this->_data.kind;
	}

	/**
	 * RTP parameters.
	 */
	RtpParameters rtpParameters()
	{
		return this->_data.rtpParameters;
	}

	/**
	 * Consumer type.
	 */
	ConsumerType type()
	{
		return this->_data.type;
	}

	/**
	 * Whether the Consumer is paused.
	 */
	bool paused()
	{
		return this->_paused;
	}

	/**
	 * Whether the associate Producer is paused.
	 */
	bool producerPaused()
	{
		return this->_producerPaused;
	}

	/**
	 * Current priority.
	 */
	int priority()
	{
		return this->_priority;
	}

	/**
	 * Consumer score.
	 */
	ConsumerScore score()
	{
		return this->_score;
	}

	/**
	 * Preferred video layers.
	 */
	get preferredLayers(): ConsumerLayers | undefined
	{
		return this->_preferredLayers;
	}

	/**
	 * Current video layers.
	 */
	get currentLayers(): ConsumerLayers | undefined
	{
		return this->_currentLayers;
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
	set appData(appData) // eslint-disable-line no-unused-vars
	{
		throw new Error("cannot override appData object");
	}

	/**
	 * Observer.
	 *
	 * @emits close
	 * @emits pause
	 * @emits resume
	 * @emits score - (score: ConsumerScore)
	 * @emits layerschange - (layers: ConsumerLayers | undefined)
	 * @emits trace - (trace: ConsumerTraceEventData)
	 */
	EnhancedEventEmitter* observer()
	{
		return this->_observer;
	}

	/**
	 * Close the Consumer.
	 */
	void close()
	{
		if (this->_closed)
			return;

		logger->debug("close()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal.consumerId);

		this->_channel->request("consumer.close", this->_internal)
			.catch(() => {});

		this->emit("@close");

		// Emit observer event.
		this->_observer.safeEmit("close");
	}

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	void transportClosed()
	{
		if (this->_closed)
			return;

		logger->debug("transportClosed()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal.consumerId);

		this->safeEmit("transportclose");

		// Emit observer event.
		this->_observer.safeEmit("close");
	}

	/**
	 * Dump Consumer.
	 */
	async dump(): Promise<any>
	{
		logger->debug("dump()");

		return this->_channel->request("consumer.dump", this->_internal);
	}

	/**
	 * Get Consumer stats.
	 */
	async getStats(): Promise<Array<ConsumerStat | ProducerStat>>
	{
		logger->debug("getStats()");

		return this->_channel->request("consumer.getStats", this->_internal);
	}

	/**
	 * Pause the Consumer.
	 */
	async pause(): Promise<void>
	{
		logger->debug("pause()");

		const wasPaused = this->_paused || this->_producerPaused;

		co_await this->_channel->request("consumer.pause", this->_internal);

		this->_paused = true;

		// Emit observer event.
		if (!wasPaused)
			this->_observer.safeEmit("pause");
	}

	/**
	 * Resume the Consumer.
	 */
	async resume(): Promise<void>
	{
		logger->debug("resume()");

		const wasPaused = this->_paused || this->_producerPaused;

		co_await this->_channel->request("consumer.resume", this->_internal);

		this->_paused = false;

		// Emit observer event.
		if (wasPaused && !this->_producerPaused)
			this->_observer.safeEmit("resume");
	}

	/**
	 * Set preferred video layers.
	 */
	async setPreferredLayers(
		ConsumerLayers{
			spatialLayer,
			temporalLayer
		}
	): Promise<void>
	{
		logger->debug("setPreferredLayers()");

		const reqData = { spatialLayer, temporalLayer };

		const data = co_await this->_channel->request(
			"consumer.setPreferredLayers", this->_internal, reqData);

		this->_preferredLayers = data || undefined;
	}

	/**
	 * Set priority.
	 */
	async setPriority(priority): Promise<void>
	{
		logger->debug("setPriority()");

		const reqData = { priority };

		const data = co_await this->_channel->request(
			"consumer.setPriority", this->_internal, reqData);

		this->_priority = data.priority;
	}

	/**
	 * Unset priority.
	 */
	async unsetPriority(): Promise<void>
	{
		logger->debug("unsetPriority()");

		const reqData = { priority: 1 };

		const data = co_await this->_channel->request(
			"consumer.setPriority", this->_internal, reqData);

		this->_priority = data.priority;
	}

	/**
	 * Request a key frame to the Producer.
	 */
	async requestKeyFrame(): Promise<void>
	{
		logger->debug("requestKeyFrame()");

		co_await this->_channel->request("consumer.requestKeyFrame", this->_internal);
	}

	/**
	 * Enable "trace" event.
	 */
	async enableTraceEvent(types: ConsumerTraceEventType[] = []): Promise<void>
	{
		logger->debug("enableTraceEvent()");

		const reqData = { types };

		co_await this->_channel->request(
			"consumer.enableTraceEvent", this->_internal, reqData);
	}

	private _handleWorkerNotifications(): void
	{
		this->_channel->on(this->_internal.consumerId, (std::string event, json data) =>
		{
			switch (event)
			{
				case "producerclose":
				{
					if (this->_closed)
						break;

					this->_closed = true;

					// Remove notification subscriptions.
					this->_channel->removeAllListeners(this->_internal.consumerId);

					this->emit("@producerclose");
					this->safeEmit("producerclose");

					// Emit observer event.
					this->_observer.safeEmit("close");

					break;
				}

				case "producerpause":
				{
					if (this->_producerPaused)
						break;

					const wasPaused = this->_paused || this->_producerPaused;

					this->_producerPaused = true;

					this->safeEmit("producerpause");

					// Emit observer event.
					if (!wasPaused)
						this->_observer.safeEmit("pause");

					break;
				}

				case "producerresume":
				{
					if (!this->_producerPaused)
						break;

					const wasPaused = this->_paused || this->_producerPaused;

					this->_producerPaused = false;

					this->safeEmit("producerresume");

					// Emit observer event.
					if (wasPaused && !this->_paused)
						this->_observer.safeEmit("resume");

					break;
				}

				case "score":
				{
					const score = data as ConsumerScore;

					this->_score = score;

					this->safeEmit("score", score);

					// Emit observer event.
					this->_observer.safeEmit("score", score);

					break;
				}

				case "layerschange":
				{
					const layers = data as ConsumerLayers | undefined;

					this->_currentLayers = layers;

					this->safeEmit("layerschange", layers);

					// Emit observer event.
					this->_observer.safeEmit("layerschange", layers);

					break;
				}

				case "trace":
				{
					const trace = data as ConsumerTraceEventData;

					this->safeEmit("trace", trace);

					// Emit observer event.
					this->_observer.safeEmit("trace", trace);

					break;
				}

				default:
				{
					logger->error("ignoring unknown event \"%s\"", event);
				}
			}
		});
	}
};

