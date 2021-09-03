
#include "espNow.h"
#include "wifiConfig.h"
#include "mqtt.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/uart.h"
#include "esp_adc_cal.h"
#include <time.h>
#include <math.h>
#include <sys/time.h>

///////////////////// UART inti//////////////////////////
#define TXD_PIN GPIO_NUM_17
#define RXD_PIN GPIO_NUM_16
const int uart_num = UART_NUM_2;
const int uart_tx_buffer_size = (1024 * 2);
const int uart_rx_buffer_size = (1024 * 2);
//////////////////////////////////////////////////////////

uint8_t broadcastAddress1[]    = {0x24, 0x6F,0x28,0x24,0xEE,0x48};   /*Mac address of the sender device */
char *TAGEspNow               = "ESPNOW MODE: ";
char *testData;
char *recievedData;
int   state                   = 0;       //Initial state        
char *welcomeMessage = "NONE";
struct timeval tv_now;
esp_now_peer_info_t peerInfo;
void Send_Data();
void set_clock(int x);
void uart_init();
/*Callback function when ESPNow send a message-----------------------------------------------*/
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  ESP_LOGI(TAGEspNow,"Last Packet Sent to: "); ESP_LOGI(TAGEspNow,"MAC: %s",macStr);
  ESP_LOGI(TAGEspNow,"Last Packet Send Status: "); 
}

/*Call back function to recieve data over ESPNow--------------------------------------------*/
void onReceiveData(const uint8_t *mac, const uint8_t *data, int len) {
 
  ESP_LOGI(TAGEspNow, "Received from MAC:");
  for (int i = 0; i < 6; i++) {
 
    printf("%02X", mac[i]);
    if (i < 5)printf(":");
  }

  /*Convert uint8_t data in to char--------------------------------------------------------*/
  recievedData  = (char*)data;
  ESP_LOGI(TAGEspNow, "Data recieved %s",recievedData);
  cJSON *parsedData = cJSON_Parse(recievedData);   //Parse JSON data from incoming message
  cJSON *messageKey = NULL;                        //JSON Object to search "Message" KEY
  cJSON *typeKey = NULL;                           //JSON Object to search "type" KEY
  //char  *jsonToString;

  /*If our incoming message is really a JSON message--------------------------------------*/
  if(parsedData != NULL)
  {

      
  }
  else
  {
      ESP_LOGI(TAGEspNow, "No data found");
  }

}

void startEspNow()
{

    /*Init ESPNow protocol ---------------------------------------------------------------------*/
    if (esp_now_init() != ESP_OK) 
    {
        ESP_LOGI(TAGEspNow, "Error at Espnow init");
        return;
    }
    else
    {
        ESP_LOGI(TAGEspNow, "Espnow init OK");
    }
    
    /*Add the mac address of our ESP32---------------------------------------------------*/
    memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;

    if ((esp_now_add_peer(&peerInfo)) != ESP_OK)
    {
    ESP_LOGI(TAGEspNow, "Error adding peer");
    return;
    }
    else
    {
        ESP_LOGI(TAGEspNow, "Adding peer OK");
    }

    /*Call back function to recieve and send data-----------------------------------------------*/
    esp_now_register_recv_cb(onReceiveData);
    esp_now_register_send_cb(OnDataSent);
    uart_init();
    set_clock(0);
    xTaskCreate(Send_Data, "Send_Data", 8192, NULL, 10, NULL);
}

/*Function to send data over ESPNow, recieve the message that want to be send --------------*/
void sendDataOverEspNow(char *message, int type){
    
    
       esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t *)message, 250);
    
    if (result == ESP_OK) {
        ESP_LOGI(TAGEspNow, "Data sent successfully");
    }
    else {
        ESP_LOGI(TAGEspNow, "Error sending data");
    } 
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}


void initMessage(){
    int a[5]={1,2,3,4,5};
    char msg[5];
    char  *jsonToString;
    for(int i=0;i<5;i++)
    {
        sprintf(&msg[i],"%d",a[i]);
    }
    cJSON *mainMessage       = cJSON_CreateObject();
    cJSON *Device1 = cJSON_CreateObject();
    


    
    jsonToString = cJSON_Print(mainMessage); 
    ESP_LOGI(TAGEspNow, "Data to sent:%s ",msg);
    sendDataOverEspNow(msg,1);
}

void Send_Data()
{
    while(1)
    {
        vTaskDelay(200);
        printf("Current Epoch Time [ %ld ]\n", time(&tv_now));
        initMessage();
    }

}

void set_clock(int x)
{
    tv_now.tv_sec = x;
    tv_now.tv_usec = 0;
    settimeofday(&tv_now, NULL);
}

void uart_init()
{
    //ESP_LOGI(TAG, "_inilizie\n");
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    // Set UART pins(TX: IO16 (UART2 default), RX: IO17 (UART2 default), RTS: IO18, CTS: IO19)
    ESP_ERROR_CHECK(uart_set_pin(uart_num, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    //uart_driver_install(uart_num, size_of_rx_buf_ring, size_of_tx_buf_ring,size_of_queue_,uart_queue,intr_flag)
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_rx_buffer_size, uart_tx_buffer_size, 0, NULL, 0));
    //ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_rx_buffer_size,uart_tx_buffer_size, 0, NULL, 0));
}