/* MQTT Alarm Controller: Ethernet Management
   
   Configuration & management of the Ethernet connection.

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

#ifndef __ETHERNETPROCESS_H__
#define __ETHERNETPROCESS_H__

#include "esp_event.h"
#include "inttypes.h"

void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void ethernetInitialise(void);

#endif // #ifndef __ETHERNETPROCESS_H__
