#include "c_types.h"
#include "c_string.h"
#include "c_stdio.h"
#include "user_interface.h"
#include "mem.h"
#include "osapi.h"
#include "user_config.h"
#include "espconn.h"

#include "mqtt.h"
#include "mqtt_msg.h"

#include "sensor/sensors.h"

#include "serial_number.h"

#define MQTT_SERVER_IP "192.168.0.101"

static os_timer_t mqtt_timer;
static MQTT_Client mqtt_client;

static os_timer_t sensor_read_timer;

static uint8_t wifiStatus = STATION_IDLE, lastWifiStatus = STATION_IDLE;

static void ICACHE_FLASH_ATTR mqtt_app_timer_cb(void *arg){

	
	struct ip_info ipConfig;
	
	wifi_get_ip_info(STATION_IF, &ipConfig);	
	wifiStatus = wifi_station_get_connect_status();

	//check wifi
	if(wifiStatus != lastWifiStatus){

		lastWifiStatus = wifiStatus;
		
		if(wifiStatus==STATION_GOT_IP && ipConfig.ip.addr != 0){
        	MQTT_DBG("MQTT: Detected wifi network up");
        	MQTT_Connect(&mqtt_client);       
	    }
	    else{
	    	MQTT_DBG("MQTT: Detected wifi network down");
	    	MQTT_Disconnect(&mqtt_client);    
	    }

	}
	    
}



static void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	MQTT_DBG("MQTT: Connected");

	int i;
	char * buff = (char *)os_zalloc(64);
        uint32_t vdd33 = readvdd33();
        
	sensor_data data;
	sensors_get_data(&data);	

	c_sprintf(buff,"%f",data.sht21.temp);
	MQTT_Publish(&mqtt_client, "temperature/"SERIAL_NUMBER, buff, strlen(buff), 0, 1);

	os_memset(buff,0,64);
	c_sprintf(buff,"%f",data.sht21.hum);
	MQTT_Publish(&mqtt_client, "humidity/"SERIAL_NUMBER, buff, strlen(buff), 0, 1);

	os_memset(buff,0,64);
	c_sprintf(buff,"%f",(float)(vdd33/1000.0));
	MQTT_Publish(&mqtt_client, "voltage/"SERIAL_NUMBER, buff, strlen(buff), 0, 1);
        
        os_free(buff);

}

static void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	MQTT_DBG("MQTT: Disconnected");
}

static void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	MQTT_DBG("MQTT: Published");
}

static void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	MQTT_DBG("Receive topic: %s, data: %s ", topicBuf, dataBuf);

	os_free(topicBuf);
	os_free(dataBuf);
}

void ICACHE_FLASH_ATTR mqtt_app_init(){

	MQTT_InitConnection(&mqtt_client, MQTT_SERVER_IP, 1883, 0);
	MQTT_InitClient(&mqtt_client, SERIAL_NUMBER, NULL, NULL, 120, 1);

	MQTT_OnConnected(&mqtt_client, mqttConnectedCb);
	MQTT_OnDisconnected(&mqtt_client, mqttDisconnectedCb);
	MQTT_OnPublished(&mqtt_client, mqttPublishedCb);
	MQTT_OnData(&mqtt_client, mqttDataCb);
	
	//arm mqtt timer
	os_memset(&mqtt_timer,0,sizeof(os_timer_t));
	os_timer_disarm(&mqtt_timer);
	os_timer_setfn(&mqtt_timer, (os_timer_func_t *)mqtt_app_timer_cb, NULL);
}