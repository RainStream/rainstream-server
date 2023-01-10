#pragma once

#include "Worker.h"

namespace mediasoup {

class ChannelNative;
class PayloadChannelNative;

class WorkerNative : public Worker
{
public:
	explicit WorkerNative(json settings);
	virtual ~WorkerNative();

protected:
	virtual void init(AStringVector spawnArgs) override;
	virtual void subClose() override;

protected:
	//
	std::thread _work_thread;
};

}
