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

#ifndef __ALARMMACHINE_H__
#define __ALARMMACHINE_H__

#include "inttypes.h"

typedef enum { Armed, Disarmed, Triggered } AlarmStates;
typedef enum { On, Off } SirenStates;

typedef struct alarmMachine {
    AlarmStates alarmState;
    SirenStates externalSirenState;
    SirenStates internalSirenState;
} AlarmMachine;

void AlarmMachine_Initialise(AlarmMachine* instance);

#endif // #ifndef __ALARMMACHINE_H__
