#pragma once 

/**
 * SRTP parameters.
 */
struct SrtpParameters
{
	/**
	 * Encryption and authentication transforms to be used.
	 */
	cryptoSuite: SrtpCryptoSuite;

	/**
	 * SRTP keying material (master key and salt) in Base64.
	 */
	std::string keyBase64;
};

/**
 * SRTP crypto suite.
 */
struct SrtpCryptoSuite =
	| "AES_CM_128_HMAC_SHA1_80"
	| "AES_CM_128_HMAC_SHA1_32";
