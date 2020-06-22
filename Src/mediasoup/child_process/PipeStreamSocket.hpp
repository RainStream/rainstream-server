#pragma once

#include <uv.h>
#include <string>
#include "Logger.hpp"

class PipeStreamSocket
{
public:
	/* Struct for the data field of uv_req_t when writing data. */
	struct UvWriteData
	{
		PipeStreamSocket* socket{ nullptr };
		uv_write_t req;
		uint8_t store[1];
	};

public:
	PipeStreamSocket(size_t bufferSize);
	PipeStreamSocket& operator=(const PipeStreamSocket&) = delete;
	PipeStreamSocket(const PipeStreamSocket&) = delete;

protected:
	virtual ~PipeStreamSocket();

public:
	uv_pipe_t * GetUvHandle() const;
	void Start();
	void Destroy();
	bool IsClosing() const;
	void Write(const uint8_t* data, size_t len);
	void Write(const std::string& data);

	/* Callbacks fired by UV events. */
public:
	void OnUvReadAlloc(size_t suggestedSize, uv_buf_t* buf);
	void OnUvRead(ssize_t nread, const uv_buf_t* buf);
	void OnUvWriteError(int error);
	void OnUvShutdown(uv_shutdown_t* req, int status);
	void OnUvClosed();

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnPipeStreamRead() = 0;
	virtual void UserOnPipeStreamSocketClosed(bool isClosedByPeer) = 0;

private:
	// Allocated by this.
	uv_pipe_t * uvHandle{ nullptr };
	// Others.
	bool isClosing{ false };
	bool isClosedByPeer{ false };
	bool hasError{ false };

protected:
	// Passed by argument.
	size_t bufferSize{ 0 };
	// Allocated by this.
	uint8_t* buffer{ nullptr };
	// Others.
	size_t bufferDataLen{ 0 };

	Logger* logger;
};

/* Inline methods. */

inline bool PipeStreamSocket::IsClosing() const
{
	return this->isClosing;
}

inline void PipeStreamSocket::Write(const std::string& data)
{
	Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
}

