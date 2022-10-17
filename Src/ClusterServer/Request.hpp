#pragma once

#include <common.hpp>
#include <unordered_map>

namespace protoo
{
	class Peer;

	class Request
	{
	public:
		Request(Peer* peer, json& jsonRequest);
		virtual ~Request();

		void Accept();
		void Accept(const json& data);
		void Reject(int errorCode, std::string errorReason);

	public:
		// Passed by argument.
		Peer* peer{ nullptr };
		uint32_t id{ 0u };
		std::string method;
		json internal;
		json data;
		// Others.
		bool replied{ false };
	};

}
