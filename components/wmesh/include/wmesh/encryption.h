#ifndef WMESH_ENCRYPTION_H_
#define WMESH_ENCRYPTION_H_

#include <stdint.h>
#include "esp_err.h"
#include "sdkconfig.h"


#if CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 8
	#error Nonce lengths greater than 8 are not supported.
#elif CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 4
	typedef uint64_t wmesh_encryption_ctr_t;
#elif CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 2
	typedef uint32_t wmesh_encryption_ctr_t;
#elif CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 1
	typedef uint16_t wmesh_encryption_ctr_t;
#elif CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH == 1
	typedef uint8_t wmesh_encryption_ctr_t;
#else
	#error Nonce length too small.
#endif

#if CONFIG_WMESH_ENCRYPTION_AES128_GCM || \
	CONFIG_WMESH_ENCRYPTION_AES192_GCM || \
	CONFIG_WMESH_ENCRYPTION_AES256_GCM

	#include "mbedtls/gcm.h"
	typedef mbedtls_gcm_context wmesh_encryption_mbedtls_ctx_t;
#endif

#if CONFIG_WMESH_ENCRYPTION_CHACHA20_POLY1305
	#include "mbedtls/chachapoly.h"
	typedef mbedtls_chachapoly_context wmesh_encryption_mbedtls_ctx_t;
#endif

#if CONFIG_WMESH_ENCRYPTION_CHACHA20
	#include "mbedtls/chacha20.h"
	/// @brief No context required.
	typedef uint8_t wmesh_encryption_mbedtls_ctx_t[CONFIG_WMESH_ENCRYPTION_KEYLENGTH];
#endif

#if CONFIG_WMESH_ENCRYPTION_DISABLED
	typedef uint8_t wmesh_encryption_mbedtls_ctx_t;
#endif


#define WMESH_CIPHERTEXT_BASE_LENGTH ( \
	CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH + \
	CONFIG_WMESH_ENCRYPTION_IV_LENGTH + \
	CONFIG_WMESH_ENCRYPTION_TAG_LENGTH \
	)
#define WMESH_CIPHERTEXT_LENGTH(x) (x + WMESH_CIPHERTEXT_BASE_LENGTH)
#define WMESH_PLAINTEXT_LENGTH(x) (x - WMESH_CIPHERTEXT_BASE_LENGTH)



/// @brief Stores encryption-related data.
typedef struct {

	/// @brief MbedTLS context.
	wmesh_encryption_mbedtls_ctx_t mbedtls_ctx;

} wmesh_encryption_ctx_t;


/// @brief Initial configuration for encryption.
typedef struct {

	/// @brief Shared key to use. Must be of length
	/// `CONFIG_WMESH_ENCRYPTION_KEYLENGTH`.
	uint8_t *key;

} wmesh_encryption_config_t;


/// @brief Creates a new encryption context.
///
/// @param[out] ctx Output context.
/// @param[in] config Initial values for context.
///
/// @return `ESP_OK` or error.
esp_err_t wmesh_encryption_ctx_new(
	wmesh_encryption_ctx_t *ctx,
	const wmesh_encryption_config_t *config
);


/// @brief Uninitializes an encryption context.
///
/// @param[in] ctx Context to free.
void wmesh_encryption_ctx_free(wmesh_encryption_ctx_t *ctx);


/// @brief Encrypts the given data.
///
/// @note Use `WMESH_CIPHERTEXT_LENGTH` to get the required output buffer size.
///
/// @param[in] ctx Encryption context.
/// @param[inout] ctr Encryption counter. Incremented when encryption succeeds.
/// @param[in] plaintext Data to encrypt.
/// @param[in] plaintext_length Size in bytes of input data.
/// @param[out] ciphertext Encrypted data. Length of the buffer must be, at
/// least, of `WMESH_CIPHERTEXT_LENGTH(plaintext_length)` bytes.
///
/// @return `ESP_OK` if successful.
esp_err_t wmesh_encrypt(
	wmesh_encryption_ctx_t *ctx,
	wmesh_encryption_ctr_t *ctr,

	const uint8_t *plaintext,
	size_t plaintext_length,

	uint8_t *ciphertext
);

