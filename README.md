# espweather

Description: 

Small WIFI weather station based on the ESP8266 and SHT21 sensor. 

The weather station is using MQTT protocol and send three simple topics : temperature/, humidity/, and voltage/ (for battery monitoring). 

This code is tested on ESP-01 module only.

Thanks to :

israellot for esp-ginx which this project is based on (https://github.com/israellot/esp-ginx)

reaper7 for the sht21 sensor support (https://github.com/reaper7/esp8266_i2c_sht21)

zarya for pure software based i2c implementation (https://github.com/zarya/esp8266_i2c_driver)
