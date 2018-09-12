#define MS_CLASS "Loop"
// #define MS_LOG_DEV

#include "Loop.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RainStreamError.hpp"
#include "Settings.hpp"
#include <cerrno>
#include <iostream> // std::cout, std::cerr
#include <string>
#include <utility> // std::pair()

/* Instance methods. */

Loop::Loop()
{
	MS_TRACE();
 
	// Set the signals handler.
	this->signalsHandler = new SignalsHandler(this);

	// Add signals to handle.
	this->signalsHandler->AddSignal(SIGINT, "INT");
	this->signalsHandler->AddSignal(SIGTERM, "TERM");

	MS_DEBUG_DEV("starting libuv loop");
	DepLibUV::RunLoop();
	MS_DEBUG_DEV("libuv loop ended");
}

Loop::~Loop()
{
	MS_TRACE();
}

void Loop::Close()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_ERROR("already closed");

		return;
	}

	this->closed = true;

	// Close the SignalsHandler.
	if (this->signalsHandler != nullptr)
		this->signalsHandler->Destroy();

}

void Loop::OnSignal(SignalsHandler* /*signalsHandler*/, int signum)
{
	MS_TRACE();

	switch (signum)
	{
		case SIGINT:
			MS_DEBUG_DEV("signal INT received, exiting");
			Close();
			break;

		case SIGTERM:
			MS_DEBUG_DEV("signal TERM received, exiting");
			Close();
			break;

		default:
			MS_WARN_DEV("received a signal (with signum %d) for which there is no handling code", signum);
	}
}
