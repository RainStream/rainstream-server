#define MS_CLASS "Timer"
// #define MS_LOG_DEV_LEVEL 3
#include "pch.h"
#include "Timer.hpp"

/* Static methods for UV callbacks. */

inline static void onTimer(uv_timer_t* handle)
{
	static_cast<Timer*>(handle->data)->OnUvTimer();
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

Timer::Timer(Listener* listener) : listener(listener)
{
	this->uvHandle       = new uv_timer_t;
	this->uvHandle->data = static_cast<void*>(this);

	int err = uv_timer_init(uv_default_loop(), this->uvHandle);

	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;
	}
}

Timer::~Timer()
{
	if (!this->closed)
		Close();
}

void Timer::Close()
{
	if (this->closed)
		return;

	this->closed = true;

	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
}

void Timer::Start(uint64_t timeout, uint64_t repeat)
{
	if (this->closed)
		return;

	this->timeout = timeout;
	this->repeat  = repeat;

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
		Stop();

	int err = uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), timeout, repeat);

	if (err != 0)
		return;
}

void Timer::Stop()
{
	if (this->closed)
		return;

	int err = uv_timer_stop(this->uvHandle);

	if (err != 0)
		return;
}

void Timer::Reset()
{
	if (this->closed)
		return;

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) == 0)
		return;

	if (this->repeat == 0u)
		return;

	int err =
	  uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), this->repeat, this->repeat);

	if (err != 0)
		return;
}

void Timer::Restart()
{
	if (this->closed)
		return;

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
		Stop();

	int err =
	  uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), this->timeout, this->repeat);

	if (err != 0)
		return;
}

inline void Timer::OnUvTimer()
{
	// Notify the listener.
	this->listener->OnTimer(this);
}
