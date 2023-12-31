#pragma once 

#include "EnhancedEventEmitter.h"

namespace mediasoup {

class Channel;
class PayloadChannel;

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
	std::string type;

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



/**
 * Producer type.
 */
using ProducerType = std::string; // = "simple" | "simulcast" | "svc";


class MS_EXPORT Producer : public EnhancedEventEmitter
{
public:
	Producer(json internal,
		json data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		json appData,
		bool paused
	);
	virtual ~Producer();
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
	json score();
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
	async_simple::coro::Lazy<json> dump();

	/**
	 * Get Producer stats.
	 */
	async_simple::coro::Lazy<json> getStats();

	/**
	 * Pause the Producer.
	 */
	async_simple::coro::Lazy<void> pause();

	/**
	 * Resume the Producer.
	 */
	async_simple::coro::Lazy<void> resume();

	/**
	 * Enable "trace" event.
	 */
	async_simple::coro::Lazy<void> enableTraceEvent(std::vector<std::string> types);

	//void send(rtpPacket);

private:
	void _handleWorkerNotifications();

private:
	// Internal data.
	json _internal;
	// Producer data.
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
	// Current score.
	json _score;
	// Observer instance.
	EnhancedEventEmitter* _observer;
};

}
