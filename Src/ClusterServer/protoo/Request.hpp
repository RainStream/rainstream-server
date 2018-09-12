#ifndef PROTOO_REQUEST_HPP
#define PROTOO_REQUEST_HPP

#include "common.hpp"
#include <string>
#include <unordered_map>

namespace protoo
{
	using AcceptFunc = std::function<void(Json json)>;
	using RejectFunc = std::function<void(int, std::string)>;

	class Request
	{
		friend class Peer;
	public:
		enum class MethodId
		{
			MEDIASOUP_UNKNOWN = 0,
			MEDIASOUP_REQUEST = 1,
			MEDIASOUP_NOTIFICATION,
			CHANGE_DISPLAY_NAME,
		};

	private:
		static std::unordered_map<std::string, MethodId> string2MethodId;

	public:
		Request(Peer* peer, Json& json);
		virtual ~Request();

		void Accept();
		void Accept(Json& data);
		void Reject(int code, std::string& reason);
		void Reject(int code, const char* reason = nullptr);

	public:
		// Passed by argument.
		Peer* peer{ nullptr };
		uint32_t id{ 0 };
		std::string method;
		MethodId methodId{ MethodId::MEDIASOUP_UNKNOWN };
		Json data;
		// Others.
		bool replied{ false };

	public:
		AcceptFunc accept{ nullptr };
		RejectFunc reject{ nullptr };
	};
} // namespace protoo

#endif
