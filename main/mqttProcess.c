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

bool MyMqttConnected = false;

int mqttMessagesQueued = 0;
bool gotTime = false;
int year = 0, month = 0, day = 0, hour = 0, minute = 0, seconds = 0;
esp_mqtt_client_handle_t client;

/******************************************************************************************************
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 ******************************************************************************************************/
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
            MyMqttConnected = true;
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            // Subscribe to the time feed
            msg_id = esp_mqtt_client_subscribe(client, "homeassistant/CurrentTime", 0);
            ESP_LOGI(TAG, "Subscribe sent for time feed, msg_id=%d", msg_id);

            // Subscribe to the siren state feeds
            sprintf(topic, "homeassistant/siren/%s/ExternalSiren/command", config.Name);
            msg_id = esp_mqtt_client_subscribe(client, topic, 0);
            ESP_LOGI(TAG, "Subscribe sent for the external siren event feed, msg_id=%d", msg_id);

            sprintf(topic, "homeassistant/siren/%s/DownstairsSiren/command", config.Name);
            msg_id = esp_mqtt_client_subscribe(client, topic, 0);
            ESP_LOGI(TAG, "Subscribe sent for the external siren event feed, msg_id=%d", msg_id);

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

            // Alarm Siren configurations
            sprintf(topic, "homeassistant/siren/%s/ExternalSiren/config", config.Name);
            sprintf(payload, "{\"unique_id\": \"%s-ES\", \
                \"device\": {\"identifiers\": [\"%s\"], \"name\": \"%s\", \"manufacturer\": \"Phillip Dimond\"}, \
                \"availability\": {\"topic\": \"homeassistant/siren/%s/availability\", \
                \"payload_on\": \"ON\", \"payload_off\": \"OFF\"}, \
                \"name\": \"External Siren\", \"retain\":true, \"device_class\": \"siren\", \
                \"command_topic\": \"homeassistant/siren/%s/ExternalSiren/command\"}",
                id, config.DeviceID, config.Name, config.Name, config.Name);
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published %s config message successfully, msg_id=%d", "ExternalSiren", msg_id);

            sprintf(topic, "homeassistant/siren/%s/DownstairsSiren/config", config.Name);
            sprintf(payload, "{\"unique_id\": \"%s-DS\", \
                \"device\": {\"identifiers\": [\"%s\"], \"name\": \"%s\", \"manufacturer\": \"Phillip Dimond\"}, \
                \"availability\": {\"topic\": \"homeassistant/siren/%s/availability\", \
                \"payload_on\": \"ON\", \"payload_off\": \"OFF\"}, \
                \"name\": \"Downstairs Interior Siren\", \"retain\":true, \"device_class\": \"siren\", \
                \"command_topic\": \"homeassistant/siren/%s/DownstairsSiren/command\"}",
                id, config.DeviceID, config.Name, config.Name, config.Name);
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published %s config message successfully, msg_id=%d", "DownstairsSiren", msg_id);

            // Send online messages
            sprintf(topic, "homeassistant/binary_sensor/%s/availability", config.Name);
            sprintf(payload, "online");
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published sensor online message successfully, msg_id=%d, topic=%s", msg_id, topic);

            sprintf(topic, "homeassistant/siren/%s/availability", config.Name);
            sprintf(payload, "online");
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published siren switch online message successfully, msg_id=%d, topic=%s", msg_id, topic);

            // Send input initial states
            for (int i = 0; i < NUM_INPUTS; i++) {
                if (config.inputs[i].active) {
                    sprintf(topic, "homeassistant/binary_sensor/%s/%s/state", config.Name, config.inputs[i].inputName);
                    sprintf(payload, "OFF");
                    int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
                    mqttMessagesQueued++;
                    ESP_LOGI(TAG, "Published state message for input %d, %s = %s successfully, msg_id=%d", 
                        i, config.inputs[i].descriptiveName, payload, msg_id);
                }
            }

            // Send initial siren states
            sprintf(topic, "homeassistant/siren/%s/ExternalSiren/command", config.Name);
            sprintf(payload, "\"state\":\"OFF\"");
            int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published initial command message for External Siren successfully, msg_id=%d", msg_id);

            // Send initial siren states
            sprintf(topic, "homeassistant/siren/%s/DownstairsSiren/command", config.Name);
            sprintf(payload, "\"state\":\"OFF\"");
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published initial command message for External Siren successfully, msg_id=%d", msg_id);

            break;
        case MQTT_EVENT_DISCONNECTED:
            MyMqttConnected = false;
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
                    msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
                    mqttMessagesQueued++;
                    ESP_LOGI(TAG, "Published sensor online message successfully, msg_id=%d, topic=%s", msg_id, topic);

                    sprintf(topic, "homeassistant/siren/%s/availability", config.Name);
                    sprintf(payload, "online");
                    msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
                    mqttMessagesQueued++;
                    ESP_LOGI(TAG, "Published siren switch online message successfully, msg_id=%d, topic=%s", msg_id, topic);
                    
                }
            } else {
                strncpy(payload, event->data, event->data_len);
                payload[event->data_len] = 0;
                ESP_LOGI(TAG, "Received unexpected message, topic=%s, payload=%s", topic, payload);
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

/***************************************************************************************************
 * 
 * Start the MQTT processes. 
 * 
 * Instantiates and starts the MQTT communication system
 * 
 * *************************************************************************************************/

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

/********************************************************************************************************
 * 
 * Send MQTT Alarm state change event
 * 
 * Send a state change message to MQTT
 * 
 *******************************************************************************************************/
void sendState(int inputNumber, bool active)
{
    char topic[200];
    char payload[20];

    // Send a state message for the specified input
    sprintf(topic, "homeassistant/binary_sensor/%s/%s/state", config.Name, config.inputs[inputNumber].inputName);
    if (active) { sprintf(payload, "ON"); } else { sprintf(payload, "OFF"); }
    int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
    mqttMessagesQueued++;
    ESP_LOGI(TAG, "Published state message for input %d, %s = %s successfully, msg_id=%d", 
        inputNumber, config.inputs[inputNumber].descriptiveName, payload, msg_id);
}
