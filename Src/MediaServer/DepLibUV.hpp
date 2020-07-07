#ifndef MS_DEP_LIBUV_HPP
#define MS_DEP_LIBUV_HPP

#include "common.hpp"
#include <uv.h>

namespace uWS {
	struct Hub;
}

class DepLibUV
{
public:
	static void ClassInit();
	static void ClassDestroy();
	static void PrintVersion();
	static void RunLoop();
	static uv_loop_t* GetLoop();
	static uWS::Hub* GetHub();
	static uint64_t GetTime();

private:
	static uv_loop_t* loop;
	static uWS::Hub* hub;
};

/* Inline static methods. */

inline uv_loop_t* DepLibUV::GetLoop()
{
	return DepLibUV::loop;
}

inline uWS::Hub* DepLibUV::GetHub()
{
	return DepLibUV::hub;
}

inline uint64_t DepLibUV::GetTime()
{
	return uv_now(DepLibUV::loop);
}

#endif
