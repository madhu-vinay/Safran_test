
#include "espNow.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include <stdio.h>
#include <stdlib.h>


#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "sdkconfig.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_spiffs.h"
#include "driver/uart.h"
#include "esp_vfs_fat.h"
#include "nvs.h"



/* #define GATTS_TAG "GATTS_DEMO"

#define NVS_PART_THINGNAME                             "thing_name"
#define NVS_PART_THINGTYPE                        "thing_type"
#define NVS_PART_THINGSECRET                      "thing_secret"
#define NAMESPACE                                 "info"

#define BLE_DEVICE_NAME            "ESP_GATTS_DEMO"
#define BLE_MANUFACTURER_DATA_LEN  17
#define SVC_INST_ID                 0
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))
#define GATTS_CHAR_NUM              5
#define GATTS_DESCR_NUM             11

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024
#define SLEEP_TIME 60000000 */

uint8_t broadcastAddress[]    = {0x24, 0x6F,0x28,0x24,0xEE,0x48};   /*Mac address of the sender device */
char *TAGEspNow               = "ESPNOW MODE: ";
char *testData;
char *recievedData;
int   state                   = 1;       //Initial state        
char *compareMessage          = "hello"; //Compare string for incoming messages
int mode =1;        
int prvmode=1;
int state2=0;
int state3=0;
int k=0;
esp_now_peer_info_t peerInfo;

#define TXD_PIN  GPIO_NUM_17
#define RXD_PIN  GPIO_NUM_16

static const char TAG[] = "UART_SLAVE";
static void rx_task();

const int uart_num = UART_NUM_2;
const int uart_tx_buffer_size = (1024 * 2);
const int uart_rx_buffer_size = (1024 * 2);
char * file_name = "/spiffs/Device_info.txt";

int First_Flag =0;
int device_id =0;
void file_remove(char* name_, char* type_, char* mac_);
void file_add(char* name_, char* type_, char* mac_ );
int line_delete(int delete_line);
uint32_t hex2int(char *hex);

    //char* file_name ;   
    char* FILE_NAME ;
    char* path_name = "/spiffs/" ;
    char* ext_name  = ".txt";
    char *buf ;
    cJSON *tree = NULL;
    char *actual = NULL;
    cJSON *tree_1 = NULL;


/////////////////////////////ESPNOW-CODE///////////////////////////////////////////


void init() {
    ESP_LOGI(TAG,"_inilizie\n");
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
ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_rx_buffer_size,uart_tx_buffer_size, 0, NULL, 0));
//ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_rx_buffer_size,uart_tx_buffer_size, 0, NULL, 0));
}

/*Function to set new state------------------------------------------------------------------*/
//void setState(int sta){
  //  state = sta;
//}

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

  /*If our incoming message is really a JSON message--------------------------------------*/
  if(parsedData != NULL){
      messageKey = cJSON_GetObjectItem(parsedData,"message"); //Search key "message"
      typeKey    = cJSON_GetObjectItem(parsedData,"Type");    //Search key "type"

      /*KEY "message" was found*/
      if(messageKey != NULL){
          
          char *helloMessage = cJSON_GetObjectItem(parsedData,"message")->valuestring;
          ESP_LOGI(TAGEspNow, "Received message %s:",helloMessage);

          /*Message is "hello"*/
          if(*helloMessage == *compareMessage){
              ESP_LOGI(TAGEspNow, "Received message match, will send Welcome message");
              state = 2;
            }
            /*Message is different from "hello (Time to send information over MQTT)*/
          else{
              ESP_LOGI(TAGEspNow, "Received message not match");
          }
      }
      /*KEY "type" was found*/
      if(typeKey != NULL){
          cJSON *Devices1 = NULL;
          char *typeMessage = cJSON_GetObjectItem(parsedData,"type")->valuestring;
          ESP_LOGI(TAGEspNow, "Received message %s:",typeMessage);
           cJSON *mainloop = cJSON_CreateObject();
          cJSON *report = cJSON_CreateString("10AM"); 
           cJSON_AddItemToObject(mainloop,"REPORT",report);
            cJSON *Device1 = cJSON_CreateObject();

                cJSON *gasSensor =cJSON_CreateString(cJSON_GetObjectItem(parsedData,"Device_Name")->valuestring);
                cJSON_AddItemToObject(Device1, "Device_Name", gasSensor);
                cJSON *type = cJSON_CreateString(cJSON_GetObjectItem(parsedData,"Type")->valuestring); 
                cJSON_AddItemToObject(Device1, "Type",type);
                cJSON *value = cJSON_CreateString(cJSON_GetObjectItem(parsedData,"Value")->valuestring); 
                cJSON_AddItemToObject(Device1, "Value",value);

            //cJSON_AddItemToArray(Devices1, Device1);
            cJSON_AddItemToObject(mainloop, "Device1", Device1);

            char *json = NULL;
            json = cJSON_Print(mainloop);
            printf("%s", json);
             const int txBytes = uart_write_bytes(UART_NUM_2, (const char*)json, strlen(json));
            if (txBytes > 0) {
               printf("Transmitt SUccessfully\n");
        
              }
             else if(txBytes==-1)
             {
                 printf("Paramettor error\n");
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            }
      }
       vTaskDelay(3000 / portTICK_PERIOD_MS);

  }else{
      ESP_LOGI(TAGEspNow, "No data found");
  }

}

