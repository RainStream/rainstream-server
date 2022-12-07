#pragma once 

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
	SctpParameters()
		: port(5000)
		, OS(0)
		, MIS(0)
		, maxMessageSize(0)
	{

	}

	SctpParameters(const json& data)
		: SctpParameters()
	{
		if (data.is_object())
		{
			port = data.value("port", port);
			OS = data.value("OS", OS);
			MIS = data.value("MIS", MIS);
			maxMessageSize = data.value("maxMessageSize", maxMessageSize);
		}
	}

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

	SctpStreamParameters(const json& data)
	{
		if (data.is_object())
		{
			streamId = data.value("port", streamId);
			ordered = data.value("OS", ordered);
			maxPacketLifeTime = data.value("MIS", maxPacketLifeTime);
			maxRetransmits = data.value("maxMessageSize", maxRetransmits);
		}
	}

	//operator json() const
	//{
	//	json data = 
	//	{
	//		{ "streamId", streamId },
	//		{ "ordered", ordered },
	//		{ "maxPacketLifeTime", maxPacketLifeTime },
	//		{ "maxRetransmits", maxRetransmits },
	//	};

	//	return data;
	//}

	/**
	 * SCTP stream id.
	 */
	uint32_t streamId;

	/**
	 * Whether data messages must be received in order. If true the messages will
	 * be sent reliably. Default true.
	 */
	bool ordered = true;

	/**
	 * When ordered is false indicates the time (in milliseconds) after which a
	 * SCTP packet will stop being retransmitted.
	 */
	int32_t maxPacketLifeTime = 0;

	/**
	 * When ordered is false indicates the maximum uint32_t of times a packet will
	 * be retransmitted.
	 */
	int32_t maxRetransmits = 0;
};
