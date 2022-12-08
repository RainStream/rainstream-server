#pragma once 

namespace mediasoup {

/**
 * SRTP crypto suite.
 */
using SrtpCryptoSuite = std::string;
// 	| "AES_CM_128_HMAC_SHA1_80"
// 	| "AES_CM_128_HMAC_SHA1_32";

/**
 * SRTP parameters.
 */
struct SrtpParameters
{
	SrtpParameters()
	{

	}

	SrtpParameters(const json& data)
		: SrtpParameters()
	{
		if (data.is_object())
		{
			this->cryptoSuite = data.value("cryptoSuite", "");
			this->keyBase64 = data.value("keyBase64", "");
		}
	}

	operator json() const
	{
		json data = {
			{ "cryptoSuite", cryptoSuite },
			{ "keyBase64", keyBase64 }
		};

		return data;
	}

	/**
	 * Encryption and authentication transforms to be used.
	 */
	SrtpCryptoSuite cryptoSuite;

	/**
	 * SRTP keying material (master key and salt) in Base64.
	 */
	std::string keyBase64;
};

}
