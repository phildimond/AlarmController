/* MQTT Alarm Controller: MQTT Processing
   
   Reads the config from and writes it to SPIFFS, and allows for entering
   the configuration data.

   Copyright 2024 Phillip C Dimond

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include "esp_log.h"
#include "inttypes.h"

#include "utilities.h"
#include "config.h"
#include "mqttProcess.h"

int mqttMessagesQueued = 0;
bool gotTime = false;
int year = 0, month = 0, day = 0, hour = 0, minute = 0, seconds = 0;
esp_mqtt_client_handle_t client;

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    char topic[160];
    char payload[2000];
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;
        case MQTT_EVENT_CONNECTED:
            mqttConnected = true;
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            // Subscribe to the time feed
            msg_id = esp_mqtt_client_subscribe(client, "homeassistant/CurrentTime", 0);
            ESP_LOGI(TAG, "Subscribe sent for time feed, msg_id=%d", msg_id);

            // Send the alarm sensor configurations.
            // Use the same command and state topics so we don't have to echo commands to state
            char id[80];
            for (int i = 0; i < 6; i++) {
                if (config.inputs[i].active) {

                    // Make up the input sensor's unique ID string
                    strcpy (id, config.UID);
                    int l = strlen(config.UID);
                    id[l] = '-'; id[l+1] = (char)(i + 0x30); id[l+2] = '\0';

                    // Send the config for this sensor
                    sprintf(topic, "homeassistant/binary_sensor/%s/%s/config", config.Name, config.inputs[i].inputName);
                    sprintf(payload, "{\"unique_id\": \"%s\", \
                        \"device\": {\"identifiers\": [\"%s\"], \"name\": \"%s\"}, \
                        \"availability\": {\"topic\": \"homeassistant/binary_sensor/%s/availability\", \
                        \"payload_on\": \"ON\", \"payload_off\": \"OFF\"}, \
                        \"name\": \"%s\", \"retain\":true, \"device_class\": \"motion\", \
                        \"state_topic\": \"homeassistant/binary_sensor/%s/%s/state\"}",
                        id, config.DeviceID, config.Name, config.Name, config.inputs[i].descriptiveName, config.Name, config.inputs[i].inputName);
                    msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
                    mqttMessagesQueued++;
                    ESP_LOGI(TAG, "Published %s config message successfully, msg_id=%d", config.inputs[i].descriptiveName, msg_id);
                }
            }

            // Send an online message
            sprintf(topic, "homeassistant/binary_sensor/%s/availability", config.Name);
            sprintf(payload, "online");
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published online message successfully, msg_id=%d, topic=%s", msg_id, topic);
    
            break;
        case MQTT_EVENT_DISCONNECTED:
            mqttConnected = false;
            ESP_LOGE(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            mqttMessagesQueued--;
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            mqttMessagesQueued--;
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA");
            //ESP_LOGI(TAG, "Event topic length = %d and data length = %d", event->topic_len, event->data_len);
            strncpy(topic, event->topic, event->topic_len);
            topic[event->topic_len] = '\0';
            //ESP_LOGI(TAG, "Received an event - topic was %s", topic);
            if (strcmp(topic, "homeassistant/CurrentTime") == 0) {
                // Process the time
                //ESP_LOGI(TAG, "Got the time from %s, as %.*s.", topic, event->data_len, event->data);
                gotTime = true;
                strncpy(payload, event->data, event->data_len);
                payload[event->data_len] = 0;
                sscanf(payload, "%d.%d.%d %d:%d:%d", &year, &month, &day, &hour, &minute, &seconds);
                // Send an online every 10 seconds
                if (seconds % 10 == 0) {
                    sprintf(topic, "homeassistant/binary_sensor/%s/availability", config.Name);
                    sprintf(payload, "online");
                    msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); // Temp sensor config, set the retain flag on the message
                    mqttMessagesQueued++;
                    ESP_LOGI(TAG, "Published online message successfully, msg_id=%d, topic=%s", msg_id, topic);
                }
            } else {
                ESP_LOGI(TAG, "Received unexpected message, topic %s", topic);
            }            
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR. ");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGE(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void mqtt_app_start(void)
{
    char lwTopic[100];
    sprintf(lwTopic, "homeassistant/binary_sensor/%s/availability", config.Name);
    const char* lwMessage = "offline\0";
    esp_mqtt_client_config_t mqtt_cfg = {
        .network = {
            .reconnect_timeout_ms = 250, // Reconnect MQTT broker after this many ms
        },
        .broker.address.uri = config.mqttBrokerUrl,
        .credentials = { 
            .username = config.mqttUsername, 
            .authentication = { 
                .password = config.mqttPassword
            }, 
        },
        .session = {
            .message_retransmit_timeout = 250,  // ms transmission retry
            .protocol_ver = MQTT_PROTOCOL_V_3_1_1,
            .keepalive = 30, // 30 second keepalive timeout
            .last_will = {
                .topic = lwTopic,
                .msg = (const char*)lwMessage,
                .msg_len = strlen(lwMessage),
                .qos = 1,
                .retain = 1
            }
        },
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_err_t err = esp_mqtt_client_start(client);
    if (err != ESP_OK) { ESP_LOGE(TAG, "MQTT client start error: %s", esp_err_to_name(err)); }
}

