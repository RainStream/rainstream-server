
#include "pch.h"
#include "utils.hpp"
#include <random>
#include <sstream>
#include <uv.h>
#include <map>
#include <queue>

static std::queue<MessageData*> checkMessageDatas;
static std::map<uint64_t, uv_timer_t*> timeInterval;

static void checkCB(uv_check_t* handle)
{
	while (checkMessageDatas.size())
	{
		MessageData* message_data = checkMessageDatas.front();

		if (message_data)
		{
			message_data->Run();

			delete message_data;
		}

		checkMessageDatas.pop();
	}

}

void newCheckInvoke(MessageData* message_data)
{
	checkMessageDatas.push(message_data);
}

void InvokeCb(uv_timer_t* handle)
{
	MessageData* message_data = static_cast<MessageData*>(handle->data);

	if (message_data)
	{
		message_data->Run();

		if (message_data->once)
		{
			delete message_data;

			uv_timer_stop(handle);
			uv_close((uv_handle_t*)handle, nullptr);
		}
	}
}

uint64_t Invoke(MessageData* message_data,
	uint64_t timeout,
	uint64_t repeat)
{
	uv_timer_t* timer_req = new uv_timer_t;
	timer_req->data = message_data;

	uv_timer_init(uv_default_loop(), timer_req);
	uv_timer_start(timer_req, InvokeCb, timeout, repeat);

	if (!message_data->once)
	{
		timeInterval.insert(std::pair(timer_req->start_id, timer_req));
	}

	return timer_req->start_id;
}

void InvokeOnce(MessageData* message_data,
	uint64_t timeout,
	uint64_t repeat)
{
	message_data->once = true;
	Invoke(message_data, timeout, repeat);
}

void clearInterval(uint64_t identifier)
{
	if (timeInterval.contains(identifier))
	{
		uv_timer_t* timer_req = timeInterval[identifier];
		uv_timer_stop(timer_req);
		uv_close((uv_handle_t*)timer_req, nullptr);

		timeInterval.erase(identifier);
	}
}

void loadGlobalCheck()
{
	uv_check_t* check_req = new uv_check_t;

	uv_check_init(uv_default_loop(), check_req);
	uv_check_start(check_req, checkCB);
}

