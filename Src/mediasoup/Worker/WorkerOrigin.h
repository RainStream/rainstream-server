#pragma once

#include "Worker.h"

namespace mediasoup {

class WorkerOrigin : public Worker
{
public:
	explicit WorkerOrigin(json settings);
	virtual ~WorkerOrigin();

protected:
	virtual void init(AStringVector spawnArgs) override;
	virtual void subClose() override;

protected:
	// mediasoup-worker child process.
	SubProcess* _child{ nullptr };

};

}
