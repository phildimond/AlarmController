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

#ifndef __MQTTPROCESS_H__
#define __MQTTPROCESS_H__

#include "mqtt_client.h"
#include "defines.h"

void mqtt_app_start(void);

#endif // #ifndef __MQTTPROCESS_H__