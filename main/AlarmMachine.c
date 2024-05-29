/* MQTT Alarm Controller: Alarm processing machine

    Handles the actual alarm system: takes input events, manages the alarm's
    state and being armed and disarmed, etc.

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
#include "AlarmMachine.h"

/* ---------- External Siren State --------------*/
void SetExternalSirenState(AlarmMachine* instance, SirenStates state)
{
    switch (state) {
        case Off:
            gpio_set_level(instance->externalSirenPin, 1); 
            SendSirenState("ExternalSiren", true);
            break;
        case On:
            gpio_set_level(instance->externalSirenPin, 1); 
            SendSirenState("ExternalSiren", true);
            break;
    }
}

/* ---------- Internal Siren State --------------*/
void SetInternalSirenState(AlarmMachine* instance, SirenStates state)
{
    switch (state) {
        case Off:
            gpio_set_level(instance->internalSirenPin, 1); 
            SendSirenState("InternalSiren", true);
            break;
        case On:
            gpio_set_level(instance->internalSirenPin, 1); 
            SendSirenState("InternalSiren", true);
            break;
    }
}

/*
-----------------------------------------------------------------------------------
    Initialise the alarm machine
-----------------------------------------------------------------------------------
*/
void AlarmMachine_Initialise(AlarmMachine* instance, gpio_num_t externalSirenPin, gpio_num_t internalSirenPin)
{
    instance->externalSirenPin = externalSirenPin;
    instance->internalSirenPin = internalSirenPin;
    instance->alarmState = Disarmed;
    instance->externalSirenState = Off;
    instance->internalSirenState = Off;
}

bool AlarmMachine_SetAlarmState(AlarmMachine* instance, AlarmStates state)
{
    instance->alarmState = state;
    if (state == Disarmed) {
    }
    return true;
}
