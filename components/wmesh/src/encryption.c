#include "wmesh/encryption.h"

#include <string.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"


#ifdef CONFIG_WMESH_ENCRYPTION_AES128_GCM
	#define ENC_TYPE_GCM
	static const mbedtls_cipher_id_t CIPHER_ID = MBEDTLS_CIPHER_ID_AES;
#elifdef CONFIG_WMESH_ENCRYPTION_AES192_GCM
	#define ENC_TYPE_GCM
	static const mbedtls_cipher_id_t CIPHER_ID = MBEDTLS_CIPHER_ID_AES;
#elifdef CONFIG_WMESH_ENCRYPTION_AES256_GCM
	#define ENC_TYPE_GCM
	static const mbedtls_cipher_id_t CIPHER_ID = MBEDTLS_CIPHER_ID_AES;
#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20_POLY1305
#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20
#elifdef CONFIG_WMESH_ENCRYPTION_DISABLED
#else
	#error Encryption algorithm undefined
#endif


static void init_mbedtls_context(wmesh_encryption_mbedtls_ctx_t *ctx);
static void free_mbedtls_context(wmesh_encryption_mbedtls_ctx_t *ctx);
static esp_err_t setkey_mbedtls_context(
	wmesh_encryption_mbedtls_ctx_t *ctx,
	const uint8_t key[CONFIG_WMESH_ENCRYPTION_KEYLENGTH]
);
static void generate_iv(uint8_t iv[CONFIG_WMESH_ENCRYPTION_IV_LENGTH]);


esp_err_t wmesh_encryption_ctx_new(
	wmesh_encryption_ctx_t *ctx,
	const wmesh_encryption_config_t *config
) {
	esp_err_t err;

	init_mbedtls_context(&ctx->mbedtls_ctx);

	if((err = setkey_mbedtls_context(&ctx->mbedtls_ctx, config->key)) != ESP_OK) {
		free_mbedtls_context(&ctx->mbedtls_ctx);
		return err;
	}

	return ESP_OK;
}


void wmesh_encryption_ctx_free(wmesh_encryption_ctx_t *ctx) {
	free_mbedtls_context(&ctx->mbedtls_ctx);
}


esp_err_t wmesh_encrypt(
	wmesh_encryption_ctx_t *ctx,
	wmesh_encryption_ctr_t *ctr,

	const uint8_t *plaintext,
	size_t plaintext_length,

	uint8_t *ciphertext
) {
	return wmesh_encrypt_aad(ctx, ctr, plaintext, plaintext_length, NULL, 0, ciphertext);
}


struct encrypted_message {
	uint8_t iv[CONFIG_WMESH_ENCRYPTION_IV_LENGTH + CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH];
	uint8_t tag[CONFIG_WMESH_ENCRYPTION_TAG_LENGTH];
	uint8_t ciphertext[];
} __attribute__((packed));

esp_err_t wmesh_encrypt_aad(
	wmesh_encryption_ctx_t *ctx,
	wmesh_encryption_ctr_t *ctr,

	const uint8_t *plaintext,
	size_t plaintext_length,

	const uint8_t *aad,
	size_t aad_size,

	uint8_t *ciphertext
) {
	// Output structure is:
	// |  IV  |  Nonce  |  Tag  |   Ciphertext   |
	struct encrypted_message *encrypted = (struct encrypted_message *) ciphertext;

	memset(encrypted->iv, 0, CONFIG_WMESH_ENCRYPTION_IV_LENGTH + CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH);
	generate_iv(encrypted->iv);
	wmesh_encryption_ctr_t send_counter = *ctr + 1;
	memcpy(encrypted->iv + CONFIG_WMESH_ENCRYPTION_IV_LENGTH, &send_counter, CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH);

	int err;
	#ifdef ENC_TYPE_GCM
		err = mbedtls_gcm_crypt_and_tag(
			&ctx->mbedtls_ctx,
			MBEDTLS_GCM_ENCRYPT,
			plaintext_length,
			encrypted->iv, CONFIG_WMESH_ENCRYPTION_IV_LENGTH + CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH,
			aad, aad_size,
			plaintext,
			encrypted->ciphertext,
			CONFIG_WMESH_ENCRYPTION_TAG_LENGTH, encrypted->tag
		);
	#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20_POLY1305
		err = mbedtls_chachapoly_encrypt_and_tag(
			&ctx->mbedtls_ctx,
			plaintext_length,
			encrypted->iv,
			aad, aad_size,
			plaintext,
			encrypted->ciphertext,
			encrypted->tag
		);
	#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20
		err = mbedtls_chacha20_crypt(
			ctx->mbedtls_ctx,
			encrypted->iv,
			0,
			plaintext_length, plaintext,
			encrypted->ciphertext
		);
	#elifdef CONFIG_WMESH_ENCRYPTION_DISABLED
		err = 0;
		memcpy(encrypted->ciphertext, plaintext, plaintext_length);
	#endif

	if(err != 0) {
		return ESP_FAIL;
	}

	*ctr += 1;
	return ESP_OK;
}


wmesh_decrypt_status_t wmesh_decrypt(
	wmesh_encryption_ctx_t *ctx,
	wmesh_encryption_ctr_t *ctr,

	const uint8_t *ciphertext,
	size_t ciphertext_length,

	uint8_t *plaintext
) {
	return wmesh_decrypt_aad(ctx, ctr, ciphertext, ciphertext_length, NULL, 0, plaintext);
}


