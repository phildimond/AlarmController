/* MQTT Alarm Controller: I/O Management
   
   Configuration, reading and debouncing of inputs and output management.

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

#include "inttypes.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "config.h"
#include "defines.h"

#include "inputOutput.h"

/******************************************************************
 * 
 * Initial Setup
 * 
 * Initialise critical IO functions required for startup
 * 
*******************************************************************/
void initialSetup()
{
    // Enable Power to PHY - needed for Olimex ESP32-POE board
    const gpio_num_t phy_power_pin = PHY_POWER_PIN;
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
}

/******************************************************************
 * 
 * Initial Setup
 * 
 * Initialise critical IO functions required for startup
 * 
*******************************************************************/
void initialiseInputs(DebouncedInput inputs[], const gpio_num_t pins[], int numInputs)
{
    gpio_config_t inputConfigs[numInputs];
    for (int i = 0; i < numInputs; i++) {
        inputs[i].gpioNumber = pins[i];
        inputConfigs[i].intr_type = GPIO_INTR_DISABLE;
        inputConfigs[i].mode = GPIO_MODE_INPUT;
        inputConfigs[i].pin_bit_mask = (1ULL << inputs[i].gpioNumber);
        inputConfigs[i].pull_down_en = GPIO_PULLDOWN_DISABLE;
        inputConfigs[i].pull_up_en = GPIO_PULLUP_ENABLE;
        ESP_ERROR_CHECK(gpio_config(&inputConfigs[i]));
        inputs[i].changeStart = 0;
        inputs[i].currentState = gpio_get_level(inputs[i].gpioNumber);
        inputs[i].previousState = inputs[i].currentState;
        inputs[i].changed = false;
    }
}

/******************************************************************
 * 
 * Is the board button pressed?
 * 
*******************************************************************/
bool buttonPressed(void) {
    if (gpio_get_level(BUTTON_PIN_IO) == 0) {
        return true;
    }
    return false;
}

/******************************************************************
 * 
 * Update the input states
 * 
*******************************************************************/
void updateInputs(DebouncedInput inputs[], int numInputs)
{
    for (int i = 0; i < numInputs; i++) {
        inputs[i].changed = false;
        int level = gpio_get_level(inputs[i].gpioNumber);
        //ESP_LOGI(TAG, "Read input %d, io %d value %d", i, inputs[i].gpioNumber, level);
        if (level != inputs[i].currentState && inputs[i].changeStart == 0) {
            if (inputs[i].changeStart == 0) { 
                ESP_LOGI(TAG, "Input %d changed state, start debouncing.", i);
                inputs[i].changeStart = esp_timer_get_time(); 
            }
        }
        if (inputs[i].changeStart != 0) {
            if (esp_timer_get_time() - inputs[i].changeStart > DEBOUNCE_TIME_US) {
                ESP_LOGI(TAG, "Input %d debounced.", i);
                inputs[i].changeStart = 0;
                if (inputs[i].currentState != level) { 
                    inputs[i].previousState = inputs[i].currentState;
                    inputs[i].currentState = level;
                    //ESP_LOGI(TAG, "Input %d changed to %d.", i, inputs[i].currentState);
                    inputs[i].changed = true; 
                } else { 
                    inputs[i].changed = false; 
                    ESP_LOGI(TAG, "Input %d didn't change after debounce time.", i);
                }
            }
        }
    }
}
