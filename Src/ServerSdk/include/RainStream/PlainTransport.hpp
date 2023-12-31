#pragma once
#include "Transport.hpp"

const logger = new Logger_1.Logger("PlainTransport");

class PlainTransport : public Transport {
    /**
     * @private
     * @emits tuple - (tuple: TransportTuple)
     * @emits rtcptuple - (rtcpTuple: TransportTuple)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
	PlainTransport(params) {
        super(params);
        logger.debug("constructor()");
        const { data } = params;
        this->_data =
            {
                rtcpMux: data.rtcpMux,
                comedia: data.comedia,
                tuple: data.tuple,
                rtcpTuple: data.rtcpTuple,
                sctpParameters: data.sctpParameters,
                sctpState: data.sctpState,
                srtpParameters: data.srtpParameters
            };
        this->_handleWorkerNotifications();
    }
    /**
     * Transport tuple.
     */
    get tuple() {
        return this->_data.tuple;
    }
    /**
     * Transport RTCP tuple.
     */
    get rtcpTuple() {
        return this->_data.rtcpTuple;
    }
    /**
     * SCTP parameters.
     */
    get sctpParameters() {
        return this->_data.sctpParameters;
    }
    /**
     * SCTP state.
     */
    get sctpState() {
        return this->_data.sctpState;
    }
    /**
     * SRTP parameters.
     */
    get srtpParameters() {
        return this->_data.srtpParameters;
    }
    /**
     * Observer.
     *
     * @override
     * @emits close
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (producer: Producer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     * @emits tuple - (tuple: TransportTuple)
     * @emits rtcptuple - (rtcpTuple: TransportTuple)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    get observer() {
        return this->_observer;
    }
    /**
     * Close the PlainTransport.
     *
     * @override
     */
    close() {
        if (this->_closed)
            return;
        if (this->_data.sctpState)
            this->_data.sctpState = "closed";
        super.close();
    }
    /**
     * Router was closed.
     *
     * @private
     * @override
     */
    routerClosed() {
        if (this->_closed)
            return;
        if (this->_data.sctpState)
            this->_data.sctpState = "closed";
        super.routerClosed();
    }
    /**
     * Get PlainTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug("getStats()");
        return this->_channel.request("transport.getStats", this->_internal);
    }
    /**
     * Provide the PlainTransport remote parameters.
     *
     * @override
     */
    async connect({ ip, port, rtcpPort, srtpParameters }) {
        logger.debug("connect()");
        const reqData = { ip, port, rtcpPort, srtpParameters };
        const data = await this->_channel.request("transport.connect", this->_internal, reqData);
        // Update data.
        if (data.tuple)
            this->_data.tuple = data.tuple;
        if (data.rtcpTuple)
            this->_data.rtcpTuple = data.rtcpTuple;
        this->_data.srtpParameters = data.srtpParameters;
    }
    _handleWorkerNotifications() {
        this->_channel.on(this->_internal.transportId, (event, data) => {
            switch (event) {
                case "tuple":
                    {
                        const tuple = data.tuple;
                        this->_data.tuple = tuple;
                        this->safeEmit("tuple", tuple);
                        // Emit observer event.
                        this->_observer.safeEmit("tuple", tuple);
                        break;
                    }
                case "rtcptuple":
                    {
                        const rtcpTuple = data.rtcpTuple;
                        this->_data.rtcpTuple = rtcpTuple;
                        this->safeEmit("rtcptuple", rtcpTuple);
                        // Emit observer event.
                        this->_observer.safeEmit("rtcptuple", rtcpTuple);
                        break;
                    }
                case "sctpstatechange":
                    {
                        const sctpState = data.sctpState;
                        this->_data.sctpState = sctpState;
                        this->safeEmit("sctpstatechange", sctpState);
                        // Emit observer event.
                        this->_observer.safeEmit("sctpstatechange", sctpState);
                        break;
                    }
                case "trace":
                    {
                        const trace = data;
                        this->safeEmit("trace", trace);
                        // Emit observer event.
                        this->_observer.safeEmit("trace", trace);
                        break;
                    }
                default:
                    {
                        logger.error("ignoring unknown event "%s"", event);
                    }
            }
        });
    }
};
/**
 * DEPRECATED: Use PlainTransport.
 */
class PlainRtpTransport : public PlainTransport {
	PlainRtpTransport(params) {
        super(params);
    }
};
