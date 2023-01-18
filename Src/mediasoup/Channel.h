#pragma once

#include <unordered_map>
#include "EnhancedEventEmitter.h"

namespace mediasoup {

class MS_EXPORT Channel : public EnhancedEventEmitter
{
public:
	Channel(int pid);

	void close();

	virtual async_simple::coro::Lazy<json> request(std::string method, std::optional<std::string> handlerId = std::nullopt, const json& data = json()) = 0;

protected:
	void _receivePayload(const std::string& nsPayload);
	void _processMessage(const json& msg);
	virtual void subClose() = 0;

protected:
	int _pid;
	// Closed flag.
	bool _closed = false;
	
	// Next id for messages sent to the worker process.
	uint32_t _nextId = 0;
	// Map of pending sent requests.
	std::unordered_map<uint32_t, async_simple::Promise<json> > _sents;
};

}
