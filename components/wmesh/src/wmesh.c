#include "wmesh/wmesh.h"

#include "esp_log.h"
#include "esp_now.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "mbedtls/gcm.h"

static const char *TAG = "Mesh";
wmesh_handle_t *wmesh_global_handle = NULL;


static esp_err_t ensure_ap_mode();
static esp_err_t initialize_esp_now(const wmesh_config_t *config);

wmesh_handle_t *wmesh_init(const wmesh_config_t *config) {
	esp_err_t err;

	if(wmesh_global_handle != NULL) {
		ESP_LOGE(TAG, "Multiple mesh instances are not supported.");
		ESP_LOGE(TAG, "Call `wmesh_stop` to stop the current mesh.");
		return NULL;
	}

	if((err = ensure_ap_mode()) != ESP_OK) {
		return NULL;
	}

	// Wi-Fi is now initialized and set to either AP or AP+STA.
	if((err = initialize_esp_now(config)) != ESP_OK) {
		return NULL;
	}

	wmesh_handle_t *handle = malloc(sizeof(*handle));
	if(!handle) {
		ESP_LOGE(TAG, "Error allocating handle");
		return NULL;
	}

	handle->peers = calloc(1, sizeof(*handle->peers));
	if(!handle->peers) {
		ESP_LOGE(TAG, "Error allocating peer list");
		esp_now_deinit();
		free(handle);
		return NULL;
	}

	handle->encryption_ctx = malloc(sizeof(*handle->encryption_ctx));
	if(!handle->encryption_ctx) {
		ESP_LOGE(TAG, "Error allocating encryption context");
		free(handle->peers);
		esp_now_deinit();
		free(handle);
		return NULL;
	}

	const wmesh_storage_config_t peer_storage_config = {
		.name = "wmesh_peer"
	};
	err = wmesh_storage_open(&handle->peer_storage, &peer_storage_config);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error opening Mesh Peer storage");
		free(handle->peers);
		free(handle->encryption_ctx);
		esp_now_deinit();
		free(handle);
		return NULL;
	}

	const wmesh_storage_config_t self_storage_config = {
		.name = "wmesh_self"
	};
	err = wmesh_storage_open(&handle->self_storage, &self_storage_config);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error opening Mesh Peer storage");
		wmesh_storage_close(&handle->peer_storage);
		free(handle->peers);
		free(handle->encryption_ctx);
		esp_now_deinit();
		free(handle);
		return NULL;
	}


	wmesh_encryption_config_t enc_cfg = {
		.key = handle->network_key
	};
	memcpy(handle->network_key, config->network_key, sizeof(handle->network_key));
	assert(wmesh_encryption_ctx_new(handle->encryption_ctx, &enc_cfg) == ESP_OK);

	if(wmesh_storage_read_ctr(&handle->self_storage, "ctr", &handle->sequence_number) != WMESH_STORAGE_OK) {
		ESP_LOGI(TAG, "First initialization detected, setting sequence number to 0");
		handle->sequence_number = 0;
	} else {
		ESP_LOGI(TAG, "Current sequence number: %"PRIu64, (uint64_t) handle->sequence_number);
	}

	handle->service_handlers = NULL;
	for(size_t i = 0; config->service_config && config->service_config[i].receive_callback; i++) {
		wmesh_register_service(handle, &config->service_config[i]);
	}

	wmesh_global_handle = handle;
	return handle;
}


esp_err_t wmesh_stop(wmesh_handle_t *handle) {
	wmesh_global_handle = NULL;
	wmesh_peer_list_store(handle->peers, &handle->peer_storage);
	wmesh_storage_write_ctr(&handle->self_storage, "ctr", &handle->sequence_number);

	wmesh_encryption_ctx_free(handle->encryption_ctx);
	free(handle->encryption_ctx);

	wmesh_storage_close(&handle->peer_storage);
	wmesh_storage_close(&handle->self_storage);

	esp_now_deinit();
	free(handle);

	return ESP_OK;
}


esp_err_t wmesh_ll_send(
	wmesh_handle_t *handle, const wmesh_address_t dest,
	const uint8_t *data, size_t data_length
) {
	size_t buffer_size = WMESH_CIPHERTEXT_LENGTH(data_length);
	uint8_t *send_buffer = malloc(buffer_size);
	if(!send_buffer) {
		ESP_LOGE(TAG, "Error allocating %zu bytes", buffer_size);
		return ESP_ERR_NO_MEM;
	}

	esp_err_t err = wmesh_encrypt(
		handle->encryption_ctx,
		&handle->sequence_number,
		data, data_length,

		send_buffer
	);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error encrypting message: %s", esp_err_to_name(err));
		free(send_buffer);
		return err;
	}

	esp_now_peer_info_t peer = {
		.ifidx = WIFI_IF_AP,
		.channel = 0,
	};

	memcpy(&peer.peer_addr, dest, sizeof(wmesh_address_t));
	esp_now_add_peer(&peer);
	err = esp_now_send(dest, send_buffer, buffer_size);
	esp_now_del_peer(dest);
	free(send_buffer);

	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error sending message using ESP-NOW: %s", esp_err_to_name(err));
		return err;
	}

	return ESP_OK;
}