/// @brief Encrypts the given data and updates internal state.
///
/// @note Use `WMESH_CIPHERTEXT_LENGTH` to get the required output buffer size.
///
/// @param[in] ctx Encryption context.
/// @param[inout] ctr Encryption counter. Incremented when encryption succeeds.
/// @param[in] plaintext Data to encrypt.
/// @param[in] plaintext_length Size in bytes of input data.
/// @param[in] aad Additional Authenticated Data. Data which needs to be
/// 		authenticated with the tag, but not encrypted.
/// @param[in] aad_size Size in bytes of `aad`.
/// @param[out] ciphertext Encrypted data. Length of the buffer must be, at
/// least, of `WMESH_CIPHERTEXT_LENGTH(plaintext_length)` bytes.
///
/// @return `ESP_OK` if successful.
esp_err_t wmesh_encrypt_aad(
	wmesh_encryption_ctx_t *ctx,
	wmesh_encryption_ctr_t *ctr,

	const uint8_t *plaintext,
	size_t plaintext_length,

	const uint8_t *aad,
	size_t aad_size,

	uint8_t *ciphertext
);


/// @brief Decryption function status.
typedef enum {
	/// @brief Decryption successful.
	WMESH_DECRYPT_OK,
	/// @brief Decryption was successful, but the nonce was reused.
	WMESH_DECRYPT_STALE,
	/// @brief Decryption was successful, but authentication was not.
	WMESH_DECRYPT_AUTH_ERROR,
	/// @brief Decryption was unsuccessful.
	WMESH_DECRYPT_ERROR,
} wmesh_decrypt_status_t;


/// @brief Decrypts the given data and updates internal state.
///
/// @param[in] ctx Encryption context.
/// @param[inout] ctr Encryption counter. Incremented when encryption succeeds.
/// @param[in] ciphertext Data to decrypt.
/// @param[in] ciphertext_length Size in bytes of input data.
/// @param[out] plaintext Decrypted data buffer. Length must be, at least,
/// `WMESH_PLAINTEXT_LENGTH(ciphertext_length)` bytes long.
///
/// @attention Decrypted data is only valid when `WMESH_DECRYPT_OK` is returned.
/// Otherwise, its contents are left uninitialized.
///
/// @return
/// 	- `WMESH_DECRYPT_OK` if successful.
///
/// 	- `WMESH_DECRYPT_STALE` in case of nonce reuse.
///
/// 	- `WMESH_DECRYPT_AUTH_ERROR` if the decryption succeeded, but the
///			authentication tag didn't match the ciphertext.
///
/// 	- `WMESH_DECRYPT_ERROR` if the ciphertext failed to decrypt.
wmesh_decrypt_status_t wmesh_decrypt(
	wmesh_encryption_ctx_t *ctx,
	wmesh_encryption_ctr_t *ctr,

	const uint8_t *ciphertext,
	size_t ciphertext_length,

	uint8_t *plaintext
);

/// @brief Decrypts the given data and updates internal state.
///
/// @param[in] ctx Encryption context.
/// @param[inout] ctr Encryption counter. Incremented when encryption succeeds.
/// @param[in] ciphertext Data to decrypt.
/// @param[in] ciphertext_length Size in bytes of input data.
/// @param[in] aad Additional Authenticated Data.
/// @param[in] aad_size Size in bytes of `aad`.
/// @param[out] plaintext Decrypted data buffer. Length must be, at least,
/// `WMESH_PLAINTEXT_LENGTH(ciphertext_length)` bytes long.
///
/// @attention Decrypted data is only valid when `WMESH_DECRYPT_OK` is returned.
/// Otherwise, its contents are left uninitialized.
///
/// @return
/// 	- `WMESH_DECRYPT_OK` if successful.
///
/// 	- `WMESH_DECRYPT_STALE` in case of nonce reuse.
///
/// 	- `WMESH_DECRYPT_AUTH_ERROR` if the decryption succeeded, but the
///			authentication tag didn't match the ciphertext.
///
/// 	- `WMESH_DECRYPT_ERROR` if the ciphertext failed to decrypt.
wmesh_decrypt_status_t wmesh_decrypt_aad(
	wmesh_encryption_ctx_t *ctx,
	wmesh_encryption_ctr_t *ctr,

	const uint8_t *ciphertext,
	size_t ciphertext_length,

	const uint8_t *aad,
	size_t aad_size,

	uint8_t *plaintext
);


#endif
