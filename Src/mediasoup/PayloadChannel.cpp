#define MSC_CLASS "PayloadChannel"

#include "common.hpp"
#include "PayloadChannel.hpp"
extern "C" {
#include <netstring.h>
}

#include "Logger.hpp"
#include "errors.hpp"
#include "child_process/Socket.hpp"

class Socket;

// netstring length for a 4194304 bytes payload.
const int NS_MESSAGE_MAX_LEN = 4194313;
const int NS_PAYLOAD_MAX_LEN = 4194304;


PayloadChannel::PayloadChannel(Socket* producerSocket,
	Socket* consumerSocket)
	: EnhancedEventEmitter()
{
	MSC_DEBUG("constructor()");

	this->_producerSocket = producerSocket;
	this->_consumerSocket = consumerSocket;

	// Read PayloadChannel notifications from the worker.
	this->_consumerSocket->on("data", [=](const std::string& nsPayload)
	{
// 		if (!this->_recvBuffer)
// 		{
// 			this->_recvBuffer = buffer;
// 		}
// 		else
// 		{
// 			this->_recvBuffer = Buffer.concat(
// 				[this->_recvBuffer, buffer],
// 				this->_recvBuffer.length + buffer.length);
// 		}
// 
// 		if (this->_recvBuffer!.length > NS_PAYLOAD_MAX_LEN)
// 		{
// 			MSC_ERROR("receiving buffer is full, discarding all data into it");
// 
// 			// Reset the buffer and exit.
// 			this->_recvBuffer = undefined;
// 
// 			return;
// 		}
// 
// 		while (true) // eslint-disable-line no-constant-condition
// 		{
// 			let nsPayload;
// 
// 			try
// 			{
// 				nsPayload = netstring.nsPayload(this->_recvBuffer);
// 			}
// 			catch (error)
// 			{
// 				MSC_ERROR(
// 					"invalid netstring data received from the worker process: %s",
// 					String(error));
// 
// 				// Reset the buffer and exit.
// 				this->_recvBuffer = undefined;
// 
// 				return;
// 			}
// 
// 			// Incomplete netstring message.
// 			if (nsPayload == = -1)
// 				return;
// 
// 			this->_processData(nsPayload);
// 
// 			// Remove the read payload from the buffer.
// 			this->_recvBuffer =
// 				this->_recvBuffer!.slice(netstring.nsLength(this->_recvBuffer));
// 
// 			if (!this->_recvBuffer.length)
// 			{
// 				this->_recvBuffer = undefined;
// 
// 				return;
// 			}
// 		}
	});

	this->_consumerSocket->on("end", [=]() {
		MSC_DEBUG("Consumer PayloadChannel ended by the worker process"); });

	this->_consumerSocket->on("error", [=](json error) {
		MSC_ERROR("Consumer PayloadChannel error: %s", error.dump().c_str()); });

	this->_producerSocket->on("end", [=]() {
		MSC_DEBUG("Producer PayloadChannel ended by the worker process");});

	this->_producerSocket->on("error", [=](json error) {
		MSC_ERROR("Producer PayloadChannel error: %s", error.dump().c_str()); });

	_consumerSocket->Start();
	//_producerSocket->Start();
}

/**
 * @private
 */
void PayloadChannel::close()
{
	if (this->_closed)
		return;

	MSC_DEBUG("close()");

	this->_closed = true;

	// Remove event listeners but leave a fake "error" hander to avoid
	// propagation.
	this->_consumerSocket->removeAllListeners("end");
	this->_consumerSocket->removeAllListeners("error");
	this->_consumerSocket->on("error", [=]() {});

	this->_producerSocket->removeAllListeners("end");
	this->_producerSocket->removeAllListeners("error");
	this->_producerSocket->on("error", [=]() {});

	// Destroy the socket after a while to allow pending incoming messages.
// 	setTimeout(() = >
// 	{
// 		try { this->_producerSocket->destroy(); }
// 		catch (error) {}
// 		try { this->_consumerSocket->destroy(); }
// 		catch (error) {}
// 	}, 200);
}

/**
 * @private
 */
void PayloadChannel::notify(
	std::string event,
	json internal,
	json data/* = json()*/,
	std::string payload/* = std::string()*/
)
{
	MSC_DEBUG("notify() [event:%s]", event.c_str());

	if (this->_closed)
		throw new InvalidStateError("PayloadChannel closed");

// 	const notification = { event, internal, data };
// 	const ns1 = netstring.nsWrite(JSON.stringify(notification));
// 	const ns2 = netstring.nsWrite(payload);
// 
// 	if (Buffer.byteLength(ns1) > NS_MESSAGE_MAX_LEN)
// 		throw new Error("PayloadChannel notification too big");
// 	else if (Buffer.byteLength(ns2) > NS_MESSAGE_MAX_LEN)
// 		throw new Error("PayloadChannel payload too big");
// 
// 	try
// 	{
// 		// This may throw if closed or remote side ended.
// 		this->_producerSocket->write(ns1);
// 	}
// 	catch (error)
// 	{
// 		MSC_WARN("notify() | sending notification failed: %s", String(error));
// 
// 		return;
// 	}
// 
// 	try
// 	{
// 		// This may throw if closed or remote side ended.
// 		this->_producerSocket->write(ns2);
// 	}
// 	catch (error)
// 	{
// 		MSC_WARN("notify() | sending payload failed: %s", String(error));
// 
// 		return;
// 	}
}

void PayloadChannel::_processData(const json& msg)
{
// 	if (!this->_ongoingNotification)
// 	{
// 		let msg;
// 
// 		try
// 		{
// 			msg = JSON.parse(data.toString("utf8"));
// 		}
// 		catch (error)
// 		{
// 			MSC_ERROR(
// 				"received invalid data from the worker process: %s",
// 				String(error));
// 
// 			return;
// 		}
// 
// 		if (!msg.targetId || !msg.event)
// 		{
// 			MSC_ERROR("received message is not a notification");
// 
// 			return;
// 		}
// 
// 		this->_ongoingNotification =
// 		{
// 			targetId: msg.targetId,
// 			event : msg.event,
// 			data : msg.data
// 		};
// 	}
// 	else
// 	{
// 		const payload = data as Buffer;
// 
// 		// Emit the corresponding event.
// 		this->emit(
// 			this->_ongoingNotification.targetId,
// 			this->_ongoingNotification.event,
// 			this->_ongoingNotification.data,
// 			payload);
// 
// 		// Unset ongoing notification.
// 		this->_ongoingNotification = undefined;
// 	}
}
