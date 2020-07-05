#define MSC_CLASS "Loop"
// #define MS_LOG_DEV

#include "Loop.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <cerrno>
#include <iostream> // std::cout, std::cerr
#include <string>
#include <utility> // std::pair()

/* Instance methods. */

Loop::Loop()
{
	MSC_TRACE();
 
	// Set the signals handler.
	this->signalsHandler = new SignalsHandler(this);

	// Add signals to handle.
	this->signalsHandler->AddSignal(SIGINT, "INT");
	this->signalsHandler->AddSignal(SIGTERM, "TERM");

	MSC_DEBUG("starting libuv loop");
	DepLibUV::RunLoop();
	MSC_DEBUG("libuv loop ended");
}

Loop::~Loop()
{
	MSC_TRACE();
}

void Loop::Close()
{
	MSC_TRACE();

	if (this->closed)
	{
		MSC_ERROR("already closed");

		return;
	}

	this->closed = true;

	// Close the SignalsHandler.
	if (this->signalsHandler != nullptr)
		this->signalsHandler->Destroy();

}

void Loop::OnSignal(SignalsHandler* /*signalsHandler*/, int signum)
{
	MSC_TRACE();

	switch (signum)
	{
		case SIGINT:
			MSC_DEBUG("signal INT received, exiting");
			Close();
			break;

		case SIGTERM:
			MSC_DEBUG("signal TERM received, exiting");
			Close();
			break;

		default:
			MSC_WARN("received a signal (with signum %d) for which there is no handling code", signum);
	}
}