void startEspNow(){

     esp_err_t ret;
    
     ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return 0;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    init();
   // xTaskCreate(rx_task, "rx_task", 8096, NULL, 10, NULL);

    /*Init ESPNow protocol ---------------------------------------------------------------------*/
    if (esp_now_init() != ESP_OK) {
        ESP_LOGI(TAGEspNow, "Error at Espnow init");
        return;
    }
    else{
        ESP_LOGI(TAGEspNow, "Espnow init OK");
    }

    /*Add the mac address of our ESP32 Slave---------------------------------------------------*/
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK){
    ESP_LOGI(TAGEspNow, "Error adding peer");
    return;
    }
    else{
        ESP_LOGI(TAGEspNow, "Adding peer OK");
    }

    /*Call back function to recieve and send data-----------------------------------------------*/
    esp_now_register_recv_cb(onReceiveData);
    esp_now_register_send_cb(OnDataSent);


    while(1){
        rx_task();
        /*State 1 is where ESP32 Master is waiting for incoming messages from peer devices*/
         if(state == 1){
            ESP_LOGI(TAGEspNow, "Waiting for messages");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            state=2;
        }
        //State 2 send the acknolegde to the peer device so can send the new information
        //Send the acknolegde 5 times to be sure that will be recieved
        if(state == 2)
           {
                ESP_LOGI(TAGEspNow, "Sending read/write");
                initMessage(); 
                vTaskDelay(1000 / portTICK_PERIOD_MS);
              if(k==1)
              {
                  cJSON *mainloop = cJSON_CreateObject();
                cJSON *report = cJSON_CreateString("10AM"); 
                cJSON_AddItemToObject(mainloop,"REPORT",report);
                char  *jsonToString;
                cJSON *mainMessage       = cJSON_CreateObject();
                cJSON *type              = NULL; 
                cJSON *data              = NULL;
                cJSON *deviceId          = NULL; 

                type = cJSON_CreateString("gas sensor"); 
                cJSON_AddItemToObject(mainMessage,"Device_Name",type);

                data = cJSON_CreateString("123"); 
                cJSON_AddItemToObject(mainMessage,"Value",data);

                deviceId = cJSON_CreateString("Sensor");
                cJSON_AddItemToObject(mainMessage,"Type",deviceId);

                jsonToString = cJSON_Print(mainMessage); 
                ESP_LOGI(TAGEspNow, "Data to sent:%s ",jsonToString);
                cJSON_AddItemToObject(mainloop, "Device1", mainMessage);

                char *json = NULL;
                    json = cJSON_Print(mainloop);
                    printf("%s", json);
                    const int txBytes = uart_write_bytes(UART_NUM_2, (const char*)json, strlen(json));
                    if (txBytes > 0) {
                    printf("Transmitt SUccessfully\n");
                
                    }
                    else if(txBytes==-1)
                    {
                        printf("Paramettor error\n");
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
            }

              }
            if(state2==2)
            {
              for(int i = 0; i<=2; i++)
              {
                ESP_LOGI(TAGEspNow, "Sending mode");
                initMessage2(); 
                vTaskDelay(1000 / portTICK_PERIOD_MS);
              }
            }
            state = 1;
        }
 
    }

}

/*Function to send data over ESPNow, recieve the message that want to be send --------------*/
void sendDataOverEspNow(char *message){
    
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)message, 250);

    if (result == ESP_OK) {
        ESP_LOGI(TAGEspNow, "Data sent successfully");
    }
    else {
        ESP_LOGI(TAGEspNow, "Error sending data");
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}

