#include "platform.h"
#include "c_string.h"
#include "c_stdlib.h"
#include "c_stdio.h"

#include "osapi.h"

#include "user_interface.h"
#include "user_config.h"

#include "ets_sys.h"
#include "driver/uart.h"
#include "mem.h"

#include "serial_number.h"
#include "mqtt/app.h"

#include "driver/i2c.h"
#include "driver/i2c_sht21.h"
#include "sensor/sensors.h"

os_timer_t sleepTimer;

static void sleepTimerCb(void *arg){
    system_deep_sleep(5*60*1000*1000);    

}

void user_rf_pre_init(){}

static void config_wifi(){
    NODE_DBG("Putting AP UP");

    struct station_config config = {
        .ssid = "matrix",
        .password = "lamarque2864",
    };
    wifi_station_set_auto_connect(1); 
    /* required to call wifi_set_opmode before station_set_config */
    wifi_set_opmode(0x01);
    wifi_station_set_config(&config);

    
}
void user_init(void)
{   
    
    uart_init(BIT_RATE_115200,BIT_RATE_115200);
    config_wifi();
    i2c_init();
    SHT21_Init();
    sensors_init();
    mqtt_app_init();    

    //Deep sleep timer
    os_memset(&sleepTimer,0,sizeof(os_timer_t));
    os_timer_disarm(&sleepTimer);
    os_timer_setfn(&sleepTimer, (os_timer_func_t *)sleepTimerCb, NULL);
    os_timer_arm(&sleepTimer, 15000, 1);
}
