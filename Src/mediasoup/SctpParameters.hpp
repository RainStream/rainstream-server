#pragma once 

#include "common.hpp"

struct NumSctpStreams
{
	/**
	 * Initially requested uint32_t of outgoing SCTP streams.
	 */
	uint32_t OS;

	/**
	 * Maximum uint32_t of incoming SCTP streams.
	 */
	uint32_t MIS;
};

struct SctpCapabilities
{
	NumSctpStreams numStreams;
};

/**
 * Both OS and MIS are part of the SCTP INIT+ACK handshake. OS refers to the
 * initial uint32_t of outgoing SCTP streams that the server side transport creates
 * (to be used by DataConsumers), while MIS refers to the maximum uint32_t of
 * incoming SCTP streams that the server side transport can receive (to be used
 * by DataProducers). So, if the server side transport will just be used to
 * create data producers (but no data consumers), OS can be low (~1). However,
 * if data consumers are desired on the server side transport, OS must have a
 * proper value and such a proper value depends on whether the remote endpoint
 * supports  SCTP_ADD_STREAMS extension or not.
 *
 * libwebrtc (Chrome, Safari, etc) does not enable SCTP_ADD_STREAMS so, if data
 * consumers are required,  OS should be 1024 (the maximum uint32_t of DataChannels
 * that libwebrtc enables).
 *
 * Firefox does enable SCTP_ADD_STREAMS so, if data consumers are required, OS
 * can be lower (16 for instance). The mediasoup transport will allocate and
 * announce more outgoing SCTM streams when needed.
 *
 * mediasoup-client provides specific per browser/version OS and MIS values via
 * the device.sctpCapabilities getter.
 */


struct SctpParameters
{
	/**
	 * Must always equal 5000.
	 */
	uint32_t port;

	/**
	 * Initially requested uint32_t of outgoing SCTP streams.
	 */
	uint32_t OS;

	/**
	 * Maximum uint32_t of incoming SCTP streams.
	 */
	uint32_t MIS;

	/**
	 * Maximum allowed size for SCTP messages.
	 */
	uint32_t maxMessageSize;
};

/**
 * SCTP stream parameters describe the reliability of a certain SCTP stream.
 * If ordered is true then maxPacketLifeTime and maxRetransmits must be
 * false.
 * If ordered if false, only one of maxPacketLifeTime or maxRetransmits
 * can be true.
 */
struct SctpStreamParameters
{
	/**
	 * SCTP stream id.
	 */
	uint32_t streamId;

	/**
	 * Whether data messages must be received in order. If true the messages will
	 * be sent reliably. Default true.
	 */
	bool ordered;

	/**
	 * When ordered is false indicates the time (in milliseconds) after which a
	 * SCTP packet will stop being retransmitted.
	 */
	uint32_t maxPacketLifeTime;

	/**
	 * When ordered is false indicates the maximum uint32_t of times a packet will
	 * be retransmitted.
	 */
	uint32_t maxRetransmits;
};
