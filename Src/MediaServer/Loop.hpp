#ifndef MS_LOOP_HPP
#define MS_LOOP_HPP

#include "common.hpp"
#include "handles/SignalsHandler.hpp"
#include <unordered_map>


class Loop : public SignalsHandler::Listener
{
public:
	explicit Loop();
	~Loop() override;

private:
	void Close();

	/* Methods inherited from SignalsHandler::Listener. */
public:
	void OnSignal(SignalsHandler* signalsHandler, int signum) override;

private:
	// Allocated by this.
	SignalsHandler* signalsHandler{ nullptr };
	// Others.
	bool closed{ false };
};

#endif
