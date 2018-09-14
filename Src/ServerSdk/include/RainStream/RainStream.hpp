#pragma once

#include "nlohmann/json.hpp"
using Json = nlohmann::json;

#include <promise.hpp>
using namespace promise;

#include <uv.h>

#define undefined nullptr

/** Evaluates to the number of elements in an array (compile-time!) */
#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

const int DEFAULT_STATS_INTERVAL = 1000;

