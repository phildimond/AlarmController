/* Ethernet Basic Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
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
#include "esp_spiffs.h"
#include "esp_system.h"

#include "ethernet_init.h"

#include "defines.h"
#include "utilities.h"
#include "config.h"
#include "mqttProcess.h"
#include "main.h"

const char *TAG = "AlarmController";

bool ethernetGotIp = false;
bool mqttConnected = false;
char s[1024];

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ethernetGotIp = true;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

void app_main(void)
{
    bool configMode = false;

    // Enable Power to PHY - needed for Olimex ESP32-POE board
    const gpio_num_t phy_power_pin = 12;
    gpio_config_t phy_power_conf = {0};
    phy_power_conf.mode = GPIO_MODE_OUTPUT;
    phy_power_conf.pin_bit_mask = (1ULL << phy_power_pin);
    ESP_ERROR_CHECK(gpio_config(&phy_power_conf));
    ESP_ERROR_CHECK(gpio_set_level(phy_power_pin, 1));
    
    // Config button
    const gpio_num_t button_pin = BUTTON_PIN_IO;
    gpio_config_t button_conf = {0};
    button_conf.mode = GPIO_MODE_INPUT;
    button_conf.pin_bit_mask = (1ULL << button_pin);
    button_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&button_conf));

    // If the config button is pressed (or jumped to ground) go into config mode.
    if (gpio_get_level(button_pin) == 0) { ESP_LOGI(TAG, "Button pressed, config mode active"); configMode = true; }

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

    // Initialize Ethernet driver
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create instance(s) of esp-netif for Ethernet(s)
    if (eth_port_cnt == 1) {
        // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
        // default esp-netif configuration parameters.
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        esp_netif_t *eth_netif = esp_netif_new(&cfg);
        // Attach Ethernet driver to TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[0])));
    } else {
        // Use ESP_NETIF_INHERENT_DEFAULT_ETH when multiple Ethernet interfaces are used and so you need to modify
        // esp-netif configuration parameters for each interface (name, priority, etc.).
        esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
        esp_netif_config_t cfg_spi = {
            .base = &esp_netif_config,
            .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
        };
        char if_key_str[10];
        char if_desc_str[10];
        char num_str[3];
        for (int i = 0; i < eth_port_cnt; i++) {
            itoa(i, num_str, 10);
            strcat(strcpy(if_key_str, "ETH_"), num_str);
            strcat(strcpy(if_desc_str, "eth"), num_str);
            esp_netif_config.if_key = if_key_str;
            esp_netif_config.if_desc = if_desc_str;
            esp_netif_config.route_prio -= i*5;
            esp_netif_t *eth_netif = esp_netif_new(&cfg_spi);

            // Attach Ethernet driver to TCP/IP stack
            ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[i])));
        }
    }

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    // Start Ethernet driver state machine
    for (int i = 0; i < eth_port_cnt; i++) {
        ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
    }

    // Wait for the Ethernet to get an IP address
    ESP_LOGI(TAG, "Waiting for DHCP to complete...");
    int ipWaits = 0;
    while (!ethernetGotIp && ipWaits < 40) { 
        vTaskDelay(250 / portTICK_PERIOD_MS); 
        ipWaits++; 
    }
    if (!ethernetGotIp) {
        ESP_LOGE(TAG, "Timed out waiting for DHCP. Rebooting.");
        vTaskDelay(2000 / portTICK_PERIOD_MS); 
        esp_restart();
    }
    
    // Start mqtt, then wait up to 40 * 0.25 = 10 seconds for it to start
    ESP_LOGI(TAG, "Starting the MQTT client...");
    mqtt_app_start();
    int mqttWaits = 0;
    while (!mqttConnected && mqttWaits < 40) { vTaskDelay(250 / portTICK_PERIOD_MS); mqttWaits++; } 
    ESP_LOGI(TAG, "MQTT client started after %f seconds.", ((float)mqttWaits) * 0.25);

    // Initialise the inputs
    int inputVals[8];
    int previousInputVals[8];
    const gpio_num_t inputPins[6] = {In1_Pin, In2_Pin, In3_Pin, In4_Pin, In5_Pin, In6_Pin};
    gpio_config_t inputConfigs[6] = { {0}, {0}, {0}, {0}, {0}, {0} };
    for (int i = 0; i < 6; i++) {
        inputConfigs[i].mode = GPIO_MODE_INPUT;
        inputConfigs[i].pin_bit_mask = (1ULL << inputPins[i]);
        inputConfigs[i].pull_up_en = GPIO_PULLUP_ENABLE;
        ESP_ERROR_CHECK(gpio_config(&inputConfigs[i]));
    }
    vTaskDelay(50 / portTICK_PERIOD_MS); // Short delay for inputs to stabilise
    for (int i = 0; i < 6; i++) {
        inputVals[i] = gpio_get_level(inputPins[i]);
        previousInputVals[i] = inputVals[i];
    }

    // Main app loop
    while (true) {
        // Read and process any changes to the inputs
        for (int i = 0; i < 6; i++) {
            inputVals[i] = gpio_get_level(inputPins[i]);
            if (inputVals[i] != previousInputVals[i]) {
                ESP_LOGI(TAG, "Input %d changed state from %d to %d", i, previousInputVals[i], inputVals[i]);
                previousInputVals[i] = inputVals[i];
            }
        }

        // Sleep and let other tasks run
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }
}
