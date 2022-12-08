#define MSC_CLASS "Socket"

#include "common.h"
#include "Socket.h"
#include "Logger.h"
#include "utils.h"
#include <cmath>   // std::ceil()
#include <cstdio>  // sprintf()
#include <cstring> // std::memmove()
#include <sstream> // std::ostringstream

namespace mediasoup {

/* Static. */

// netstring length for a 65536 bytes payload.
static constexpr size_t MaxSize{ 65543 };
static constexpr size_t MessageMaxSize{ 65536 };


const int MESSAGE_MAX_LEN = 4194308;
const int PAYLOAD_MAX_LEN = 4194304;


/* Instance methods. */

Socket::Socket()
	: PipeStreamSocket(MaxSize)
{

}

Socket::~Socket()
{
}

void Socket::SetListener(Listener* listener)
{
	this->listener = listener;
}

void Socket::UserOnPipeStreamRead()
{
	if (this->bufferDataLen > MaxSize)
	{
		MSC_ERROR("receiving buffer is full, discarding all data in it");

		// Reset the buffer and exit.
		this->msgStart = 0;
		this->bufferDataLen = 0;

		return;
	}

	// Be ready to parse more than a single message in a single TCP chunk.
	while (true)
	{
		if (IsClosing())
			return;

		size_t readLen = this->bufferDataLen - this->msgStart;

		if (readLen < 4)
		{
			// Incomplete data.
			break;
		}

		uint32_t dataLen = Utils::Byte::Get4Bytes(this->buffer, this->msgStart);

		if (readLen < 4 + dataLen)
		{
			// Incomplete data.
			break;
		}

		char* dataStart = reinterpret_cast<char*>(this->buffer + this->msgStart + 4);

		std::string payload = std::string((const char*)dataStart, dataLen);

		this->msgStart += 4 + dataLen;
		

		// If here it means that dataStart points to the beginning of a JSON string
		// with dataLen bytes length, so recalculate readLen.
		readLen = reinterpret_cast<const uint8_t*>(dataStart) - (this->buffer + this->msgStart) +
			dataLen + 1;

		// Notify the listener.
		this->processMessage(payload);
	}

	if (this->msgStart != 0)
	{
		size_t readLen = this->bufferDataLen - this->msgStart;
		std::memmove(this->buffer, this->buffer + this->msgStart, readLen);
		this->msgStart = 0;
		this->bufferDataLen = readLen;
	}
}

void Socket::UserOnPipeStreamSocketClosed(bool isClosedByPeer)
{
	this->closed = true;

	if (isClosedByPeer)
	{
		this->emit("close", isClosedByPeer);
	}
}

void Socket::processMessage(const std::string& msg)
{
	this->emit("data", msg);
}

}
