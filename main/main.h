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

#ifndef __MAIN_H__
#define __MAIN_H__

#define TWDT_TIMEOUT_MS 10000 // Watchdog timeout in milliseconds

void app_main(void);

#endif // __MAIN_H__