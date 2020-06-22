#pragma once 

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"
#include "Channel.hpp"
#include "RtpParameters.hpp"

struct ProducerOptions
{
	/**
	 * Producer id (just for Router.pipeToRouter() method).
	 */
	std::string id;

	/**
	 * Media kind ("audio" or "video").
	 */
	std::string kind;

	/**
	 * RTP parameters defining what the endpoint is sending.
	 */
	RtpParameters rtpParameters;

	/**
	 * Whether the producer must start in paused mode. Default false.
	 */
	bool paused;

	/**
	 * Just for video. Time (in ms) before asking the sender for a new key frame
	 * after having asked a previous one. Default 0.
	 */
	uint32_t keyFrameRequestDelay;

	/**
	 * Custom application data.
	 */
	json appData;
};

/**
 * Valid types for "trace" event.
 */
using ProducerTraceEventType = std::string ;// = "rtp" | "keyframe" | "nack" | "pli" | "fir";

/**
 * "trace" event data.
 */
struct ProducerTraceEventData
{
	/**
	 * Trace type.
	 */
	ProducerTraceEventType type;

	/**
	 * Event timestamp.
	 */
	uint32_t timestamp;

	/**
	 * Event direction.
	 */
	std::string direction;// "in" | "out";

	/**
	 * Per type information.
	 */
	json info;
};

struct ProducerScore
{
	/**
	 * SSRC of the RTP stream.
	 */
	uint32_t ssrc;

	/**
	 * RID of the RTP stream.
	 */
	std::string rid;

	/**
	 * The score of the RTP stream.
	 */
	uint32_t score;
};

struct ProducerVideoOrientation
{
	/**
	 * Whether the source is a video camera.
	 */
	bool camera;

	/**
	 * Whether the video source is flipped.
	 */
	bool flip;

	/**
	 * Rotation degrees (0, 90, 180 or 270).
	 */
	uint32_t rotation;
};

struct ProducerStat
{
	// Common to all RtpStreams.
	std::string type;
	uint32_t timestamp;
	uint32_t ssrc;
	uint32_t rtxSsrc;
	std::string rid;
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
	// RtpStreamRecv specific.
	uint32_t jitter;
	json bitrateByLayer;
};

/**
 * Producer type.
 */
using ProducerType = std::string; // = "simple" | "simulcast" | "svc";


class Producer : public EnhancedEventEmitter
{
	Logger* logger;
	// Internal data.
private:
	json _internal/*:
	{
		routerId: string;
		transportId: string;
		producerId: string;
	}*/;

	// Producer data.
	readonly _data:
	{
		kind: MediaKind;
		rtpParameters: RtpParameters;
		type: ProducerType;
		consumableRtpParameters: RtpParameters;
	};

	// Channel instance.
	Channel* _channel;

	// Closed flag.
	bool _closed = false;

	// Custom app data.
	readonly _appData?: any;

	// Paused flag.
	bool _paused = false;

	// Current score.
	_score: ProducerScore[] = [];

	// Observer instance.
	EnhancedEventEmitter* _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @emits transportclose
	 * @emits score - (score: ProducerScore[])
	 * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
	 * @emits trace - (trace: ProducerTraceEventData)
	 * @emits @close
	 */
	Producer(
		{
			internal,
			data,
			channel,
			appData,
			paused
		}
		: logger(new Logger("Producer"))

		{
			internal: any;
			data: any;
			channel: Channel;
			appData?: any;
			paused: bool;
		}
	)
	{
		super();

		logger->debug("constructor()");

		this->_internal = internal;
		this->_data = data;
		this->_channel = channel;
		this->_appData = appData;
		this->_paused = paused;

		this->_handleWorkerNotifications();
	}

	/**
	 * Producer id.
	 */
	std::string id()
	{
		return this->_internal.producerId;
	}

	/**
	 * Whether the Producer is closed.
	 */
	bool closed()
	{
		return this->_closed;
	}

	/**
	 * Media kind.
	 */
	get kind(): MediaKind
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
	 * Producer type.
	 */
	ProducerType type()
	{
		return this->_data.type;
	}

	/**
	 * Consumable RTP parameters.
	 *
	 * @private
	 */
	RtpParameters consumableRtpParameters()
	{
		return this->_data.consumableRtpParameters;
	}

	/**
	 * Whether the Producer is paused.
	 */
	bool paused()
	{
		return this->_paused;
	}

	/**
	 * Producer score list.
	 */
	get score(): ProducerScore[]
	{
		return this->_score;
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
	 * @emits pause
	 * @emits resume
	 * @emits score - (score: ProducerScore[])
	 * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
	 * @emits trace - (trace: ProducerTraceEventData)
	 */
	get observer(): EnhancedEventEmitter
	{
		return this->_observer;
	}

	/**
	 * Close the Producer.
	 */
	void close()
	{
		if (this->_closed)
			return;

		logger->debug("close()");

		this->_closed = true;

		// Remove notification subscriptions.
		this->_channel->removeAllListeners(this->_internal.producerId);

		this->_channel->request("producer.close", this->_internal)
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
		this->_channel->removeAllListeners(this->_internal.producerId);

		this->safeEmit("transportclose");

		// Emit observer event.
		this->_observer.safeEmit("close");
	}

	/**
	 * Dump Producer.
	 */
	async dump(): Promise<any>
	{
		logger->debug("dump()");

		return this->_channel->request("producer.dump", this->_internal);
	}

	/**
	 * Get Producer stats.
	 */
	async getStats(): Promise<ProducerStat[]>
	{
		logger->debug("getStats()");

		return this->_channel->request("producer.getStats", this->_internal);
	}

	/**
	 * Pause the Producer.
	 */
	async pause(): Promise<void>
	{
		logger->debug("pause()");

		const wasPaused = this->_paused;

		co_await this->_channel->request("producer.pause", this->_internal);

		this->_paused = true;

		// Emit observer event.
		if (!wasPaused)
			this->_observer.safeEmit("pause");
	}

	/**
	 * Resume the Producer.
	 */
	async resume(): Promise<void>
	{
		logger->debug("resume()");

		const wasPaused = this->_paused;

		co_await this->_channel->request("producer.resume", this->_internal);

		this->_paused = false;

		// Emit observer event.
		if (wasPaused)
			this->_observer.safeEmit("resume");
	}

	/**
	 * Enable "trace" event.
	 */
	async enableTraceEvent(types: ProducerTraceEventType[] = []): Promise<void>
	{
		logger->debug("enableTraceEvent()");

		const reqData = { types };

		co_await this->_channel->request(
			"producer.enableTraceEvent", this->_internal, reqData);
	}

	private _handleWorkerNotifications(): void
	{
		this->_channel->on(this->_internal.producerId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case "score":
				{
					const score = data as ProducerScore[];

					this->_score = score;

					this->safeEmit("score", score);

					// Emit observer event.
					this->_observer.safeEmit("score", score);

					break;
				}

				case "videoorientationchange":
				{
					const videoOrientation = data as ProducerVideoOrientation;

					this->safeEmit("videoorientationchange", videoOrientation);

					// Emit observer event.
					this->_observer.safeEmit("videoorientationchange", videoOrientation);

					break;
				}

				case "trace":
				{
					const trace = data as ProducerTraceEventData;

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

