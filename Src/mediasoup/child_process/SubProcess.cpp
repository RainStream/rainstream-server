#define MSC_CLASS "SubProcess"

#include "common.hpp"
#include "SubProcess.hpp"
#include "Socket.hpp"
#include "Logger.hpp"
#include "utils.hpp"

/* Static. */

// netstring length for a 65536 bytes payload.
static constexpr size_t MaxSize{ 65543 };
static constexpr size_t MessageMaxSize{ 65536 };
static uint8_t WriteBuffer[MaxSize];

inline static void onReqClose(uv_process_t *handle, int64_t exit_status, int term_signal)
{
	static_cast<SubProcess*>(handle->data)->OnUvReqClosed(exit_status, term_signal);
}

SubProcess* SubProcess::spawn(std::string workerPath, AStringVector parameters, json options)
{
	SubProcess *subProcess = new SubProcess();

	parameters.insert(parameters.begin(), workerPath);

	char** args = new char*[parameters.size() + 1];
	for (size_t i = 0; i < parameters.size(); ++i)
	{
		args[i] = strdup(parameters[i].c_str());
	}
	args[parameters.size()] = nullptr;

	std::vector<uv_stdio_container_t> child_stdios;

	if (options.contains("stdio") && options["stdio"].is_array())
	{
		// fd 0 (stdin)   : Just ignore it.
		// fd 1 (stdout)  : Pipe it for 3rd libraries that log their own stuff.
		// fd 2 (stderr)  : Same as stdout.
		// fd 3 (channel) : Producer Channel fd.
		// fd 4 (channel) : Consumer Channel fd.
		// fd 5 (channel) : Producer PayloadChannel fd.
		// fd 6 (channel) : Consumer PayloadChannel fd.

		int i = 0;
		for (auto& stdio : options["stdio"])
		{
			std::string stdio_type = stdio.get<std::string>();

			uv_stdio_container_t child_stdio;
			Socket* socket = nullptr;

			if (stdio_type == "ignore")
			{
				//child_stdio.data.fd = i;
				child_stdio.flags = UV_IGNORE;
			}
			else if (stdio_type == "pipe")
			{
				socket = new Socket;
				//child_stdio.data.fd = i;
				child_stdio.flags = static_cast<uv_stdio_flags>(
					UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
				child_stdio.data.stream = (uv_stream_t*)socket->GetUvHandle();
			}
			else if (stdio_type == "warp")
			{
				socket = new Socket;
				//child_stdio.data.fd = i;
				child_stdio.flags = UV_INHERIT_STREAM;
				child_stdio.data.stream = (uv_stream_t*)socket->GetUvHandle();
			}
			else
			{
				child_stdio.data.fd = i;
				child_stdio.flags = UV_INHERIT_FD;
			}

			subProcess->_stdio.push_back(socket);
			child_stdios.push_back(child_stdio);

			i++;
		}
	}

	std::vector<std::string> strEnvs;
	if (options.contains("env") && options["env"].is_object())
	{
		for (auto& el : options["env"].items()) {
			std::string env = Utils::Printf("%s=%s", el.key().c_str(), std::string(el.value()).c_str());
			strEnvs.push_back(env);
		}
	}

	subProcess->options.env = new char*[strEnvs.size() + 1];
	for (int i = 0; i < strEnvs.size(); ++i)
	{
		subProcess->options.env[i] = strdup(strEnvs[i].c_str());
	}
	subProcess->options.env[strEnvs.size()] = nullptr;

	subProcess->options.args = args;
	subProcess->options.file = strdup(workerPath.c_str());
	subProcess->options.stdio = child_stdios.data();
	subProcess->options.stdio_count = child_stdios.size();
	subProcess->options.exit_cb = onReqClose;
	//subProcess->options.flags = UV_PROCESS_DETACHED;

	subProcess->req.data = (void*)subProcess;
	// Create the rainstream-worker child process.
	int err = uv_spawn(uv_default_loop(), &subProcess->req, &subProcess->options);
	if (err != 0)
	{
		MSC_ERROR("uv_spawn() failed: %s", uv_strerror(err));

		delete subProcess;
		subProcess = nullptr;
	}

	if (args) {
		for (int i = 0; args[i]; i++) 
			free(args[i]);
		delete[] args;
	}

// 	if (subProcess->options.env) {
// 		for (int i = 0; subProcess->options.env[i]; i++) free(subProcess->options.env[i]);
// 		delete[] subProcess->options.env;
// 	}

	//delete[] subProcess->options.stdio;


	return subProcess;
}

SubProcess::SubProcess()
{

}

int SubProcess::pid()
{
	return this->req.pid;
}

void SubProcess::Close(std::string error)
{
	if (this->closed)
	{
		MSC_ERROR("already closed");
		return;
	}

	this->closed = true;

	if (req.pid)
	{
		uv_close((uv_handle_t*)&req, NULL);
		req.pid = 0;
	}

	// Close the Channel socket.

	for (auto stdio : this->_stdio)
	{
		if (stdio)
		{
			stdio->Destroy();
		}
	}

	this->emit("close", error);
}

std::vector<Socket*>& SubProcess::stdio()
{
	return this->_stdio;
}


void SubProcess::OnUvReqClosed(int64_t exit_status, int term_signal)
{
	Close();

	emit("exit", exit_status, term_signal);

	MSC_ERROR("child process exited code:%lld, signal:%d",
		exit_status, term_signal);
}