esp_err_t wmesh_send(
	wmesh_handle_t *handle, const wmesh_address_t dest,
	wmesh_service_id_t service,
	const uint8_t *data, size_t data_length
) {
	size_t buffer_size = data_length + 1;
	uint8_t *send_buffer = malloc(buffer_size);
	send_buffer[0] = service;
	memcpy(send_buffer + 1, data, data_length);
	esp_err_t err = wmesh_ll_send(handle, dest, send_buffer, buffer_size);

	free(send_buffer);
	return err;
}


/// @brief Checks whether the Wi-Fi modem is initialized and configured as AP
/// or AP+STA, if not, tries to set it to a compatible mode.
///
/// @return `ESP_OK` or error.
static esp_err_t ensure_ap_mode() {
	esp_err_t err;

	wifi_mode_t wifi_mode;
	err = esp_wifi_get_mode(&wifi_mode);
	switch(err) {
		case ESP_OK:
			break;

		case ESP_ERR_WIFI_NOT_INIT:
			ESP_LOGE(TAG, "Wi-Fi modem not initialized");
			return err;

		case ESP_ERR_INVALID_ARG:
			ESP_LOGE(TAG, "Internal error: %s", esp_err_to_name(err));
			return err;

		default:
			ESP_LOGE(TAG, "Unexpected error: %s", esp_err_to_name(err));
			return err;
	}

	// Check whether Wi-Fi modem is configured as AP; and, if not, whether it
	// can be reconfigured.
	switch(wifi_mode) {
		case WIFI_MODE_AP:
		case WIFI_MODE_APSTA:
			break;

		case WIFI_MODE_STA:
			ESP_LOGW(TAG, "Wi-Fi modem configured as STA. Setting to SoftAP + STA...");

			if((err = esp_wifi_set_mode(WIFI_MODE_APSTA)) != ESP_OK) {
				ESP_LOGE(TAG,
					"Error reconfiguring Wi-Fi modem as SoftAP + STA: %s",
					esp_err_to_name(err)
				);

				return err;
			}

			ESP_LOGW(TAG, "Wi-Fi mode set to SoftAP + STA");

			break;

		default:
			const char *wifi_mode_names[] = {
				[WIFI_MODE_NULL] = "WIFI_MODE_NULL",
				[WIFI_MODE_STA] = "WIFI_MODE_STA",
				[WIFI_MODE_AP] = "WIFI_MODE_AP",
				[WIFI_MODE_APSTA] = "WIFI_MODE_APSTA",
				[WIFI_MODE_NAN] = "WIFI_MODE_NAN",
				[WIFI_MODE_MAX] = "WIFI_MODE_MAX",
			};
			ESP_LOGE(TAG,
				"Incompatible Wi-Fi mode for mesh operation: %s",
				wifi_mode_names[wifi_mode]
			);

			ESP_LOGE(TAG, "Compatible modes are:");
			ESP_LOGE(TAG, "	- WIFI_MODE_AP");
			ESP_LOGE(TAG, "	- WIFI_MODE_APSTA");

			return ESP_ERR_WIFI_MODE;
	}

	return ESP_OK;
}


esp_err_t wmesh_register_service(
	wmesh_handle_t *handle, const wmesh_service_config_t *config
) {
	wmesh_service_handle_t *service_handle = handle->service_handlers;

	if(!service_handle) {
		handle->service_handlers = calloc(1, sizeof(*handle->service_handlers));
		if(!handle->service_handlers) {
			ESP_LOGE(TAG, "Error allocating memory for service handle");
			return ESP_ERR_NO_MEM;
		}
		service_handle = handle->service_handlers;
	} else {
		service_handle = handle->service_handlers;
		while(service_handle->next) {
			service_handle = service_handle->next;
		}

		service_handle = service_handle->next = malloc(sizeof(*handle->service_handlers));
		if(!service_handle) {
			ESP_LOGE(TAG, "Error allocating memory for service handle");
			return ESP_ERR_NO_MEM;
		}
	}

	service_handle->id = config->id;
	service_handle->ctx = config->ctx;
	service_handle->receive_callback = config->receive_callback;
	service_handle->next = NULL;

	ESP_LOGI(TAG, "Added handler for service %"PRIu8, config->id);
	return ESP_OK;
}


esp_err_t wmesh_unregister_service(
	wmesh_handle_t *handle, wmesh_service_id_t id
) {
	if(!handle->service_handlers) {
		return ESP_FAIL;
	}

	// Special case for first node.
	wmesh_service_handle_t *service_handle = handle->service_handlers;
	if(service_handle->id == id) {
		handle->service_handlers = service_handle;
		free(service_handle);
		return ESP_OK;
	}

	while(service_handle->next && service_handle->next->id != id) {
		service_handle = service_handle->next;
	}

	if(service_handle->next == NULL) {
		return ESP_FAIL;
	}

	wmesh_service_handle_t *free_handle = service_handle->next;
	service_handle->next = free_handle->next;
	free(free_handle);

	return ESP_OK;
}


