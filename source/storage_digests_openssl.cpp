#include <game/storage.hpp>

#include <string>
#include <vector>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

static class digests_openssl : public game::storage
{
public:
	digests_openssl()
	{
		ERR_load_crypto_strings();
		OpenSSL_add_all_algorithms();
		mdctx = EVP_MD_CTX_create();
	}
	~digests_openssl()
	{
		EVP_MD_CTX_destroy(mdctx);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ERR_free_strings();
	}
	process_result digest(std::initializer_list<std::vector<uint8_t> const *> data, decltype(EVP_sha3_512()) algorithm, std::string algorithm_name, game::identifiers & what)
	{
		static thread_local std::string result;
		static thread_local std::vector<uint8_t> bytes;
		bytes.resize(EVP_MAX_MD_SIZE);

		EVP_DigestInit_ex(mdctx, algorithm, NULL);

		for (auto & chunk : data) {
			EVP_DigestUpdate(mdctx, chunk->data(), chunk->size());
		}

		unsigned int size;
		EVP_DigestFinal_ex(mdctx, bytes.data(), &size);
		bytes.resize(size);

		result.resize(size * 2);

		static char hex[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

		for (int i = 0; i < size; ++ i) {
			int j = i << 1;
			result[j] = hex[bytes[i] >> 4];
			result[j+1] = hex[bytes[i] & 0xf];
		}

		if (what.count(algorithm_name)) {
			if (what[algorithm_name] != result) { return process_result::INCONSISTENT; }
		} else {
			what[algorithm_name] = result;
		}
		return process_result::VERIFIED;
	}

	virtual process_result process(std::vector<uint8_t> & data, game::identifiers & what, bool keep_stored) override
	{
		std::vector<std::pair<std::string, decltype(EVP_blake2b512())>> digests = {
#ifndef OPENSSL_NO_BLAKE2
			{"blake2b512", EVP_blake2b512()},
#endif
			{"sha3_512", EVP_sha3_512()},
			{"sha512_256", EVP_sha512_256()}
		};

		for (auto & pair : digests) {
			auto result = digest({&data}, pair.second, pair.first, what);
			if (result == process_result::INCONSISTENT) { return result; }
		}
		return process_result::VERIFIED;
	}

private:
	EVP_MD_CTX * mdctx;
} storage_digests_openssl;
