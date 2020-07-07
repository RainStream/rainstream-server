#define MSC_CLASS "DepLibUV"
// #define MS_LOG_DEV

#include "DepLibUV.hpp"
#include <Logger.hpp>
#include <cstdlib> // std::abort()
#include <uWS.h>

/* Static variables. */

uv_loop_t* DepLibUV::loop{ nullptr };\
uWS::Hub* DepLibUV::hub{ nullptr };

/* Static methods. */

void DepLibUV::ClassInit()
{
	// NOTE: Logger depends on this so we cannot log anything here.

	int err;

	DepLibUV::loop = uv_default_loop();
	if (loop == 0)
		MSC_ABORT("libuv initialization failed");

	//here must use default loop
	DepLibUV::hub = new uWS::Hub(0, true);
	if (hub == 0)
		MSC_ABORT("uWS initialization failed");
}

void DepLibUV::ClassDestroy()
{
	MSC_TRACE();

	// This should never happen.
	if (DepLibUV::loop == nullptr)
		MSC_ABORT("DepLibUV::loop was not allocated");

	uv_loop_close(DepLibUV::loop);
	delete DepLibUV::loop;
	delete DepLibUV::hub;
}

void DepLibUV::PrintVersion()
{
	MSC_TRACE();

	MSC_DEBUG("loaded libuv version: \"%s\"", uv_version_string());
}

void DepLibUV::RunLoop()
{
	MSC_TRACE();

	// This should never happen.
	if (DepLibUV::loop == nullptr)
		MSC_ABORT("DepLibUV::loop was not allocated");

	uv_run(DepLibUV::loop, UV_RUN_DEFAULT);
}
