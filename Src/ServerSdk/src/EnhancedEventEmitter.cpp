#include "RainStream.hpp"
#include "EnhancedEventEmitter.hpp"
#include "Logger.hpp"

namespace rs
{
	EnhancedEventEmitter::EnhancedEventEmitter()
	{
		//this->setMaxListeners(Infinity);

		this->_logger = new Logger("EnhancedEventEmitter");
	}

}
