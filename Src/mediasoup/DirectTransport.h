#pragma once 

#include "Transport.h"

namespace mediasoup {

class Channel;
class PayloadChannel;

struct DirectTransportOptions
{
	/**
	 * Maximum allowed size for direct messages sent from DataProducers.
	 * Default 262144.
	 */
	uint32_t maxMessageSize = 262144;

	/**
	 * Custom application data.
	 */
	json appData;
};

class MS_EXPORT DirectTransport : public Transport
{
public:
	/**
	 * @private
	 */
	DirectTransport(const json& internal,
		const json& data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		const json& appData,
		GetRouterRtpCapabilities getRouterRtpCapabilities,
		GetProducerById getProducerById,
		GetDataProducerById getDataProducerById);

	/**
	 * Close the DirectTransport.
	 *
	 * @override
	 */
	void close();

	/**
	 * Router was closed.
	 *
	 * @private
	 * @override
	 */
	void routerClosed();

	/**
	 * Get DirectTransport stats.
	 *
	 * @override
	 */
	async_simple::coro::Lazy<json> getStats();

	/**
	 * NO-OP method in DirectTransport.
	 *
	 * @override
	 */
	async_simple::coro::Lazy<void> connect();

	/**
	 * @override
	 */
	async_simple::coro::Lazy<void> setMaxIncomingBitrate(uint32_t bitrate);

	/**
	 * @override
	 */
	async_simple::coro::Lazy<void> setMaxOutgoingBitrate(uint32_t bitrate);

	virtual std::string typeName();

private:
	void _handleWorkerNotifications();

protected:
	json _data;
};

}
