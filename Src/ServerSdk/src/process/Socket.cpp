#include "RainStream.hpp"
#include "Socket.hpp"
#include "Logger.hpp"
#include <cmath>   // std::ceil()
#include <cstdio>  // sprintf()
#include <cstring> // std::memmove()
#include <sstream> // std::ostringstream
extern "C" {
#include <netstring.h>
}

namespace rs
{
	/* Static. */

	// netstring length for a 65536 bytes payload.
	static constexpr size_t MaxSize{ 65543 };
	static constexpr size_t MessageMaxSize{ 65536 };
	static uint8_t WriteBuffer[MaxSize];

	/* Instance methods. */

	Socket::Socket()
		: PipeStreamSocket(MaxSize)
		, logger(new Logger("Socket"))
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
		// Be ready to parse more than a single message in a single TCP chunk.
		while (true)
		{
			if (IsClosing())
				return;

			size_t readLen = this->bufferDataLen - this->msgStart;
			char* dataStart = nullptr;
			size_t dataLen;
			int nsRet = netstring_read(
				reinterpret_cast<char*>(this->buffer + this->msgStart), readLen, &dataStart, &dataLen);

			if (nsRet != 0)
			{
				switch (nsRet)
				{
				case NETSTRING_ERROR_TOO_SHORT:
					// Check if the buffer is full.
					if (this->bufferDataLen == this->bufferSize)
					{
						// First case: the incomplete message does not begin at position 0 of
						// the buffer, so move the incomplete message to the position 0.
						if (this->msgStart != 0)
						{
							std::memmove(this->buffer, this->buffer + this->msgStart, readLen);
							this->msgStart = 0;
							this->bufferDataLen = readLen;
						}
						// Second case: the incomplete message begins at position 0 of the buffer.
						// The message is too big, so discard it.
						else
						{
							logger->error(
								"no more space in the buffer for the unfinished message being parsed, "
								"discarding it");

							this->msgStart = 0;
							this->bufferDataLen = 0;
						}
					}
					// Otherwise the buffer is not full, just wait.

					// Exit the parsing loop.
					return;

				case NETSTRING_ERROR_TOO_LONG:
					logger->error("NETSTRING_ERROR_TOO_LONG");
					break;

				case NETSTRING_ERROR_NO_COLON:
					logger->error("NETSTRING_ERROR_NO_COLON");
					break;

				case NETSTRING_ERROR_NO_COMMA:
					logger->error("NETSTRING_ERROR_NO_COMMA");
					break;

				case NETSTRING_ERROR_LEADING_ZERO:
					logger->error("NETSTRING_ERROR_LEADING_ZERO");
					break;

				case NETSTRING_ERROR_NO_LENGTH:
					logger->error("NETSTRING_ERROR_NO_LENGTH");
					break;
				}

				// Error, so reset and exit the parsing loop.
				this->msgStart = 0;
				this->bufferDataLen = 0;

				return;
			}

			// If here it means that dataStart points to the beginning of a JSON string
			// with dataLen bytes length, so recalculate readLen.
			readLen = reinterpret_cast<const uint8_t*>(dataStart) - (this->buffer + this->msgStart) +
				dataLen + 1;

			std::string nsPayload = std::string((const char*)dataStart, dataLen);

			// Notify the listener.
			this->processMessage(nsPayload);


			// If there is no more space available in the buffer and that is because
			// the latest parsed message filled it, then empty the full buffer.
			if ((this->msgStart + readLen) == this->bufferSize)
			{
				this->msgStart = 0;
				this->bufferDataLen = 0;
			}
			// If there is still space in the buffer, set the beginning of the next
			// parsing to the next position after the parsed message.
			else
			{
				this->msgStart += readLen;
			}

			// If there is more data in the buffer after the parsed message
			// then parse again. Otherwise break here and wait for more data.
			if (this->bufferDataLen > this->msgStart)
			{
				continue;
			}

			break;
		}
	}

	void Socket::UserOnPipeStreamSocketClosed(bool isClosedByPeer)
	{
		this->closed = true;

		if (isClosedByPeer)
		{
			emit("close", isClosedByPeer);
		}
	}

	void Socket::processMessage(const std::string& msg)
	{
		emit("data", msg);
	}
}
