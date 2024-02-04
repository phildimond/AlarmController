#ifndef __DEFINES_H__
#define __DEFINES_H__

extern const char *TAG;
extern bool mqttConnected;
extern bool ethernetGotIp;

#define PHY_POWER_PIN GPIO_NUM_12
#define BUTTON_PIN_IO GPIO_NUM_34
#define In1_Pin GPIO_NUM_13
#define In2_Pin GPIO_NUM_14
#define In3_Pin GPIO_NUM_15
#define In4_Pin GPIO_NUM_16
#define In5_Pin GPIO_NUM_32
#define In6_Pin GPIO_NUM_33
#define S_TO_uS(s) (s * 1000000)
#define uS_TO_S(s) (s / 1000000)
#define DEBOUNCE_TIME_US 20000

#endif // #ifndef __DEFINES_H__