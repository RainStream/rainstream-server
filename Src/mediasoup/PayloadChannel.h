#pragma once 

#include "EnhancedEventEmitter.h"

namespace mediasoup {

	class Socket;

	class PayloadChannel : public EnhancedEventEmitter
	{
	public:
		/**
		 * @private
		 */
		PayloadChannel(Socket* producerSocket, Socket* consumerSocket);

		/**
		 * @private
		 */
		void close();

		/**
		 * @private
		 */
		void notify(
			std::string event,
			json internal,
			json data = json(),
			std::string payload = std::string()
		);

	private:
		void _processData(const json& msg);

	private:
		// Closed flag.
		bool _closed = false;

		// Unix Socket instance for sending messages to the worker process.
		Socket* _producerSocket;

		// Unix Socket instance for receiving messages to the worker process.
		Socket* _consumerSocket;

		// Buffer for reading messages from the worker.
	//	Buffer _recvBuffer;

		// Ongoing notification (waiting for its payload).
	//	json _ongoingNotification : { targetId; event; data ? : any };
	};

}
