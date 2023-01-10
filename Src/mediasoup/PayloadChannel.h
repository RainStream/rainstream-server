#pragma once 

#include "EnhancedEventEmitter.h"

namespace mediasoup {

class Socket;

class MS_EXPORT PayloadChannel : public EnhancedEventEmitter
{
public:
	/**
	* @private
	*/
	PayloadChannel();

	/**
	* @private
	*/
	void close();

	/**
	* @private
	*/
	void notify(
		std::string event,
		json internal,
		json data = json(),
		std::string payload = std::string()
	);

private:
	void _processData(const json& msg);
	virtual void subClose() = 0;

private:
	// Closed flag.
	bool _closed = false;

	// Buffer for reading messages from the worker.
//	Buffer _recvBuffer;

	// Ongoing notification (waiting for its payload).
//	json _ongoingNotification : { targetId; event; data ? : any };
};

}
