/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef __SENSORS_H
#define __SENSORS_H

typedef struct{

	float temp;
	float hum;

} sht21_data;


typedef struct {

	sht21_data sht21;
} sensor_data;


void ICACHE_FLASH_ATTR sensors_init();
void ICACHE_FLASH_ATTR sensors_get_data(sensor_data *return_data);


#endif