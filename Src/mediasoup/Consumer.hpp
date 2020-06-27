#pragma once

#include "common.hpp"
#include "Logger.hpp"
#include "EnhancedEventEmitter.hpp"

class Channel;

struct ConsumerLayers
{
	ConsumerLayers()
		: spatialLayer(0)
		, temporalLayer(0)
	{

	}

	ConsumerLayers(const json& data)
		: ConsumerLayers()
	{
		if (data.is_object())
		{
			this->spatialLayer = data.value("spatialLayer", 0);
			this->temporalLayer = data.value("temporalLayer", 0);
		}
	}

	operator json() const
	{
		json data = {
			{ "spatialLayer", spatialLayer },
			{ "temporalLayer", temporalLayer }
		};

		return data;
	}
 	/**
 	 * The spatial layer index (from 0 to N).
 	 */
 	uint32_t spatialLayer;
 
 	/**
 	 * The temporal layer index (from 0 to N).
 	 */
 	uint32_t temporalLayer;
};

struct ConsumerOptions
{
	/**
	 * The id of the Producer to consume.
	 */
	std::string producerId;

	/**
	 * RTP capabilities of the consuming endpoint.
	 */
	json rtpCapabilities;

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
using ConsumerTraceEventType = std::string;// = "rtp" | "keyframe" | "nack" | "pli" | "fir";

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
	std::string direction;// : "in" | "out";

	/**
	 * Per type information.
	 */
	json info;
};

struct ConsumerScore
{
	ConsumerScore()
		: score(0)
		, producerScore(0)
	{

	}

	ConsumerScore(const json& data)
		: ConsumerScore()
	{
		if (data.is_object())
		{
			this->score = data.value("score", 0);
			this->producerScore = data.value("producerScore", 0);
			for (auto &item : data["producerScores"])
			{
				producerScores.push_back(item);
			}
		}
	}

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
};

/**
 * Consumer type.
 */
using ConsumerType = std::string;// "simple" | "simulcast" | "svc" | "pipe";


class Consumer : public EnhancedEventEmitter
{
public:
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
		json internal,
		json data,
		Channel* channel,
		json appData,
		bool paused,
		bool producerPaused,
		ConsumerScore score = ConsumerScore(),
		ConsumerLayers preferredLayers = ConsumerLayers());

	/**
	 * Consumer id.
	 */
	std::string id();

	/**
	 * Associated Producer id.
	 */
	std::string producerId();

	/**
	 * Whether the Consumer is closed.
	 */
	bool closed();

	/**
	 * Media kind.
	 */
	std::string kind();

	/**
	 * RTP parameters.
	 */
	json rtpParameters();

	/**
	 * Consumer type.
	 */
	ConsumerType type();

	/**
	 * Whether the Consumer is paused.
	 */
	bool paused();

	/**
	 * Whether the associate Producer is paused.
	 */
	bool producerPaused();

	/**
	 * Current priority.
	 */
	int priority();

	/**
	 * Consumer score.
	 */
	ConsumerScore score();

	/**
	 * Preferred video layers.
	 */
	ConsumerLayers preferredLayers();

	/**
	 * Current video layers.
	 */
	ConsumerLayers currentLayers();

	/**
	 * App custom data.
	 */
	json appData();

	/**
	 * Invalid setter.
	 */
	void appData(json appData);

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
	EnhancedEventEmitter* observer();

	/**
	 * Close the Consumer.
	 */
	void close();

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	void transportClosed();

	/**
	 * Dump Consumer.
	 */
	std::future<json> dump();

	/**
	 * Get Consumer stats.
	 */
	std::future<json> getStats();

	/**
	 * Pause the Consumer.
	 */
	std::future<void> pause();

	/**
	 * Resume the Consumer.
	 */
	std::future<void> resume();

	/**
	 * Set preferred video layers.
	 */
	std::future<void> setPreferredLayers(
		uint32_t spatialLayer,
		uint32_t temporalLayer
	);

	/**
	 * Set priority.
	 */
	std::future<void> setPriority(int priority);

	/**
	 * Unset priority.
	 */
	std::future<void> unsetPriority();

	/**
	 * Request a key frame to the Producer.
	 */
	std::future<void> requestKeyFrame();

	/**
	 * Enable "trace" event.
	 */
	std::future<void> enableTraceEvent(std::vector<ConsumerTraceEventType> types);

private:
	void _handleWorkerNotifications();

private:
	Logger* logger;
	// Internal data.
	json _internal;

	// Consumer data.
	json _data;

	// Channel instance.
	Channel* _channel;

	// Closed flag.
	bool _closed = false;

	// Custom app data.
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
};

