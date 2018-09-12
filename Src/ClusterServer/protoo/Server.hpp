#ifndef PROTOO_SERVER_HPP
#define PROTOO_SERVER_HPP

#include "common.hpp"

namespace protoo
{
	class WebSocketServer;

	class Server
	{
	public:
		Server();
		~Server();

	public:
	

	protected:

	private:

		// Closed flag.
		bool _closed = false;
	};
}

#endif

