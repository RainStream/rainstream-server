#pragma once
// Object.defineProperty(exports, "__esModule", { value: true });
// const events_1 = require("events");
// const Logger_1 = require("./Logger");
// const logger = new Logger_1.Logger("EnhancedEventEmitter");

class EnhancedEventEmitter : public EventEmitter
{
	EnhancedEventEmitter()
		: EventEmitter()
	{
		this->setMaxListeners(Infinity);
	}

	void safeEmit(std::string event, ...args)
	{
		const numListeners = this->listenerCount(event);
		try {
			return this->emit(event, ...args);
		}
		catch (error) {
			logger.error("safeEmit() | event listener threw an error [event:%s]:%o", event, error);
			return Boolean(numListeners);
		}
	}

	async safeEmitAsPromise(std::string event, ...args)
	{
		return new Promise((resolve, reject) = > (this->safeEmit(event, ...args, resolve, reject)));
	}
};
