#include "nodes/common.h"


cJSON *spv_telemetry_msg_to_json(spv_telemetry_msg *msg) {
	cJSON *array = cJSON_CreateArray();
	for(size_t i = 0; i < msg->num_datos; i++) {
		cJSON *json = spv_telemetry_reading_to_json(
			&msg->datos[i],
			msg->node_name,
			msg->fecha + i * CONFIG_SPV_SENSOR_READ_FREQUENCY_SECONDS
		);
		cJSON_AddItemToArray(array, json);
	}

	return array;
}


cJSON* spv_telemetry_reading_to_json(
	spv_telemetry_reading_t *reading,
	const char *node_name,
	spv_timestamp_t timestamp
) {
	cJSON *json = cJSON_CreateObject();
	cJSON *telemetry_json = cJSON_AddObjectToObject(json, node_name);
	cJSON_AddNumberToObject(
		telemetry_json, "ts", timestamp * 1000
	);

	const char *telemetry_names[] = {
		[SPV_ULP_NOISE_READING] = "noise",
		[SPV_ULP_LUMINOSITY_READING] = "luminosity",
		[SPV_ULP_CO2_READING] = "co2",
		[SPV_ULP_VOC_READING] = "voc",
		[SPV_ULP_HUMIDITY_READING] = "humidity",
		[SPV_ULP_TEMPERATURE_READING] = "temperature",
	};
	for(spv_ulp_reading_type_t type = SPV_ULP_READING_FIRST + 1; type < SPV_ULP_READING_LAST; type++) {
		switch(type) {
			case SPV_ULP_NOISE_READING:
				cJSON_AddNumberToObject(telemetry_json, telemetry_names[type], reading->noise);
				break;
			case SPV_ULP_LUMINOSITY_READING:
				cJSON_AddNumberToObject(telemetry_json, telemetry_names[type], reading->luminosity);
				break;
			case SPV_ULP_CO2_READING:
				cJSON_AddNumberToObject(telemetry_json, telemetry_names[type], reading->co2);
				break;
			case SPV_ULP_VOC_READING:
				cJSON_AddNumberToObject(telemetry_json, telemetry_names[type], reading->voc);
				break;
			case SPV_ULP_HUMIDITY_READING:
				cJSON_AddNumberToObject(telemetry_json, telemetry_names[type], reading->humidity);
				break;
			case SPV_ULP_TEMPERATURE_READING:
				cJSON_AddNumberToObject(telemetry_json, telemetry_names[type], reading->temperature);
				break;

			default:
				break;
		}
	}

	return json;
}
