/**
 * NOTE: This code cannot log to the Channel since this is the base code of the
 * Channel.
 */

#define MS_CLASS "Transport"

#include "handles/Transport.hpp"
#include "Logger.hpp"
#include "RainStreamError.hpp"
#include <cstdlib> // std::malloc(), std::free()
#include <cstring> // std::memcpy()

/* Instance methods. */

Transport::Transport() 
{
	MS_TRACE_STD();

	int err;

}

Transport::~Transport()
{
	MS_TRACE_STD();

}

void Transport::Start()
{
	
}

void Transport::Destroy()
{
	MS_TRACE_STD();

	if (this->isClosing)
		return;

	int err;

	this->isClosing = true;	
}

void Transport::Close(int code, std::string reason)
{

}