/*Initial response message for the slaves devices "Welcome"-------------------------------*/
void initMessage(){
    /*Build the JSON message---------------------------------------*/
    /*JSON Format variables to create messages---------------------*/
    ////////// Read Mode////////

    char  *jsonToString;
    cJSON *mainMessage       = cJSON_CreateObject();
    cJSON *welcomeMessage    = NULL;  
    if(prvmode!=mode || state3==1)
    {
      welcomeMessage = cJSON_CreateString("read"); 
	  cJSON_AddItemToObject(mainMessage,"message",welcomeMessage);
      jsonToString = cJSON_Print(mainMessage); 
      ESP_LOGI(TAGEspNow, "Data to sent:%s ",jsonToString);
      sendDataOverEspNow(jsonToString);
      prvmode=mode;
      state2=2;
      state3=0;
    }
    else
    {
       welcomeMessage = cJSON_CreateString("write"); 
	  cJSON_AddItemToObject(mainMessage,"message",welcomeMessage);
      jsonToString = cJSON_Print(mainMessage); 
      ESP_LOGI(TAGEspNow, "Data to sent:%s ",jsonToString);
      sendDataOverEspNow(jsonToString);
    }
}
void initMessage2()
{
   char  *jsonToString;
    cJSON *mainMessage       = cJSON_CreateObject();
    cJSON *welcomeMessage    = NULL;  

	  cJSON_AddItemToObject(mainMessage,"mode",welcomeMessage);
      jsonToString = cJSON_Print(mainMessage); 
      ESP_LOGI(TAGEspNow, "Data to sent:%s ",jsonToString);
      sendDataOverEspNow(jsonToString);
}

static void rx_task()
{
    ESP_LOGI(TAG,"rx_task_inilizie\n");
     static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data1 = (uint8_t*) malloc(uart_rx_buffer_size+1);
    int rxBytes;
        // Read data from UART.
        //const int rxBytes = 0;
        //ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&rxBytes));
          rxBytes = uart_read_bytes(uart_num, data1, uart_rx_buffer_size, 1000 / portTICK_RATE_MS);
           vTaskDelay(500 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data1[rxBytes] = 0;
            // vTaskDelay(2000 / portTICK_PERIOD_MS);
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes,(char*)data1);
            //ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
             vTaskDelay(1000 / portTICK_PERIOD_MS);
             //printf("uart_num_0\n");
            //uart_write_bytes(UART_NUM_0, (const char *) data, rxBytes);
            //prase_json_data((const char*)data);
            char * recievedData2  = (char*)data1;
            printf("\nreceived data %s\n",recievedData2);
            cJSON *parsedData = cJSON_Parse(recievedData2);
            char *token = cJSON_GetObjectItem(parsedData,"Token")->valuestring;
            if(token!= NULL)
            {
                state3=1;
                if(strcmp(token,"add_slave")==0)
                {
                  int j =0;
                  int* code_val;
                   tree_1             = cJSON_Parse(recievedData2);
                   cJSON *name        = cJSON_GetObjectItem(tree_1,"Device_Name");
                   char* device_name  = cJSON_GetObjectItem(tree_1,"Device_Name")->valuestring;
                    cJSON *type        = cJSON_GetObjectItem(tree_1,"Device_Type");
                  char  *device_type = cJSON_GetObjectItem(tree_1,"Device_Type")->valuestring;
                    cJSON *mac_        = cJSON_GetObjectItem(tree_1,"Device_MAC");
                     char  *mac_value   = cJSON_GetObjectItem(tree_1,"Device_MAC")->valuestring;
                      int a= 0;
                        char *cds = strtok(mac_value, ",");
                        while (cds) {
                        if (a < 7) {
                        broadcastAddress[a++] = hex2int(cds);
                        }
                        cds = strtok(NULL, ",");
                        }
                    for (int i = 0; i < 6; i++) {
                    printf("%02X",broadcastAddress[i]);
            v
                    if (i < 5){printf(":");}
                    }
                    printf("/////////////////received mac addresss////////////////////////");
                    k=1;
                    /* FILE* f = fopen(file_name, "w");
                        if (f == NULL) {
                        ESP_LOGE(TAG, "Failed to open  the file %s for reading", file_name);
                        return;
                        }
                    char buf8[1024];
                    f = fopen(file_name, "r");
                        char* content1 = fread(buf8, 1, 1024, f); 
                        printf("content: %s\n", buf8);
                    fclose(f);
                     file_add(device_name,  device_type,  mac_value ); */
                }
                if(strcmp(token,"add_buzzer")==0)
                {
                    int j =0;
                  int* code_val;
                   tree_1             = cJSON_Parse(recievedData2);
                   cJSON *name        = cJSON_GetObjectItem(tree_1,"Device_Name");
                   char* device_name  = cJSON_GetObjectItem(tree_1,"Device_Name")->valuestring;
                    cJSON *type        = cJSON_GetObjectItem(tree_1,"Device_Type");
                  char  *device_type = cJSON_GetObjectItem(tree_1,"Device_Type")->valuestring;
                    cJSON *mac_        = cJSON_GetObjectItem(tree_1,"Device_MAC");
                     char  *mac_value   = cJSON_GetObjectItem(tree_1,"Device_MAC")->valuestring;
                    FILE* f = fopen(file_name, "w");
                        if (f == NULL) {
                        ESP_LOGE(TAG, "Failed to open  the file %s for reading", file_name);
                        return;
                        }
                    char buf8[1024];
                    f = fopen(file_name, "r");
                        char* content1 = fread(buf8, 1, 1024, f); 
                        printf("content: %s\n", buf8);
                    fclose(f);
                     file_add(device_name,  device_type,  mac_value );
                    printf("\nsending the received data to slave sensor\n");
                    sendDataOverEspNow(recievedData2);
                }
                else
                {
                    printf("\n invalid token \n");
                }
                
            }
        }
    free(data1);
    vTaskDelay(100);
}

