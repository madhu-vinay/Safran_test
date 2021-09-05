
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

/* 0- debug off & 1 for on*/
#define debug 0

/* 0- ESPNOW off & 1 for on*/
#define espnow 1

/* 0- UART off & 1 for on*/
#define uart 1

///////////////////// UART inti//////////////////////////
#define TXD_PIN GPIO_NUM_17
#define RXD_PIN GPIO_NUM_16
const int uart_num = UART_NUM_2;
const int uart_tx_buffer_size = (1024 * 2);
const int uart_rx_buffer_size = (1024 * 2);
//////////////////////////////////////////////////////////
esp_now_peer_info_t peerInfo;

/* function declarations */
void Send_Data();
void uart_init();
static void uart_read();
void read_data(char *msg);

/* Global variables*/
int change=0;
uint8_t broadcastAddress1[]    = {0x4c, 0x11,0xae,0x75,0xd7,0x54};   /*Mac address of the sender device */
char *TAGEspNow               = "ESPNOW MODE: ";
char *recievedData;
struct timeval tv_now;
char num_char[1];
char  *jsonToString;

int num=6;
char item_char[100]="1102,2110,1023,2310,1021,3109";
char count_char[100]="10,10,5,10,15,20";

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
  read_data(recievedData);
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
    change=1;
    uart_init();
    xTaskCreate(Send_Data, "Send_Data", 8192, NULL, 10, NULL);
    xTaskCreate(uart_read, "uart_read", 8192, NULL, 10, NULL);
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

/**
 * @brief this function sends the data in JSON format
 *  via UART and ESPNOW 
 */
void initMessage(){ 
    sprintf(&num_char[0],"%d",num); 
    cJSON *mainMessage       = cJSON_CreateObject();
    cJSON *count_j = cJSON_CreateString(count_char);
    cJSON_AddItemToObject(mainMessage, "count", count_j);
     cJSON *item_j = cJSON_CreateString(item_char);
    cJSON_AddItemToObject(mainMessage, "item", item_j);
     cJSON *num_j = cJSON_CreateString(num_char);
    cJSON_AddItemToObject(mainMessage, "number", num_j);

    jsonToString = cJSON_Print(mainMessage); 
    ESP_LOGI(TAGEspNow, "Data to sent:%s ",jsonToString);
    #if espnow
    sendDataOverEspNow(jsonToString,1);
    #endif

    #if uart
    const int txBytes = uart_write_bytes(UART_NUM_2, (const char*)jsonToString, strlen(jsonToString));
    if (txBytes > 0) 
    {
     printf("Transmitt SUccessfully\n");
                
    }
    #endif
}

/**
 * @brief If the device gets an input from the sensor it will send 
 * the data to other devices
 */
void Send_Data()
{
    while(1)
    {
        if(change==1)
        {
        initMessage();
        change=0;
        }
        vTaskDelay(500);
    }

}

/**
 * @brief function to initialise UART 
 */
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

/**
 * @brief If this device gets data from other devices it calls this functions
 * this function breaks the received data and compares with the data present
 * and appends if there is any data missed to the devices data.
 */
static void uart_read()
{
    //ESP_LOGI(TAG,"rx_task_inilizie\n");
     static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data1 = (uint8_t*) malloc(uart_rx_buffer_size+1);
    int rxBytes;
        // Read data from UART.
     while(1)
        {
          rxBytes = uart_read_bytes(uart_num, data1, uart_rx_buffer_size, 1000 / portTICK_RATE_MS);
           vTaskDelay(500 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data1[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes,(char*)data1);
                        char * recievedData2  = (char*)data1;

         read_data(recievedData2);
         }    

    vTaskDelay(100);   
        }
}
        
void read_data(char *msg)
{
  cJSON *parsedData = cJSON_Parse(msg);   //Parse JSON data from incoming message
  /*If our incoming message is really a JSON message--------------------------------------*/
  if(parsedData != NULL)
  {
    char *numKey= cJSON_GetObjectItem(parsedData,"number")->valuestring;
    char *itemKey= cJSON_GetObjectItem(parsedData,"item")->valuestring;
    char *countKey= cJSON_GetObjectItem(parsedData,"count")->valuestring;
    #if debug
    printf("%s\n", countKey);
    printf("%s", numKey);
    printf("%s\n", itemKey);
    #endif

  
  int len = atoi(numKey);
  int item_arr[len];
  int count_arr[len];
  int org_item_arr[num];

  char delim[]=",";
  char *ptr = strtok(itemKey,delim);
  int j=0;
  while(ptr!=NULL)
  {
      #if debug
      printf("%d\n",atoi(ptr));
      #endif
     item_arr[j]=atoi(ptr);
     j++;
      ptr=strtok(NULL,delim);
  }
  j=0;
  int k=0;
  char *ptr1 = strtok(countKey,delim);
  while(ptr1!=NULL)
  {
    #if debug
    printf("%d\n",atoi(ptr1));
    #endif
    count_arr[k]=atoi(ptr1);
     k++;
    ptr1=strtok(NULL,delim);
  }

  char tmp_item[100];
  strcpy(tmp_item,item_char);
  int l=0;
  char *ptr2 = strtok(tmp_item,delim);
  while(ptr2!=NULL)
  {
    #if debug
    printf("%d\n",atoi(ptr2));
    #endif
    org_item_arr[l]=atoi(ptr2);
    l++;
    ptr2=strtok(NULL,delim);
  }

  for(int i=0;i<len;i++)
  {
      int flag=0;
   for(j=0;j<num;j++)
   {
       #if debug
       printf("in %d///////%d\n",item_arr[i],org_item_arr[j]);
       #endif
       if(item_arr[i]==org_item_arr[j])
       {
        flag=1;
       break;
       }
   }
   if(flag==0)
   {
       char str[10];
       sprintf(str,"%d",item_arr[i]);
       strcat(item_char,",");
       strcat(item_char,str);
       
       sprintf(str,"%d",count_arr[i]);
       strcat(count_char,",");
       strcat(count_char,str);
       num=num+1;
   }

  }
    #if debug
    printf("%s/n",item_char);
    printf("%s/n",count_char);
    #endif
  
    sprintf(&num_char[0],"%d",num); 
    
    cJSON *mainMessage       = cJSON_CreateObject();
    cJSON *count_j = cJSON_CreateString(count_char);
    cJSON_AddItemToObject(mainMessage, "count", count_j);
     cJSON *item_j = cJSON_CreateString(item_char);
    cJSON_AddItemToObject(mainMessage, "item", item_j);
     cJSON *num_j = cJSON_CreateString(num_char);
    cJSON_AddItemToObject(mainMessage, "number", num_j);

    jsonToString = cJSON_Print(mainMessage); 
    printf("\nData after synching :%s ",jsonToString); 
   }
  else
  {
      ESP_LOGI(TAGEspNow, "No data found");
  }
}
