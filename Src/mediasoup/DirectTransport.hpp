#pragma once 

#include "Transport.hpp"

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

class DirectTransport : public Transport
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
	task_t<json> getStats();

	/**
	 * NO-OP method in DirectTransport.
	 *
	 * @override
	 */
	task_t<void> connect();

	/**
	 * @override
	 */
	task_t<void> setMaxIncomingBitrate(uint32_t bitrate);

	/**
	 * @override
	 */
	task_t<void> setMaxOutgoingBitrate(uint32_t bitrate);

	virtual std::string typeName();

private:
	void _handleWorkerNotifications();

protected:
	json _data;
};
