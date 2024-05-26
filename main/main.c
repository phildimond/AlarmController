/* Home alarm system with MQTT support intended to integrate 
   with Home Assistant, etc.
   
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

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "inttypes.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_spiffs.h"
#include "esp_system.h"

#include "defines.h"
#include "utilities.h"
#include "config.h"
#include "ethernetProcess.h"
#include "mqttProcess.h"
#include "inputOutput.h"

#include "main.h"

extern bool MyEthernetIsConnected;
extern bool MyEthernetGotIp;
extern bool MyEthernetHasDHCP;
extern bool MyMqttConnected;

const char *TAG = "AlarmController";

char s[1024];

const gpio_num_t inputPins[NUM_INPUTS] = {In1_Pin, In2_Pin, In3_Pin, In4_Pin, In5_Pin, In6_Pin};
DebouncedInput inputs[NUM_INPUTS];

void app_main(void)
{
    bool configMode = false;

    // Initial critical IO setup
    initialSetup();

    // If the config button is pressed (or jumped to ground) go into config mode.
    if (buttonPressed()) { ESP_LOGI(TAG, "Button pressed, config mode active"); configMode = true; }

    // Initialise the SPIFFS system
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};
    
    esp_err_t err = esp_vfs_spiffs_register(&spiffs_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "SPIFFS Mount Failed: %s\r\n", esp_err_to_name(err));
        ESP_LOGE(TAG, "Reformatting the SPIFFs partition, please restart.");
        return;
    }

    // Load the configuration from the file system
    bool configLoad = LoadConfiguration();
    if (configLoad == false || config.configOK == false) 
    {
        if (configLoad == false)
        {
            ESP_LOGI(TAG, "Loading the configuration failed. Please enter the configuration details.\r\n");
        }
        else if (config.configOK == false)
        {
            ESP_LOGE(TAG, "The stored configuration is marked as invalid. Please enter the configuration details.\r\n");
        }
        UserConfigEntry();
    }
    else
    {
        ESP_LOGI(TAG, "Loaded config: configOK: %d, Name: %s, Device ID: %s", config.configOK, config.Name, config.DeviceID);
        ESP_LOGI(TAG, "               UID: %s, battVCalFactor: %fV", config.UID, config.battVCalFactor);
        ESP_LOGI(TAG, "               WiFi SSID: %s, WiFi Password: %s", config.ssid, config.pass);
        ESP_LOGI(TAG, "               MQTT URL: %s, Username: %s, Password: %s", config.mqttBrokerUrl, config.mqttUsername, config.mqttPassword);
    }
    
    // If we're in config mode, ask if the user wants to change the config
    if (configMode) {
        printf("\r\nDo you want to change the configuration (y/n)? ");
        char c = 'n';
        if (getLineInput(s, 1) > 0) { c = s[0]; }
        printf("\r\n");
        if (c == 'y' || c == 'Y') { UserConfigEntry(); }
    }

    // Initialise & start the ethernet interface and wait for it to get an IP address
    // If IP acquisition fails, we will continue on as it will auto-connect later if
    // the Ethernet connect becomes available.
    ethernetInitialise();
    ESP_LOGI(TAG, "Waiting for DHCP to complete...");
    int ipWaits = 0;
    while (!MyEthernetGotIp && ipWaits < 40) { 
        vTaskDelay(250 / portTICK_PERIOD_MS); 
        ipWaits++; 
    }
    if (!MyEthernetGotIp) {
        ESP_LOGE(TAG, "Timed out waiting for DHCP. We'll continue on, it will connect later if the connection becomes available.");
    }
    
    // Start mqtt, then wait up to 40 * 0.25 = 10 seconds for it to start. If it doesn't start we'll move on as this is
    // an alarm system, but it should connect later if the MQTT connection becomes available.
    ESP_LOGI(TAG, "Starting the MQTT client...");
    mqtt_app_start();
    int mqttWaits = 0;
    while (!MyMqttConnected && mqttWaits < 40) { vTaskDelay(250 / portTICK_PERIOD_MS); mqttWaits++; } 
    if (MyMqttConnected) { ESP_LOGI(TAG, "MQTT client started after %f seconds.", ((float)mqttWaits) * 0.25); }
    else { ESP_LOGI(TAG, "MQTT client didn't connect after %f seconds, but we'll solider on...", ((float)mqttWaits) * 0.25); }

    // Initialise the inputs
    initialiseInputs(inputs, inputPins, NUM_INPUTS);
    vTaskDelay(50 / portTICK_PERIOD_MS); // Short delay for inputs to stabilise

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT_1, };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t adc1_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, BATT_ADC_CHANNEL, &adc1_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, VIN_ADC_CHANNEL, &adc1_config));

//    int batt_volts_raw = 0;
//    int vin_volts_raw = 0;

    uint64_t last_ADC_Update = esp_timer_get_time();

    uint32_t elevel = 0;
    uint32_t ilevel = 0;

    // Main app loop
    while (true) {

        /*
        if (esp_netif_is_netif_up(eth_netif)) {
            // Ethernet is connected, start MQTT if not already started
            mqtt_app_start();
        } else {
            ESP_LOGI(TAG, "Ethernet is down, trying to reconnect...");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for a second before retrying
        }
        */

        // Read and process any changes to the inputs
        updateInputs(inputs, NUM_INPUTS);
        for (int i = 0; i < NUM_INPUTS; i++) {
            if (inputs[i].changed) {
                //ESP_LOGI(TAG, "Input %d changed to %d", i, inputs[i].currentState);
                bool state = false;
                if (config.inputs[i].normallyClosed) { 
                    if (inputs[i].currentState) { state = true; } else { state = false; } 
                } else {
                    if (inputs[i].currentState) { state = false; } else { state = true; } 
                }
                if (config.inputs[i].active) { 
                    sendState(i, state); 
                } else {
                    ESP_LOGI(TAG, "Not sending to MQTT as input %d is disabled.", i); 
                }
            }
        }

        // Read battery and VIN voltages
        uint64_t usecs = esp_timer_get_time();
        if (usecs - last_ADC_Update > S_TO_uS(5)) {
            last_ADC_Update = usecs;
            /*
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, BATT_ADC_CHANNEL, &batt_volts_raw));
            ESP_LOGI(TAG, "Battery voltage raw ADC value = %d", batt_volts_raw);
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, VIN_ADC_CHANNEL, &vin_volts_raw));
            ESP_LOGI(TAG, "5V rail voltage raw ADC value = %d", vin_volts_raw);
            */

            if (elevel == 0) { 
                //ESP_LOGI(TAG, "Turning external siren ON.");
                gpio_set_level(ExternalSirenPin, 1); 
                elevel = 1;
            } else { 
                //ESP_LOGI(TAG, "Turning external siren OFF.");
                gpio_set_level(ExternalSirenPin, 0); 
                elevel = 0;
            }

            if (ilevel == 0) { 
                //ESP_LOGI(TAG, "Turning downstairs siren ON.");
                gpio_set_level(DownstairsSirenPin, 1); 
                ilevel = 1;
            } else { 
                //ESP_LOGI(TAG, "Turning downstairs siren OFF.");
                gpio_set_level(DownstairsSirenPin, 0); 
                ilevel = 0;
            }

        }
        
        // Sleep and let other tasks run
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }
}
