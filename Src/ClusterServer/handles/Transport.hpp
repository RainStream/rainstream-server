#ifndef MS_TRANSPORT_HPP
#define MS_TRANSPORT_HPP

#include "common.hpp"
#include <string>

class Transport
{
public:
	Transport();
	Transport& operator=(const Transport&) = delete;
	Transport(const Transport&)            = delete;

protected:
	virtual ~Transport();

public:
	void Start();
	void Destroy();
	bool IsClosing() const;
	void Close(int code, std::string reason);


	/* Pure virtual methods that must be implemented by the subclass. */
protected:
// 	virtual void UserOnPipeStreamRead()                            = 0;
// 	virtual void UserOnPipeStreamSocketClosed(bool isClosedByPeer) = 0;

private:

	// Others.
	bool isClosing{ false };
	bool isClosedByPeer{ false };
	bool hasError{ false };

protected:

};

/* Inline methods. */

inline bool Transport::IsClosing() const
{
	return this->isClosing;
}

#endif
