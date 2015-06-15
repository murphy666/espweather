#ifndef __CONFIG_H
#define __CONFIG_H

#include "cpu_esp8266.h"

#define CONFIG_SECTOR (FLASH_SEC_NUM - 4) //last sector
#define CONFIG_ADDRESS (INTERNAL_FLASH_START_ADDRESS+CONFIG_SECTOR*SPI_FLASH_SEC_SIZE)

#define CONFIG_MAGIC 0x1FAF15FD				 


typedef struct {

	uint32_t magic;
} config_data;

void config_save(config_data * data);
config_data * config_read();
void config_init();



#endif