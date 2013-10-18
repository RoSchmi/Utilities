#include "Cryptography.h"

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#include <wincrypt.h>
#elif defined POSIX
	#include <openssl/evp.h>
#endif

using namespace std;
using namespace util;
using namespace util::crypto;

random_device crypto::device;
mt19937_64 crypto::generator(crypto::device());

array<uint8, crypto::sha2_length> crypto::calculate_sha2(const uint8* source, word length) {
	array<uint8, crypto::sha2_length> result;

#ifdef WINDOWS
	HCRYPTPROV provider = 0;
	HCRYPTHASH hasher = 0;
	DWORD hash_length = crypto::sha2_length;

	CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
	CryptCreateHash(provider, CALG_SHA_512, 0, 0, &hasher);
	CryptHashData(hasher, source, static_cast<DWORD>(length), 0);

	CryptGetHashParam(hasher, HP_HASHVAL, result.data(), &hash_length, 0);

	CryptDestroyHash(hasher);
	CryptReleaseContext(provider, 0);
#elif defined POSIX
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();

	EVP_DigestInit_ex(ctx, EVP_sha512(), nullptr);
	EVP_DigestUpdate(ctx, const_cast<void*>(reinterpret_cast<const void*>(source)), length);
	EVP_DigestFinal_ex(ctx, reinterpret_cast<unsigned char*>(result.data()), nullptr);

	EVP_MD_CTX_destroy(ctx);
#endif

	return result;
}

array<uint8, crypto::sha1_length> crypto::calculate_sha1(const uint8* source, word length) {
	array<uint8, crypto::sha1_length> result;

#ifdef WINDOWS
	HCRYPTPROV provider = 0;
	HCRYPTHASH hasher = 0;
	DWORD hash_length = crypto::sha1_length;

	CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	CryptCreateHash(provider, CALG_SHA1, 0, 0, &hasher);
	CryptHashData(hasher, source, static_cast<DWORD>(length), 0);

	CryptGetHashParam(hasher, HP_HASHVAL, result.data(), &hash_length, 0);

	CryptDestroyHash(hasher);
	CryptReleaseContext(provider, 0);
#elif defined POSIX
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();

	EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
	EVP_DigestUpdate(ctx, const_cast<void*>(reinterpret_cast<const void*>(source)), length);
	EVP_DigestFinal_ex(ctx, reinterpret_cast<unsigned char*>(result.data()), nullptr);
#endif

	return result;
}

void crypto::random_bytes(uint8* buffer, word count) {
	uniform_int_distribution<uint64> distribution;

	if (count > 0) {
		for (; count > 7; count -= 8)
			*reinterpret_cast<uint64*>(buffer + count - 8) = distribution(generator);

		for (; count > 0; count--)
			*(buffer + count - 1) = static_cast<uint8>(distribution(generator));
	}
}

int64 crypto::random_int64(int64 floor, int64 ceiling) {
	return uniform_int_distribution<int64>(floor, ceiling)(generator);
}

uint64 crypto::random_uint64(uint64 floor, uint64 ceiling) {
	return uniform_int_distribution<uint64>(floor, ceiling)(generator);
}


float64 crypto::random_float64(float64 floor, float64 ceiling) {
	return uniform_real_distribution<float64>(floor, ceiling)(generator);
}
