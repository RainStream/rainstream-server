#pragma once 

#include "Transport.hpp"

class Producer;
class Consumer;
class PayloadChannel;
struct ConsumerOptions;

class MS_EXPORT PipeTransport : public Transport
{
public:
	/**
	 * @private
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	PipeTransport(const json& internal,
		const json& data,
		Channel* channel,
		PayloadChannel* payloadChannel,
		const json& appData,
		GetRouterRtpCapabilities getRouterRtpCapabilities,
		GetProducerById getProducerById,
		GetDataProducerById getDataProducerById);

	/**
	 * Transport tuple.
	 */
	TransportTuple tuple();

	/**
	 * SCTP parameters.
	 */
	SctpParameters sctpParameters();

	/**
	 * SCTP state.
	 */
	SctpState sctpState();

	/**
	 * SRTP parameters.
	 */
	SrtpParameters srtpParameters();

	virtual std::string typeName();

	/**
	 * Observer.
	 *
	 * @override
	 * @emits close
	 * @emits newproducer - (producer: Producer)
	 * @emits newconsumer - (producer: Producer)
	 * @emits newdataproducer - (dataProducer: DataProducer)
	 * @emits newdataconsumer - (dataProducer: DataProducer)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	EnhancedEventEmitter* observer();

	/**
	 * Close the PipeTransport.
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
	 * Get PipeTransport stats.
	 *
	 * @override
	 */
	virtual task_t<json> getStats();

	/**
	 * Provide the PipeTransport remote parameters.
	 *
	 * @override
	 */
	task_t<void> connect(
		std::string ip,
		uint32_t port,
		SrtpParameters& srtpParameters
	);

	/**
	 * Create a pipe Consumer.
	 *
	 * @override
	 */
	task_t<Consumer*> consume(ConsumerOptions& options);

private:
	void _handleWorkerNotifications();

private:
	// PipeTransport data.
	json _data;

};
