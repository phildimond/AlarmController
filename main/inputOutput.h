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

#ifndef __INPUTOUTPUT_H__
#define __INPUTOUTPUT_H__

#include "inttypes.h"

typedef struct {
  gpio_num_t gpioNumber;
  int previousState;
  int currentState;
  int64_t changeStart;
  bool changed;
} DebouncedInput;

void initialSetup(void);
void initialiseInputs(DebouncedInput inputs[], const gpio_num_t pins[], int numInputs);
bool buttonPressed(void);
void updateInputs(DebouncedInput inputs[], int numInputs);

#endif // #ifndef __INPUTOUTPUT_H__