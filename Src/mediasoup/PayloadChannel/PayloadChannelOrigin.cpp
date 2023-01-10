#define MSC_CLASS "PayloadChannelOrigin"

#include "common.h"
#include "PayloadChannelOrigin.h"
#include "child_process/Socket.h"

namespace mediasoup {

PayloadChannelOrigin::PayloadChannelOrigin(Socket* producerSocket, Socket* consumerSocket)
	: PayloadChannel()
{
	this->_producerSocket = producerSocket;
	this->_consumerSocket = consumerSocket;

	// Read PayloadChannel notifications from the worker.
	this->_consumerSocket->on("data", [=](const std::string& nsPayload)
		{

		});

	this->_consumerSocket->on("end", [=]() {
		MSC_DEBUG("Consumer PayloadChannel ended by the worker process"); });

	this->_consumerSocket->on("error", [=](json error) {
		MSC_ERROR("Consumer PayloadChannel error: %s", error.dump().c_str()); });

	this->_producerSocket->on("end", [=]() {
		MSC_DEBUG("Producer PayloadChannel ended by the worker process"); });

	this->_producerSocket->on("error", [=](json error) {
		MSC_ERROR("Producer PayloadChannel error: %s", error.dump().c_str()); });

	_consumerSocket->Start();
	//_producerSocket->Start();
}

void PayloadChannelOrigin::subClose()
{
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

}
