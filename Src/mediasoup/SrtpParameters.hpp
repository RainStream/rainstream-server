#pragma once 

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
	/**
	 * Encryption and authentication transforms to be used.
	 */
	SrtpCryptoSuite cryptoSuite;

	/**
	 * SRTP keying material (master key and salt) in Base64.
	 */
	std::string keyBase64;
};


