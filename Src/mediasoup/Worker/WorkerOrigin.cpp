#define MSC_CLASS "WorkerOrigin"

#include "common.h"
#include "errors.h"
#include "WorkerOrigin.h"
#include "Channel/ChannelOrigin.h"
#include "PayloadChannel/PayloadChannelOrigin.h"
#include "child_process/SubProcess.h"

#define WORK_PATH "mediasoup-worker.exe"

namespace mediasoup {

WorkerOrigin::WorkerOrigin(json settings)
	: Worker(settings)
{
	this->_appData = settings.value("appData", json());
}

WorkerOrigin:: ~WorkerOrigin()
{

}


void WorkerOrigin::init(AStringVector spawnArgs)
{
	json spawnOptions = {
		{ "env", { {"MEDIASOUP_VERSION", __MEDIASOUP_VERSION__} } },
		{ "detached", false },
		{ "stdio", { "ignore", "pipe", "pipe", "pipe", "pipe", "pipe", "pipe"} },
		{ "windowsHide", false }
	};

	this->_child = SubProcess::spawn(WORK_PATH, spawnArgs, spawnOptions);

	this->_pid = this->_child->pid();

	this->_channel = new ChannelOrigin(
		this->_child->stdio()[3],
		this->_child->stdio()[4],
		this->_pid);

	this->_payloadChannel = new PayloadChannelOrigin(
		this->_child->stdio()[5],
		this->_child->stdio()[6]);

	bool spawnDone = false;

	// Listen for "running" notification.
	this->_channel->once(std::to_string(this->_pid), [&](std::string event, const json& data)
		{
			if (!spawnDone && event == "running")
			{
				spawnDone = true;

				MSC_DEBUG("worker process running [pid:%d]", this->_pid);

				this->emit("@success");
			}
		});

	this->_child->on("exit", [&](int code, int signal)
		{
			this->_child = nullptr;
	this->close();

	if (!spawnDone)
	{
		spawnDone = true;

		if (code == 42)
		{
			MSC_ERROR(
				"worker process failed due to wrong settings [pid:%d]", this->_pid);

			this->emit("@failure", new TypeError("wrong settings"));
		}
		else
		{
			MSC_ERROR(
				"worker process failed unexpectedly [pid:%d, code:%d, signal:%d]",
				this->_pid, code, signal);

			this->emit(
				"@failure",
				new Error("[pid:${ this->_pid }, code : ${ code }, signal : ${ signal }]"));
		}
	}
	else
	{
		MSC_ERROR(
			"worker process died unexpectedly [pid:%d, code:%d, signal:%d]",
			this->_pid, code, signal);

		this->safeEmit(
			"died",
			new Error("[pid:${ this->_pid }, code : ${ code }, signal : ${ signal }]"));
	}
		});

	this->_child->on("error", [=, &spawnDone](Error error)
		{
			this->_child = nullptr;
	this->close();

	if (!spawnDone)
	{
		spawnDone = true;

		MSC_ERROR(
			"worker process failed [pid:%d]: %s", this->_pid, error.ToString().c_str());

		this->emit("@failure", error);
	}
	else
	{
		MSC_ERROR(
			"worker process error [pid:%d]: %s", this->_pid, error.ToString().c_str());

		this->safeEmit("died", error);
	}
		});

	// Be ready for 3rd party worker libraries logging to stdout.
// 	this->_child->stdout!.on("data", (buffer) = >
// 	{
// 		for (const line of buffer.toString("utf8").split("\n"))
// 		{
// 			if (line)
// 				workerLogger.debug(`(stdout) $ { line }`);
// 		}
// 	});
// 
// 	// In case of a worker bug, mediasoup will log to stderr.
// 	this->_child->stderr!.on("data", (buffer) = >
// 	{
// 		for (const line of buffer.toString("utf8").split("\n"))
// 		{
// 			if (line)
// 				workerLogger.error(`(stderr) $ { line }`);
// 		}
// 	});
}

void WorkerOrigin::subClose()
{
	// Kill the worker process.
	if (this->_child)
	{
		// Remove event listeners but leave a fake "error" hander to avoid
		// propagation.
		this->_child->removeAllListeners("exit");
		this->_child->removeAllListeners("error");
		this->_child->on("error", []() {});
		//this->_child->kill("SIGTERM");
		this->_child = nullptr;
	}
}

}