static wmesh_service_handle_t* find_service_handle(wmesh_handle_t *handle, wmesh_service_id_t service_id) {
	if(!handle->service_handlers) {
		return NULL;
	}

	ESP_LOGD(TAG, "Searching service handler for id %"PRIu8, service_id);

	wmesh_service_handle_t *service_handle = handle->service_handlers;
	while(service_handle && service_handle->id != service_id) {
		ESP_LOGD(TAG,
			"Trying %p, id %"PRIu8", callback %p",
			service_handle, service_handle->id, service_handle->receive_callback
		);
		service_handle = service_handle->next;
	}

	if(!service_handle) {
		ESP_LOGD(TAG, "Not found");
	} else {
		ESP_LOGD(TAG,
			"Found %p, id %"PRIu8", callback %p",
			service_handle, service_handle->id, service_handle->receive_callback
		);
	}

	return service_handle;
}


static void recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
	if(data_len < WMESH_CIPHERTEXT_BASE_LENGTH || data_len < 0) {
		return;
	}

	size_t plaintext_length = WMESH_PLAINTEXT_LENGTH((size_t) data_len);
	uint8_t *plaintext = malloc(plaintext_length);
	if(!plaintext) {
		ESP_LOGE(TAG, "Error allocating %zu bytes", plaintext_length);
		return;
	}

	wmesh_peer_list_t *peer_list = wmesh_global_handle->peers;
	wmesh_peer_t *peer = wmesh_peer_list_get_or_create(
		peer_list,
		esp_now_info->src_addr,
		&wmesh_global_handle->peer_storage
	);

	wmesh_decrypt_status_t err = wmesh_peer_decrypt(
		peer, wmesh_global_handle->encryption_ctx,
		data, data_len,
		plaintext
	);

	switch(err) {
		case WMESH_DECRYPT_OK:
			wmesh_service_id_t service_id = plaintext[0];
			wmesh_service_handle_t *service_handle = find_service_handle(wmesh_global_handle, service_id);
			if(service_handle) {

				ESP_LOGD(TAG, "Received message with id %"PRIu8, service_id);
				service_handle->receive_callback(
					wmesh_global_handle,
					esp_now_info->src_addr,
					plaintext + 1,
					plaintext_length - 1,
					service_handle->ctx
				);
			} else {
				ESP_LOGD(TAG,
					"Received message with id %"PRIu8", but no handler is configured",
					service_id
				);
			}
			break;

		case WMESH_DECRYPT_STALE:
			ESP_LOGW(TAG,
				"Received stale data. Expected sequence %"PRIu64,
				(uint64_t) peer->sequence
			);
			break;

		case WMESH_DECRYPT_AUTH_ERROR:
			ESP_LOGE(TAG, "Tag error");
			break;

		case WMESH_DECRYPT_ERROR:
			ESP_LOGE(TAG, "Decryption error");
			break;
	}

	free(plaintext);
}


/// @brief Initializes ESP-NOW.
///
/// @return `ESP_OK` or error.
static esp_err_t initialize_esp_now(const wmesh_config_t *config) {
	esp_err_t err;

	if((err = esp_now_init()) != ESP_OK) {
		ESP_LOGE(TAG, "Error initializing ESP-NOW: %s", esp_err_to_name(err));
		return err;
	}

	uint32_t esp_now_version;
	if(esp_now_get_version(&esp_now_version) == ESP_OK) {
		switch(esp_now_version) {
			case 1:
				ESP_LOGE(TAG, "ESP-NOW version 1.0 is not supported");
				ESP_LOGE(TAG, "Please use ESP-NOW version 2.0");
				esp_now_deinit();
				return ESP_ERR_INVALID_VERSION;
				break;

			case 2:
				ESP_LOGI(TAG, "Using ESP-NOW version 2.0");
				break;

			default:
				ESP_LOGW(TAG,
					"Using unknown ESP-NOW version %" PRIu32,
					esp_now_version
				);
				break;
		}
	} else {
		ESP_LOGE(TAG, "Unable to get ESP-NOW version");
	}

	if((err = esp_now_register_recv_cb(recv_cb)) != ESP_OK) {
		ESP_LOGE(TAG, "Error registering ESP-NOW callback");
		esp_now_deinit();
		return err;
	}

	if((err = esp_now_set_pmk(config->network_key)) != ESP_OK) {
		ESP_LOGE(TAG, "Error registering Primary Master key");
		esp_now_deinit();
		return err;
	}

	ESP_LOGI(TAG, "ESP-NOW initialized");
	return ESP_OK;
}
