#pragma once

#include "EnhancedEventEmitter.hpp"

class Channel;
class PayloadChannel;

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
	 * When creating a video Consumer, it's recommended to set paused to true,
	 * then transmit the Consumer parameters to the consuming endpoint and, once
	 * the consuming endpoint has created its local side Consumer, unpause the
	 * server side Consumer using the resume() method. This is an optimization
	 * to make it possible for the consuming endpoint to render the video as far
	 * as possible. If the server side Consumer was created with paused: false,
	 * mediasoup will immediately request a key frame to the remote Producer and
	 * suych a key frame may reach the consuming endpoint even before it's ready
	 * to consume it, generating ��black�� video until the device requests a keyframe
	 * by itself.
	 */
	bool paused = false;

	/**
	 * The MID for the Consumer. If not specified, a sequentially growing
	 * number will be assigned.
	 */
	std::optional<std::string> mid;

	/**
	 * Preferred spatial and temporal layer for simulcast or SVC media sources.
	 * If unset, the highest ones are selected.
	 */
	json preferredLayers = {
		{ "spatialLayer", 0 },
		{ "temporalLayer", 0 }
	};

	/**
	 * Whether this Consumer should ignore DTX packets (only valid for Opus codec).
	 * If set, DTX packets are not forwarded to the remote Consumer.
	 */
	bool ignoreDtx = false;

	/**
	 * Whether this Consumer should consume all RTP streams generated by the
	 * Producer.
	 */
	bool pipe = false;

	/**
	 * Custom application data.
	 */
	json appData = json();
};

struct ConsumerLayers
{
	ConsumerLayers()
		: spatialLayer(0)
		, temporalLayer(0)
	{

	}

	ConsumerLayers(const ConsumerLayers& layers)
		: spatialLayer(layers.spatialLayer)
		, temporalLayer(layers.temporalLayer)
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


/**
 * Consumer type.
 */
using ConsumerType = std::string;// "simple" | "simulcast" | "svc" | "pipe";


class MS_EXPORT Consumer : public EnhancedEventEmitter
{
public:
	Consumer(
		json internal,
		json data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		json appData,
		bool paused,
		bool producerPaused,
		json score = json(),
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
	json score();

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
	 */
	EnhancedEventEmitter* observer();
	/**
	 * @private
	 * Just for testing purposes.
	 */
	Channel* channelForTesting();

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
	task_t<json> dump();

	/**
	 * Get Consumer stats.
	 */
	task_t<json> getStats();

	/**
	 * Pause the Consumer.
	 */
	task_t<void> pause();

	/**
	 * Resume the Consumer.
	 */
	task_t<void> resume();

	/**
	 * Set preferred video layers.
	 */
	task_t<void> setPreferredLayers(
		int spatialLayer,
		int temporalLayer
	);

	/**
	 * Set priority.
	 */
	task_t<void> setPriority(int priority);

	/**
	 * Unset priority.
	 */
	task_t<void> unsetPriority();

	/**
	 * Request a key frame to the Producer.
	 */
	task_t<void> requestKeyFrame();

	/**
	 * Enable "trace" event.
	 */
	task_t<void> enableTraceEvent(std::vector<std::string> types);

private:
	void _handleWorkerNotifications();

private:
	// Internal data.
	json _internal;
	// Consumer data.
	json _data;
	// Channel instance.
	Channel* _channel;
	// PayloadChannel instance.
	PayloadChannel* _payloadChannel;
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
	json _score;
	// Preferred layers.
	ConsumerLayers _preferredLayers;
	// Curent layers.
	ConsumerLayers _currentLayers;
	// Observer instance.
	EnhancedEventEmitter* _observer{ nullptr };
};

