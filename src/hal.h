#ifndef HAL_H
#define HAL_H

#include "defs.h"

void hal_init();
void hal_init_gpios();
void hal_init_twai();
void hal_init_uart();
void hal_init_i2c();
void hal_init_sd();
void get_rtc_time(char* time_str, int max_len);
void log_alarm_to_sd();
void set_output(int mos_idx, int rel_idx, bool state);
int get_input(int in_idx);

#endif // HAL_H
