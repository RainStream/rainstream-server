#define MSC_CLASS "PipeStreamSocket"

/**
 * NOTE: This code cannot log to the Channel since this is the base code of the
 * Channel.
 */

#include "common.hpp"
#include "child_process/PipeStreamSocket.hpp"
#include <cstdlib> // std::malloc(), std::free()
#include <cstring> // std::memcpy()


 /* Static methods for UV callbacks. */

inline static void onAlloc(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf)
{
	static_cast<PipeStreamSocket*>(handle->data)->OnUvReadAlloc(suggestedSize, buf);
}

inline static void onRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	static_cast<PipeStreamSocket*>(handle->data)->OnUvRead(nread, buf);
}

inline static void onWrite(uv_write_t* req, int status)
{
	auto* writeData = static_cast<PipeStreamSocket::UvWriteData*>(req->data);
	auto* handle = req->handle;
	auto* socket = static_cast<PipeStreamSocket*>(handle->data);

	// Delete the UvWriteData struct (which includes the uv_req_t and the store char[]).
	std::free(writeData);

	// Just notify the PipeStreamSocket when error.
	if (status != 0)
		socket->OnUvWriteError(status);
}

inline static void onShutdown(uv_shutdown_t* req, int status)
{
	static_cast<PipeStreamSocket*>(req->data)->OnUvShutdown(req, status);
}

inline static void onClose(uv_handle_t* handle)
{
	static_cast<PipeStreamSocket*>(handle->data)->OnUvClosed();
}

inline static void onErrorClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

PipeStreamSocket::PipeStreamSocket(size_t bufferSize)
	: bufferSize(bufferSize)
{

	int err;

	this->uvHandle = new uv_pipe_t{ 0 };
	this->uvHandle->data = (void*)this;

	err = uv_pipe_init(uv_default_loop(), this->uvHandle, 0);
	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MSC_WARN("uv_pipe_init() failed: %s", uv_strerror(err));
	}
}

PipeStreamSocket::~PipeStreamSocket()
{
	delete this->uvHandle;
	delete[] this->buffer;
}

uv_pipe_t* PipeStreamSocket::GetUvHandle() const
{
	return this->uvHandle;
}

void PipeStreamSocket::Start()
{
	int err;
	// Start reading.
	err = uv_read_start(
		reinterpret_cast<uv_stream_t*>(this->uvHandle),
		static_cast<uv_alloc_cb>(onAlloc),
		static_cast<uv_read_cb>(onRead));
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onErrorClose));

		MSC_WARN("uv_read_start() failed: %s", uv_strerror(err));
	}
}

void PipeStreamSocket::Destroy()
{
	if (this->isClosing)
		return;

	int err;

	this->isClosing = true;

	// Don"t read more.
	err = uv_read_stop(reinterpret_cast<uv_stream_t*>(this->uvHandle));
	if (err != 0)
	{
		MSC_ERROR("uv_read_stop() failed: %s", uv_strerror(err));
		std::abort();
	}

	// If there is no error and the peer didn"t close its pipe side then close gracefully.
	if (!this->hasError && !this->isClosedByPeer)
	{
		// Use uv_shutdown() so pending data to be written will be sent to the peer before closing.
		auto req = new uv_shutdown_t;
		req->data = (void*)this;
		err = uv_shutdown(
			req, reinterpret_cast<uv_stream_t*>(this->uvHandle), static_cast<uv_shutdown_cb>(onShutdown));
		if (err != 0)
		{
			MSC_ERROR("uv_shutdown() failed: %s", uv_strerror(err));
			std::abort();
		}

	}
	// Otherwise directly close the socket.
	else
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
	}
}

void PipeStreamSocket::Write(const uint8_t* data, size_t len)
{
	if (this->isClosing)
		return;

	if (len == 0)
		return;

	// First try uv_try_write(). In case it can not directly send all the given data
	// then build a uv_req_t and use uv_write().

	uv_buf_t buffer = uv_buf_init(reinterpret_cast<char*>(const_cast<uint8_t*>(data)), len);
	int written = uv_try_write(reinterpret_cast<uv_stream_t*>(this->uvHandle), &buffer, 1);

	// All the data was written. Done.
	if (written == static_cast<int>(len))
	{
		return;
	}
	// Cannot write any data at first time. Use uv_write().
	else if (written == UV_EAGAIN || written == UV_ENOSYS)
	{
		// Set written to 0 so pendingLen can be properly calculated.
		written = 0;
	}
	// Any other error.
	else if (written < 0)
	{
		MSC_ERROR("uv_try_write() failed, trying uv_write(): %s", uv_strerror(written));

		// Set written to 0 so pendingLen can be properly calculated.
		written = 0;
	}

	size_t pendingLen = len - written;
	auto* writeData = new UvWriteData(pendingLen);

	writeData->req.data = static_cast<void*>(writeData);
	std::memcpy(writeData->store, data + written, pendingLen);

	buffer = uv_buf_init(reinterpret_cast<char*>(writeData->store), pendingLen);

	int err = uv_write(
		&writeData->req,
		reinterpret_cast<uv_stream_t*>(this->uvHandle),
		&buffer,
		1,
		static_cast<uv_write_cb>(onWrite));

	if (err != 0)
	{
		MSC_ERROR("uv_write() failed: %s", uv_strerror(err));

		// Delete the UvSendData struct.
		delete writeData;
	}
}

inline void PipeStreamSocket::OnUvReadAlloc(size_t /*suggestedSize*/, uv_buf_t* buf)
{
	// If this is the first call to onUvReadAlloc() then allocate the receiving buffer now.
	if (this->buffer == nullptr)
		this->buffer = new uint8_t[this->bufferSize];

	// Tell UV to write after the last data byte in the buffer.
	buf->base = reinterpret_cast<char*>(this->buffer + this->bufferDataLen);
	// Give UV all the remaining space in the buffer.
	if (this->bufferSize > this->bufferDataLen)
	{
		buf->len = this->bufferSize - this->bufferDataLen;
	}
	else
	{
		buf->len = 0;

		MSC_ERROR("no available space in the buffer");
	}
}

inline void PipeStreamSocket::OnUvRead(ssize_t nread, const uv_buf_t* /*buf*/)
{
	if (nread == 0)
		return;

	// Data received.
	if (nread > 0)
	{
		// Update the buffer data length.
		this->bufferDataLen += static_cast<size_t>(nread);

		// Notify the subclass.
		UserOnPipeStreamRead();
	}
	// Peer disconneted.
	else if (nread == UV_EOF || nread == UV_ECONNRESET)
	{
		this->isClosedByPeer = true;

		// Close local side of the pipe.
		Destroy();
	}
	// Some error.
	else
	{
		MSC_ERROR("read error, closing the pipe: %s", uv_strerror(nread));

		this->hasError = true;

		// Close the socket.
		Destroy();
	}
}

inline void PipeStreamSocket::OnUvWriteError(int error)
{
	if (this->isClosing)
		return;

	if (error != UV_EPIPE && error != UV_ENOTCONN)
		this->hasError = true;

	MSC_ERROR("write error, closing the pipe: %s", uv_strerror(error));

	Destroy();
}

inline void PipeStreamSocket::OnUvShutdown(uv_shutdown_t* req, int status)
{

	delete req;

	if (status != 0)
	{
		MSC_ERROR("shutdown error: %s", uv_strerror(status));
	}

	// Now do close the handle.
	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
}

inline void PipeStreamSocket::OnUvClosed()
{
	// Notify the subclass.
	UserOnPipeStreamSocketClosed(this->isClosedByPeer);

	// And delete this.
	delete this;
}

