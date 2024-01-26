#include "esp_log.h"
#include "inttypes.h"

#include "utilities.h"
#include "config.h"
#include "mqttProcess.h"

int mqttMessagesQueued = 0;
bool gotTime = false;
int year = 0, month = 0, day = 0, hour = 0, minute = 0, seconds = 0;
extern char* s;
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
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
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
            sprintf(topic, "homeassistant/binary_sensor/HouseAlarm/HallwayMotion/config");
            sprintf(payload, "{\"unique_id\": \"7y3fghn8y8jjh\", \
                \"device\": {\"identifiers\": [\"HouseAlarm-ID\"], \"name\": \"HouseAlarm\"}, \
                \"availability\": {\"topic\": \"homeassistant/binary_sensor/HouseAlarm/availability\", \
                \"payload_on\": \"ON\", \"payload_off\": \"OFF\"}, \
                \"name\": \"Hallway Motion\", \"retain\":true, \"device_class\": \"motion\", \
                \"state_topic\": \"homeassistant/binary_sensor/HouseAlarm/HallwayMotion/state\"}");
                //,config.UID, config.DeviceID, config.Name, config.Name, config.Name, config.Name);
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published Alarm Office Motion config message successfully, msg_id=%d", msg_id);

            sprintf(topic, "homeassistant/binary_sensor/HouseAlarm/RumpusMotion/config");
            sprintf(payload, "{\"unique_id\": \"34876fh3f738as\", \
                \"device\": {\"identifiers\": [\"HouseAlarm-ID\"], \"name\": \"HouseAlarm\"}, \
                \"availability\": {\"topic\": \"homeassistant/binary_sensor/HouseAlarm/availability\", \
                \"payload_on\": \"ON\", \"payload_off\": \"OFF\"}, \
                \"name\": \"Rumpus Motion\", \"retain\":true, \"device_class\": \"motion\", \
                \"state_topic\": \"homeassistant/binary_sensor/HouseAlarm/RumpusMotion/state\"}");
                //,config.UID, config.DeviceID, config.Name, config.Name, config.Name, config.Name);
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published Alarm Rumpus Motion config message successfully, msg_id=%d", msg_id);

            sprintf(topic, "homeassistant/binary_sensor/HouseAlarm/EntryMotion/config");
            sprintf(payload, "{\"unique_id\": \"nklg65iyvb68go\", \
                \"device\": {\"identifiers\": [\"HouseAlarm-ID\"], \"name\": \"HouseAlarm\"}, \
                \"availability\": {\"topic\": \"homeassistant/binary_sensor/HouseAlarm/availability\", \
                \"payload_on\": \"ON\", \"payload_off\": \"OFF\"}, \"device_class\": \"motion\", \
                \"name\": \"Entry Motion\", \"retain\":true, \
                \"state_topic\": \"homeassistant/binary_sensor/HouseAlarm/EntryMotion/state\"}");
                //,config.UID, config.DeviceID, config.Name, config.Name, config.Name, config.Name);
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published Alarm Entry Motion config message successfully, msg_id=%d", msg_id);

            sprintf(topic, "homeassistant/binary_sensor/HouseAlarm/LoungeMotion/config");
            sprintf(payload, "{\"unique_id\": \"7b6gliyvu6fku6f5\", \
                \"device\": {\"identifiers\": [\"HouseAlarm-ID\"], \"name\": \"HouseAlarm\"}, \
                \"availability\": {\"topic\": \"homeassistant/binary_sensor/HouseAlarm/availability\", \
                \"payload_on\": \"ON\", \"payload_off\": \"OFF\"}, \"device_class\": \"motion\", \
                \"name\": \"Lounge Motion\", \"retain\":true, \
                \"state_topic\": \"homeassistant/binary_sensor/HouseAlarm/LoungeMotion/state\"}");
                //,config.UID, config.DeviceID, config.Name, config.Name, config.Name, config.Name);
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published Alarm Lounge Motion config message successfully, msg_id=%d", msg_id);

            /*            
            // Send the curtailment enable switch configuration. Use the relay number availability topic
            // Use the same command and state topics so we don't have to echo commands to state
            sprintf(topic, "homeassistant/switch/%s/config",config.Name);
            sprintf(payload, "{\"unique_id\": \"S_%s\", \"retain\": \"true\", \
                \"device\": {\"identifiers\": [\"%s\"], \"name\": \"%s\"}, \
                \"availability\": {\"topic\": \"homeassistant/number/%s/availability\", \"payload_available\": \"online\", \"payload_not_available\": \"offline\"}, \
                \"command_topic\": \"homeassistant/switch/%s/command\", \"state_topic\": \"homeassistant/switch/%s/command\"}"
                ,config.UID, config.DeviceID, config.Name, config.Name, config.Name, config.Name);
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published Envoy Relay config message successfully, msg_id=%d", msg_id);

            // Send the manual control switch configuration. Use the relay number availability topic
            // Use the same command and state topics so we don't have to echo commands to state
            sprintf(topic, "homeassistant/switch/%s-manual/config",config.Name);
            sprintf(payload, "{\"unique_id\": \"S_%s-manual\", \"retain\": \"true\", \
                \"device\": {\"identifiers\": [\"%s\"], \"name\": \"%s\"}, \
                \"availability\": {\"topic\": \"homeassistant/number/%s/availability\", \"payload_available\": \"online\", \"payload_not_available\": \"offline\"}, \
                \"command_topic\": \"homeassistant/switch/%s-manual/command\", \"state_topic\": \"homeassistant/switch/%s-manual/command\"}"
                ,config.UID, config.DeviceID, config.Name, config.Name, config.Name, config.Name);
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published Envoy Relay config message successfully, msg_id=%d", msg_id);

            */

            // Send an online message
            sprintf(topic, "homeassistant/binary_sensor/HouseAlarm/availability");
            sprintf(payload, "online");
            msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); 
            mqttMessagesQueued++;
            ESP_LOGI(TAG, "Published House Alarm online message successfully, msg_id=%d, topic=%s", msg_id, topic);
    
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
            /*
            //ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            strncpy(s, event->topic, event->topic_len);
            s[event->topic_len] = '\0';
            ESP_LOGV(TAG, "Received an event - topic was %s", s);
            if (strcmp(s, "homeassistant/CurrentTime") == 0) {
                // Process the time
                ESP_LOGV(TAG, "Got the time from %s, as %.*s.", s, event->data_len, event->data);
                gotTime = true;
                strncpy(s, event->data, event->data_len);
                s[event->data_len] = 0;
                sscanf(s, "%d.%d.%d %d:%d:%d", &year, &month, &day, &hour, &minute, &seconds);

                // Send an online every 10 seconds
                if (seconds % 10 == 0) {
                    sprintf(topic, "homeassistant/number/%s/availability", config.Name);
                    sprintf(payload, "online");
                    msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 1); // Temp sensor config, set the retain flag on the message
                    mqttMessagesQueued++;
                    ESP_LOGV(TAG, "Published Envoy Relay online message successfully, msg_id=%d, topic=%s", msg_id, topic);
                }

            } else if (strstr(s, "command") != NULL) {
                if (strstr(s, "number")) {
                    strncpy(s, event->data, event->data_len);
                    s[event->data_len] = 0;
                    ESP_LOGV(TAG, "Received command %s.", s);
                    uint8_t val = (uint8_t)(atoi((const char*)s));
                    // use this value to set the relays if we're in manual control
                    commandedRelayValue = val;
                    if (manualControl == true && val <= 15) { // Unsigned so always >= 0
                        oldRelayValue = relayValue;
                        relayValue = val;
                        ESP_LOGV(TAG, "Set relay value to $%02X", relayValue);
                    }      
                } else if (strstr(s, "switch")) {
                    // Switch state changed from Home Assistant
                    if (strstr(s, "manual")) { // Enable switch?
                        strncpy(s, event->data, event->data_len);
                        s[event->data_len] = 0;
                        if (strstr(s, "ON")) { 
                            manualControl = true; 
                            relayValue = commandedRelayValue;
                        } else { manualControl = false; }
                        ESP_LOGI(TAG, "Manual control switch state change received %s - changed to %d", s, manualControl);
                    } else { // must be the curtailment switch
                        strncpy(s, event->data, event->data_len);
                        s[event->data_len] = 0;
                        if (strstr(s, "ON")) { curtailmentEnabled  = true; } else { curtailmentEnabled = false; }
                        ESP_LOGI(TAG, "Curtailment switch state change received %s - changed to %d", s, curtailmentEnabled);
                    }
                } else {
                    // Don't know what this topic was
                    ESP_LOGE(TAG, "Received unknown command topic: %s", s);
                }
            } else if (strcmp(s, "homeassistant/Power") == 0) {
                strncpy(s, event->data, event->data_len);
                s[event->data_len] = 0;
                ESP_LOGV(TAG, "Received power data: %s", s);
                if (PowerManager_Decode(&powerValues, (const char*)s) == 0) {
                    ESP_LOGV(TAG, "Successfully decoded power values from JSON string.");
                    powerValuesUpdated = true;    // Flag that we have received valid power values
                } else {
                    ESP_LOGE(TAG, "Error decoding power values from JSON string.");
                }
            } else {
                ESP_LOGI(TAG, "Received unexpected message, topic %s", s);
            }
            */
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
    sprintf(lwTopic, "homeassistant/number/%s/availability", config.Name);
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

