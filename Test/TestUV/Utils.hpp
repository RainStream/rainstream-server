#pragma once

#include <random>
#include <string>
#include <vector>

using AStringVector = std::vector<std::string>;

/** Evaluates to the uint32_t of elements in an array (compile-time!) */
#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);                \
    TypeName& operator=(const TypeName&)


class MessageData {
public:
	MessageData() {}
	virtual ~MessageData() {}

	virtual void Run() = 0;

	bool once = false;
};

template <class FunctorT>
class MessageWithFunctor : public MessageData {
public:
	explicit MessageWithFunctor(FunctorT&& functor)
		: functor_(std::forward<FunctorT>(functor)) {}

	void Run() override { functor_(); }

private:
	~MessageWithFunctor() {}

	typename std::remove_reference<FunctorT>::type functor_;

	DISALLOW_COPY_AND_ASSIGN(MessageWithFunctor);
};

void newCheckInvoke(MessageData* message_data);

uint64_t Invoke(MessageData* message_data,
	uint64_t timeout = 0, 
	uint64_t repeat = 0);

void InvokeOnce(MessageData* message_data,
	uint64_t timeout = 0,
	uint64_t repeat = 0);

template <class FunctorT>
void setImmediate(FunctorT&& functor) {
	newCheckInvoke(new MessageWithFunctor<FunctorT>(
		std::forward<FunctorT>(functor)));
}

template <class FunctorT>
void setTimeout(FunctorT&& functor, int timeout) {
	InvokeOnce(new MessageWithFunctor<FunctorT>(
		std::forward<FunctorT>(functor)), timeout);
}

template <class FunctorT>
uint64_t setInterval(FunctorT&& functor, int interval){
	return Invoke(new MessageWithFunctor<FunctorT>(
		std::forward<FunctorT>(functor)), interval, interval);
}

void clearInterval(uint64_t identifier);

void loadGlobalCheck();

