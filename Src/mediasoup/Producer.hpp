#pragma once 

#include "EnhancedEventEmitter.hpp"

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
	json rtpParameters;

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

class Channel;

class Producer : public EnhancedEventEmitter
{
public:
	/**
	 * @private
	 * @emits transportclose
	 * @emits score - (score: ProducerScore[])
	 * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
	 * @emits trace - (trace: ProducerTraceEventData)
	 * @emits @close
	 */
	Producer(json internal,
		json data,
		Channel* channel,
		json appData,
		bool paused
	);

	/**
	 * Producer id.
	 */
	std::string id();

	/**
	 * Whether the Producer is closed.
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
	 * Producer type.
	 */
	ProducerType type();

	/**
	 * Consumable RTP parameters.
	 *
	 * @private
	 */
	json consumableRtpParameters();

	/**
	 * Whether the Producer is paused.
	 */
	bool paused();

	/**
	 * Producer score list.
	 */
	std::vector<ProducerScore> score();

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
	 * @emits score - (score: ProducerScore[])
	 * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
	 * @emits trace - (trace: ProducerTraceEventData)
	 */
	EnhancedEventEmitter* observer();

	/**
	 * Close the Producer.
	 */
	void close();

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	void transportClosed();

	/**
	 * Dump Producer.
	 */
	std::future<json> dump();

	/**
	 * Get Producer stats.
	 */
	std::future<json> getStats();

	/**
	 * Pause the Producer.
	 */
	std::future<void> pause();

	/**
	 * Resume the Producer.
	 */
	std::future<void> resume();

	/**
	 * Enable "trace" event.
	 */
	std::future<void> enableTraceEvent(std::vector<ProducerTraceEventType> types);

private:
	void _handleWorkerNotifications();

	private:
		// Internal data.
		json _internal;		

		// Producer data.
		json _data;		

		// Channel instance.
		Channel* _channel;

		// Closed flag.
		bool _closed = false;

		// Custom app data.
		json _appData = json();

		// Paused flag.
		bool _paused = false;

		// Current score.
		std::vector<ProducerScore> _score;

		// Observer instance.
		EnhancedEventEmitter* _observer = new EnhancedEventEmitter();

};

