#pragma once 

struct SctpCapabilities =
{
  numStreams: NumSctpStreams;
}

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
struct NumSctpStreams =
{
	/**
	 * Initially requested uint32_t of outgoing SCTP streams.
	 */
	OS: uint32_t;

	/**
	 * Maximum uint32_t of incoming SCTP streams.
	 */
	MIS: uint32_t;
}

struct SctpParameters =
{
	/**
	 * Must always equal 5000.
	 */
	port: uint32_t;

	/**
	 * Initially requested uint32_t of outgoing SCTP streams.
	 */
	OS: uint32_t;

	/**
	 * Maximum uint32_t of incoming SCTP streams.
	 */
	MIS: uint32_t;

	/**
	 * Maximum allowed size for SCTP messages.
	 */
	maxMessageSize: uint32_t;
}

/**
 * SCTP stream parameters describe the reliability of a certain SCTP stream.
 * If ordered is true then maxPacketLifeTime and maxRetransmits must be
 * false.
 * If ordered if false, only one of maxPacketLifeTime or maxRetransmits
 * can be true.
 */
struct SctpStreamParameters =
{
	/**
	 * SCTP stream id.
	 */
	streamId: uint32_t;

	/**
	 * Whether data messages must be received in order. If true the messages will
	 * be sent reliably. Default true.
	 */
	ordered?: bool;

	/**
	 * When ordered is false indicates the time (in milliseconds) after which a
	 * SCTP packet will stop being retransmitted.
	 */
	maxPacketLifeTime?: uint32_t;

	/**
	 * When ordered is false indicates the maximum uint32_t of times a packet will
	 * be retransmitted.
	 */
	maxRetransmits?: uint32_t;
}
