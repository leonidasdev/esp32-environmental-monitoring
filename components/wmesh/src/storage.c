#include "wmesh/storage.h"

esp_err_t wmesh_storage_open(
	wmesh_storage_handle_t *handle,
	const wmesh_storage_config_t *config
) {
	#ifdef CONFIG_WMESH_PERSISTENCE_NVS
		return nvs_open(config->name, NVS_READWRITE, &handle->nvs_handle);
	#endif
}


void wmesh_storage_close(wmesh_storage_handle_t *handle) {
	#ifdef CONFIG_WMESH_PERSISTENCE_NVS
		nvs_commit(handle->nvs_handle);
		nvs_close(handle->nvs_handle);
	#endif
}

wmesh_storage_result_t wmesh_storage_read_ctr(
	wmesh_storage_handle_t *handle,
	const char *key,
	wmesh_encryption_ctr_t *ctr
) {
	#ifdef CONFIG_WMESH_PERSISTENCE_NVS
		esp_err_t err;
		#if CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 4
			err = nvs_get_u64(handle->nvs_handle, key, ctr);
		#elif CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 2
			err = nvs_get_u32(handle->nvs_handle, key, ctr);
		#elif CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 1
			err = nvs_get_u16(handle->nvs_handle, key, ctr);
		#else
			err = nvs_get_u8(handle->nvs_handle, key, ctr);
		#endif

		switch(err) {
			case ESP_OK:
				return WMESH_STORAGE_OK;

			case ESP_ERR_NVS_NOT_FOUND:
				return WMESH_STORAGE_NOT_FOUND;

			default:
				return WMESH_STORAGE_ERROR;
		}
	#endif
}


wmesh_storage_result_t wmesh_storage_write_ctr(
	wmesh_storage_handle_t *handle,
	const char *key,
	const wmesh_encryption_ctr_t *ctr
) {
	#ifdef CONFIG_WMESH_PERSISTENCE_NVS
		esp_err_t err;
		#if CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 4
			err = nvs_set_u64(handle->nvs_handle, key, *ctr);
		#elif CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 2
			err = nvs_set_u32(handle->nvs_handle, key, *ctr);
		#elif CONFIG_WMESH_ENCRYPTION_NONCE_LENGTH > 1
			err = nvs_set_u16(handle->nvs_handle, key, *ctr);
		#else
			err = nvs_set_u8(handle->nvs_handle, key, *ctr);
		#endif

		switch(err) {
			case ESP_OK:
				return WMESH_STORAGE_OK;

			default:
				return WMESH_STORAGE_ERROR;
		}
	#endif
}
