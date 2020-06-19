#pragma once

#include <uv.h>
#include "utils.hpp"
#include "EventEmitter.hpp"


class Socket;

class SubProcess : public EventEmitter
{
public:
	static SubProcess* spawn(std::string workerPath, AStringVector parameters, json options = json::object());

protected:
	SubProcess();

	/* Callbacks fired by UV events. */
public:
	void OnUvReqClosed(int64_t exit_status, int term_signal);

public:
	int pid();
	void Close(std::string error = "");
	std::vector<Socket*>& stdio();

private:
	// Allocated by this.
	uv_process_t req{ 0 };
	uv_process_options_t options{ 0 };

	// Others.
	bool closed{ false };

	std::vector<Socket*> _stdio;
};
