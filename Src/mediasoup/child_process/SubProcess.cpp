#include "common.hpp"
#include "SubProcess.hpp"
#include "Socket.hpp"
#include "Logger.hpp"
#include "utils.hpp"

namespace rs
{
	/* Static. */

	// netstring length for a 65536 bytes payload.
	static constexpr size_t MaxSize{ 65543 };
	static constexpr size_t MessageMaxSize{ 65536 };
	static uint8_t WriteBuffer[MaxSize];

	inline static void onReqClose(uv_process_t *handle, int64_t exit_status, int term_signal)
	{
		static_cast<SubProcess*>(handle->data)->OnUvReqClosed(exit_status, term_signal);
	}

	SubProcess* SubProcess::spawn(std::string workerPath, AStringVector parameters)
	{
		SubProcess *subProcess = new SubProcess();

		int argc = parameters.size() + 2;
		char** spawnArgs = new char*[argc];
		spawnArgs[0] = (char*)workerPath.c_str();
		spawnArgs[argc - 1] = NULL;
		for (size_t i = 0; i < parameters.size(); ++i)
		{
			spawnArgs[i + 1] = (char*)parameters[i].c_str();
		}

		uv_stdio_container_t child_stdio[4];
		child_stdio[0].flags = UV_IGNORE;
		child_stdio[0].data.fd = 0;//stdin
		child_stdio[1].flags = UV_INHERIT_FD;
		child_stdio[1].data.fd = 1;//stdout
		child_stdio[2].flags = UV_INHERIT_FD;
		child_stdio[2].data.fd = 2;//stderr
		child_stdio[3].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
		child_stdio[3].data.stream = (uv_stream_t*)subProcess->socket->GetUvHandle();

		char* envs[2];
		envs[0] = "MEDIASOUP_VERSION=__MEDIASOUP_VERSION__";
		envs[1] = NULL;

		subProcess->options.args = spawnArgs;
		subProcess->options.env = envs;
		subProcess->options.file = spawnArgs[0];
		subProcess->options.stdio = child_stdio;
		subProcess->options.stdio_count = ARRAYCOUNT(child_stdio);
		subProcess->options.exit_cb = onReqClose;

		subProcess->req.data = (void*)subProcess;
		// Create the rainstream-worker child process.
		int err = uv_spawn(uv_default_loop(), &subProcess->req, &subProcess->options);
		if (err != 0)
		{
			LOG(ERROR) << "uv_spawn() failed: " << uv_strerror(err);

			delete subProcess;
			subProcess = nullptr;

			return nullptr;
		}

		return subProcess;
	}

	SubProcess::SubProcess()
	{
		socket = new Socket;
	}

	void SubProcess::Close(std::string error)
	{
		if (this->closed)
		{
			LOG(ERROR) << "already closed";

			return;
		}

		this->closed = true;

		if (req.pid)
		{
			uv_close((uv_handle_t*)&req, NULL);
			req.pid = 0;
		}

		// Close the Channel socket.
		if (this->socket != nullptr)
			this->socket->Destroy();

		this->doEvent("close", error);
	}


	void SubProcess::OnUvReqClosed(int64_t exit_status, int term_signal)
	{
		Close();


		LOG(ERROR) << "child process exited "
			<<" code:" << exit_status
			<< " signal:" <<  term_signal;
	}
}
