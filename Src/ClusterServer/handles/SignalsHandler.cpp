#define MSC_CLASS "SignalsHandler"
// #define MSC_LOG_DEV

#include "handles/SignalsHandler.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "errors.hpp"

/* Static methods for UV callbacks. */

inline static void onSignal(uv_signal_t* handle, int signum)
{
	static_cast<SignalsHandler*>(handle->data)->OnUvSignal(signum);
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

SignalsHandler::SignalsHandler(Listener* listener) : listener(listener)
{
	MSC_TRACE();
}

SignalsHandler::~SignalsHandler()
{
	MSC_TRACE();

	if (!this->closed)
		Close();
}

void SignalsHandler::Close()
{
	MSC_TRACE();

	if (this->closed)
		return;

	this->closed = true;

	for (auto* uvHandle : this->uvHandles)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(uvHandle), static_cast<uv_close_cb>(onClose));
	}
}

void SignalsHandler::AddSignal(int signum, const std::string& name)
{
	MSC_TRACE();

	if (this->closed)
		MSC_THROW_ERROR("closed");

	int err;
	auto uvHandle = new uv_signal_t;

	uvHandle->data = static_cast<void*>(this);

	err = uv_signal_init(DepLibUV::GetLoop(), uvHandle);

	if (err != 0)
	{
		delete uvHandle;

		MSC_THROW_ERROR("uv_signal_init() failed for signal %s: %s", name.c_str(), uv_strerror(err));
	}

	err = uv_signal_start(uvHandle, static_cast<uv_signal_cb>(onSignal), signum);

	if (err != 0)
		MSC_THROW_ERROR("uv_signal_start() failed for signal %s: %s", name.c_str(), uv_strerror(err));

	// Enter the UV handle into the vector.
	this->uvHandles.push_back(uvHandle);
}

inline void SignalsHandler::OnUvSignal(int signum)
{
	MSC_TRACE();

	// Notify the listener.
	this->listener->OnSignal(this, signum);
}
