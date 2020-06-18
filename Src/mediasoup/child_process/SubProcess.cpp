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

	SubProcess* SubProcess::spawn(std::string workerPath, AStringVector parameters, json options)
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
			for (auto& stdio: options["stdio"])
			{
				std::string stdio_type = stdio.get<std::string>();

				uv_stdio_container_t child_stdio;

				if (i == 0)
				{
					child_stdio.data.fd = i;//stdin
					child_stdio.flags = UV_IGNORE;
				}
				else if (i == 1)
				{
					child_stdio.data.fd = i;//stdout
					child_stdio.flags = UV_INHERIT_FD;
				}
				else if (i == 2)
				{
					child_stdio.data.fd = i;//stderr
					child_stdio.flags = UV_INHERIT_FD;
				}
				else
				{
					if (stdio_type == "pipe")
					{
						child_stdio.data.fd = i;//channel
						child_stdio.flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
						child_stdio.data.stream = (uv_stream_t*)subProcess->socket->GetUvHandle();
					}
					else
					{
						child_stdio.data.fd = i;
						child_stdio.flags = UV_IGNORE;
					}
				}
				
				child_stdios.push_back(child_stdio);
			}
		}

		std::vector<std::string> strEnvs;
		if (options.contains("env") && options["env"].is_object())
		{
			for (auto& el : options["env"].items()) {
				std::string env = utils::Printf("%s=%s", el.key().c_str(), std::string(el.value()).c_str());
				strEnvs.push_back(env);
			}
		}

		std::vector<char*> envs;
		for (auto &env : strEnvs)
		{
			envs.push_back(env.data());
		}
		envs.push_back('\0');

		subProcess->options.args = spawnArgs;
		subProcess->options.env = envs.data();
		subProcess->options.file = spawnArgs[0];
		subProcess->options.stdio = child_stdios.data();
		subProcess->options.stdio_count = child_stdios.size();
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