wmesh_decrypt_status_t wmesh_decrypt_aad(
	wmesh_encryption_ctx_t *ctx,
	wmesh_encryption_ctr_t *ctr,

	const uint8_t *ciphertext,
	size_t ciphertext_length,

	const uint8_t *aad,
	size_t aad_size,

	uint8_t *plaintext
) {

	// Input structure is:
	// |  IV  |  Nonce  |  Tag  |   Ciphertext   |
	struct encrypted_message *encrypted = (struct encrypted_message *) ciphertext;

	if(ciphertext_length < WMESH_CIPHERTEXT_BASE_LENGTH) {
		return WMESH_DECRYPT_ERROR;
	}
	size_t data_length = WMESH_PLAINTEXT_LENGTH(ciphertext_length);

	wmesh_encryption_ctr_t received_ctr = 0;
	memcpy(&received_ctr, encrypted->iv + CONFIG_WMESH_ENCRYPTION_IV_LENGTH, CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH);
	if(*ctr >= received_ctr) {
		return WMESH_DECRYPT_STALE;
	}

	int err;
	#ifdef ENC_TYPE_GCM
		err = mbedtls_gcm_auth_decrypt(
			&ctx->mbedtls_ctx,
			data_length,
			encrypted->iv, CONFIG_WMESH_ENCRYPTION_IV_LENGTH + CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH,
			aad, aad_size,
			encrypted->tag, CONFIG_WMESH_ENCRYPTION_TAG_LENGTH,
			encrypted->ciphertext, plaintext
		);

		switch(err) {
			case 0:
				*ctr = received_ctr;
				return WMESH_DECRYPT_OK;

			case MBEDTLS_ERR_GCM_AUTH_FAILED:
				return WMESH_DECRYPT_AUTH_ERROR;

			default:
				return WMESH_DECRYPT_ERROR;
		}

	#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20_POLY1305
		err = mbedtls_chachapoly_auth_decrypt(
			&ctx->mbedtls_ctx,
			data_length,
			encrypted->iv,
			aad, aad_size,
			encrypted->tag,
			encrypted->ciphertext,
			plaintext
		);

		switch(err) {
			case 0:
				*ctr = received_ctr;
				return WMESH_DECRYPT_OK;

			case MBEDTLS_ERR_CHACHAPOLY_AUTH_FAILED:
				return WMESH_DECRYPT_AUTH_ERROR;

			default:
				return WMESH_DECRYPT_ERROR;
		}

	#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20
		err = mbedtls_chacha20_crypt(
			ctx->mbedtls_ctx,
			encrypted->iv,
			0,
			data_length, encrypted->ciphertext,
			plaintext
		);

		switch(err) {
			case 0:
				*ctr = received_ctr;
				return WMESH_DECRYPT_OK;

			default:
				return WMESH_DECRYPT_ERROR;
		}

	#elifdef CONFIG_WMESH_ENCRYPTION_DISABLED
		memcpy(plaintext, encrypted->ciphertext, data_length);
		err = WMESH_DECRYPT_OK;
		return err;
	#endif

}


static void generate_iv(uint8_t iv[CONFIG_WMESH_ENCRYPTION_IV_LENGTH]) {
	#ifdef CONFIG_WMESH_ENCRYPTION_IV_SOURCE_MAC
		esp_read_mac(iv, ESP_MAC_WIFI_SOFTAP);
	#elifdef CONFIG_WMESH_ENCRYPTION_IV_SOURCE_RANDOM
		esp_fill_random(iv, CONFIG_WMESH_ENCRYPTION_IV_LENGTH);
	#endif
}


static void init_mbedtls_context(wmesh_encryption_mbedtls_ctx_t *ctx) {
	#ifdef ENC_TYPE_GCM
		mbedtls_gcm_init(ctx);
	#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20_POLY1305
		mbedtls_chachapoly_init(ctx);
	#endif
}


static void free_mbedtls_context(wmesh_encryption_mbedtls_ctx_t *ctx) {
	#ifdef ENC_TYPE_GCM
		mbedtls_gcm_free(ctx);
	#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20_POLY1305
		mbedtls_chachapoly_free(ctx);
	#endif
}


static esp_err_t setkey_mbedtls_context(
	wmesh_encryption_mbedtls_ctx_t *ctx,
	const uint8_t key[CONFIG_WMESH_ENCRYPTION_KEYLENGTH]
) {
	int error;

	#ifdef ENC_TYPE_GCM
		error = mbedtls_gcm_setkey(
			ctx,
			CIPHER_ID,
			key, CONFIG_WMESH_ENCRYPTION_KEYLENGTH * 8
		);
	#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20_POLY1305
		error = mbedtls_chachapoly_setkey(
			ctx,
			key
		);
	#elifdef CONFIG_WMESH_ENCRYPTION_CHACHA20
		memcpy(*ctx, key, CONFIG_WMESH_ENCRYPTION_KEYLENGTH);
		error = 0;
	#elifdef CONFIG_WMESH_ENCRYPTION_DISABLED
		error = 0;
	#endif

	return error == 0 ? ESP_OK : ESP_FAIL;
}
