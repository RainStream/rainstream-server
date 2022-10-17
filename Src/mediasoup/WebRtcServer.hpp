#pragma once

#include "EnhancedEventEmitter.hpp"
#include "Transport.hpp"

class Channel;
class WebRtcTransport;

struct WebRtcServerListenInfo
{
	/**
	 * Network protocol.
	 */
	TransportProtocol protocol;

	/**
	 * Listening IPv4 or IPv6.
	 */
	std::string ip;

	/**
	 * Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with
	 * private IP).
	 */
	std::optional<std::string> announcedIp;

	/**
	 * Listening port.
	 */
	uint16_t port;

	WebRtcServerListenInfo(const json& listenInfo)
	{
		this->protocol = listenInfo.value("protocol", "");
		this->ip = listenInfo.value("ip", "");
		if (listenInfo.contains("announcedIp"))
		{
			this->announcedIp = listenInfo.value("announcedIp", "");
		}
		this->port = listenInfo.value("port", 0);
	}

	operator json() const 
	{
		json data = {
			{ "protocol", protocol },
			{ "ip", ip },
			{ "port", port },
		};

		if (announcedIp.has_value())
			data["announcedIp"] = announcedIp.value();

		return data;
	}
};

struct WebRtcServerOptions
{
	WebRtcServerOptions(const json& options) {
		for (const json& listenInfo: options["listenInfos"])
		{
			this->listenInfos.push_back(WebRtcServerListenInfo(listenInfo));
		}
	}
	/**
	 * Listen infos.
	 */
	std::list<WebRtcServerListenInfo> listenInfos;

	/**
	 * Custom application data.
	 */
	json appData;
};


class MS_EXPORT WebRtcServer : public EnhancedEventEmitter
{
public:
	WebRtcServer(const json& internal, Channel* channel, const json& appData);

	/**
	 * WebRtcServer id.
	 */
	std::string id();

	/**
	 * Whether the WebRtcServer is closed.
	 */
	bool closed();

	/**
	 * App custom data.
	 */
	json appData();

	/**
	 * Invalid setter.
	 */
	void appData(json appData);

	/**
	 * @private
	 * Just for testing purposes.
	 */
	std::map<std::string, WebRtcTransport*> webRtcTransportsForTesting();

	/**
	 * Close the WebRtcServer.
	 */
	void  close();

	/**
	 * Worker was closed.
	 *
	 * @private
	 */
	void workerClosed();

	/**
	 * Dump WebRtcServer.
	 */
	task_t<json> dump();

	/**
	 * @private
	 */
	void  handleWebRtcTransport(WebRtcTransport* webRtcTransport);

private:
	// Internal data.
	json _internal;
	// Channel instance.
	Channel* _channel;
	// Closed flag.
	bool _closed = false;
	// Custom app data.
	json _appData;
	// Transports map.
	std::map<std::string, WebRtcTransport*> _webRtcTransports;
	// Observer instance.
	EnhancedEventEmitter* _observer;
};
