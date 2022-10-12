/* This is a simple HTTP(S) web server much like Python's SimpleHTTPServer */

#include <uwebsockets/App.h>

/* Helpers for this example */
#include "helpers/AsyncFileReader.h"
#include "helpers/AsyncFileStreamer.h"
#include "helpers/Middleware.h"

/* optparse */
#define OPTPARSE_IMPLEMENTATION
#include "helpers/optparse.h"

void PrintCachedFiles(std::map<std::string_view, AsyncFileReader*> dict) {
	for (auto it = dict.begin(); it != dict.end(); it++) {
		std::cout << it->first << std::endl;
	}
}

int main(int argc, char** argv) {
	int port = 3000;
	struct uWS::SocketContextOptions ssl_options = {
		.key_file_name = "certs/privkey.pem",
		.cert_file_name = "certs/fullchain.pem",
	};

	const char* root = "../public";

	AsyncFileStreamer asyncFileStreamer(root);

	/* HTTPS */
	uWS::SSLApp(ssl_options).get("/*", [&asyncFileStreamer](auto* res, auto* req) {
		std::cout << "get request: " << req->getUrl() << "  " << req->getQuery() << std::endl;
		serveFile(res, req);
		asyncFileStreamer.streamFile(res, req->getUrl());
	}).listen(port, [port, root](auto* token) {
		if (token) {
			std::cout << "Serving " << root << " over HTTPS a " << port << std::endl;
		}
	}).run();

	std::cout << "Failed to listen to port " << port << std::endl;
}
