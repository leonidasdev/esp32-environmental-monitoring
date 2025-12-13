#ifndef WMESH_STORAGE_H_
#define WMESH_STORAGE_H_

#include "nvs_flash.h"
#include "sdkconfig.h"
#include "wmesh/encryption.h"


/// @brief Storage handle.
typedef struct {
	#ifdef CONFIG_WMESH_PERSISTENCE_NVS
		nvs_handle_t nvs_handle;
	#endif
} wmesh_storage_handle_t;


/// @brief Storage configuration. Used when opening a handle.
typedef struct {
	#ifdef CONFIG_WMESH_PERSISTENCE_NVS
		/// @brief NVS namespace.
		const char *name;
	#endif
} wmesh_storage_config_t;

/// @brief Opens a storage device.
///
/// @param[out] handle Handle. Only valid if `ESP_OK` is returned.
/// @param[in] config Storage configuration.
///
/// @return `ESP_OK` or error.
esp_err_t wmesh_storage_open(
	wmesh_storage_handle_t *handle,
	const wmesh_storage_config_t *config
);


/// @brief Closes a storage device.
///
/// @param[in] handle Handle to close.
void wmesh_storage_close(wmesh_storage_handle_t *handle);


/// @brief Error values used for storage functions.
typedef enum {

	/// @brief Success.
	WMESH_STORAGE_OK,

	/// @brief Key was not found.
	WMESH_STORAGE_NOT_FOUND,

	/// @brief Error during operation.
	WMESH_STORAGE_ERROR,

} wmesh_storage_result_t;


/// @brief Load an encryption counter.
///
/// @param[in] handle Storage handle.
/// @param[in] key Name for this value.
/// @param[out] ctr Counter value. Only valid when `WMESH_STORAGE_OK` is returned.
///
/// @return `WMESH_STORAGE_OK` or error.
wmesh_storage_result_t wmesh_storage_read_ctr(
	wmesh_storage_handle_t *handle,
	const char *key,
	wmesh_encryption_ctr_t *ctr
);

/// @brief Stores an encryption counter.
///
/// @param[in] handle Storage handle.
/// @param[in] key Name for this value.
/// @param[in] ctr Counter value.
///
/// @return `WMESH_STORAGE_OK` or error.
wmesh_storage_result_t wmesh_storage_write_ctr(
	wmesh_storage_handle_t *handle,
	const char *key,
	const wmesh_encryption_ctr_t *ctr
);
#endif
