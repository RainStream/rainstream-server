#pragma once

#define GLOG_NO_ABBREVIATED_SEVERITIES 1

#include "glog/logging.h"

namespace rs
{
#define APP_NAME = "rainstream";

#define RS_ABORT(desc, ...) \
	do \
	{ \
		std::fprintf(stderr, "ABORT" desc , ##__VA_ARGS__); \
		std::fflush(stderr); \
		std::abort(); \
	} \
	while (false)
}
