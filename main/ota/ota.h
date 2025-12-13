#ifndef SPV_OTA_OTA_H_
#define SPV_OTA_OTA_H_

#include "esp_err.h"
#include "esp_ota_ops.h"

/// @brief Downloads an OTA update from `url` and applies it.
///
/// @param[in] url HTTP URL.
///
/// @return
///
///	- `ESP_OK`: OTA update successful. Reboot to apply.
///
/// - `ESP_FAIL`: Error downloading OTA.
esp_err_t spv_ota_download(const char *url);


/// @brief Starts an OTA update on the next available partition.
///
/// @param[in] ota_size Size in bytes of the OTA update.
/// @param[out] handle Handle for subsequent OTA function calls.
///
/// @note Use `esp_ota_write` to perform the OTA update.
///
/// @return
///
///	- `ESP_OK`: OTA started successfully.
///
/// - `ESP_FAIL`: OTA error.
esp_err_t spv_ota_begin(size_t ota_size, esp_ota_handle_t *handle);


/// @brief Aborts the current OTA update.
///
/// @param[in] handle OTA update handle.
///
/// @return `ESP_OK` or error.
esp_err_t spv_ota_abort(esp_ota_handle_t handle);


/// @brief Finishes the OTA update and switches to the new OTA partition.
///
/// @note OTA update will be applied only after rebooting.
///
/// @param[in] handle OTA update handle.
///
/// @return
///
///	- `ESP_OK`: OTA updated successfully.
///
///	- `ESP_ERR_OTA_VALIDATE_FAILED`: OTA update is invalid.
///
/// - `ESP_FAIL`: Other error.
esp_err_t spv_ota_end(esp_ota_handle_t handle);


/// @brief Returns the running firmware version.
uint64_t spv_ota_get_current_version();

#endif