uint32_t hex2int(char *hex) {
    uint32_t val = 0;
    while (*hex) {
        // get current character then increment
        uint8_t byte = *hex++; 
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;    
        // shift 4 to make space for new digit, and add the 4 bits of the new digit 
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}

void file_add(char* name_, char* type_, char* mac_ )
{

    struct stat st;
    FILE* fp;
    char buf[1024];
    int Func_Found_Flag = 0;
   int line_no = 0;

            printf("read file\n"); 
            fp = fopen(file_name, "r");
            while(fgets(buf, 1024,fp) !=NULL ) {
                line_no++;
                if((strstr(buf,name_)) != NULL) {
                   Func_Found_Flag = 1; 
                   break;
                }  

            }  
    printf("line no : %d\n",line_no);
    if (Func_Found_Flag ==0) {
        if (First_Flag==0){
            ESP_LOGI(TAG, "opening file for Writing");
                fp = fopen(file_name, "w");
                First_Flag= 1;
                if (fp == NULL) {
                    ESP_LOGE(TAG, "Failed to open file for appending");
                    return;
                } 

        }
        else{
                ESP_LOGI(TAG, "opening file for appending");
                fp = fopen(file_name, "a");
                if (fp == NULL) {
                    ESP_LOGE(TAG, "Failed to open file for appending");
                    return;
                } 
        }
            device_id++;
            fprintf (fp, "Device_Name: %s\n",name_);
            fprintf (fp, "Device_Type: %s\n",type_);
            fprintf (fp, "Device_MAC : %s\n",mac_ );
            fprintf (fp, "Device_%d\n",device_id );

        fclose(fp);
         char buf9[1024];
        fp= fopen(file_name, "r");
        char* content2= fread(buf9, 1, 1024, fp); 
        printf("content: %s\n", buf9);

            fclose(fp);  
            ESP_LOGI(TAG, "File is written");
    }
    else{
        ESP_LOGI(TAG, "Data is already present ");
    }
}

void file_remove(char* name_, char* type_, char* mac_)
{
    char buf[1024];
    int Func_Found_Flag = 0;
    int line_no = 0;
            printf("read file\n"); 
            FILE* fp = fopen(file_name, "r");
            while(fgets(buf, 1024,fp) !=NULL ) {
                line_no++;
                if((strstr(buf,name_)) != NULL) {
                   Func_Found_Flag = 1; 
                   break;
                }   
            } 
            fclose(fp);
    printf("remove--line no:%d\n",line_no); 
    int n = line_no+3;
    printf("n:%d\n",n);
    for(int i =line_no;i<= (line_no+3);i++){
        line_delete(line_no);
        printf("for loop\n");
       
    }

}


int line_delete(int delete_line){
    int ctr = 0;
    
        FILE *fptr1, *fptr2;
        //char fname[256];
        char str[256]="";
        char* temp = "/spiffs/temp.txt";
        printf("\n\n Delete a specific line from a file :\n");
        
          
        fptr1 = fopen(file_name, "r");
        if (!fptr1) 
        {
                printf(" File not found or unable to open the input file!!\n");
                return 0;
        }
       
         fptr2 = fopen(temp, "w"); // open the temporary file in write mode 
        if (!fptr2) 
        {
                printf("Unable to open a temporary file to write!!\n");
                fclose(fptr1);
                return 0;
        }
      
        while (!feof(fptr1)) 
        {
            strcpy(str, "\0");
            fgets(str, 256, fptr1);
            if (!feof(fptr1)) 
            {
                ctr++;
                /* skip the line at given line number */
                if (ctr != delete_line) 
                {
                    printf("%s",str);
                    fprintf(fptr2, "%s", str);
                }
            }
        }
        fclose(fptr1);
        fclose(fptr2);
        remove(file_name);          // remove the original file
       rename(temp, file_name);    // rename the temporary file to original name
       return 0;
}