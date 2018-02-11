#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"

int str2morse(int n, const char* str);
const char* morse(char c);
void morse_send(const char* msg);
/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void task_blink(void* ignore)
{
  const char* str = "hola     mundo";
  const char* buf = str;
  //calcula el tamaño del string
  int n = 0; //tamaño del string
  while(*buf != '\0'){
      n++;
      buf++;
  }
  //llama a la funcion str2morse que convierte el string de caracteres en un string de morse
  str2morse(n, str);
  vTaskDelete(NULL);
}
/******************************************************************************
 * FunctionName : morse
 * Description  : make a equivalence table between morse and ascii
 * Parameters   : character to be trasnformed
 * Returns      : character transformed into morse
*******************************************************************************/
const char* morse(char c){
  static const char* morse_ch[] = {
    "._ ", /*A*/
    "_... ", /*B*/
    "_._. ", /*C*/
    "_.. ", /*D*/
    ". ", /*E*/
    ".._. ", /*F*/
    "__. ", /*G*/
    ".... ", /*H*/
    ".. ", /*I*/
    ".___ ", /*J*/
    "_._ ", /*K*/
    "._.. ", /*L*/
    "__ ", /*M*/
    "_. ", /*N*/
    "___ ", /*O*/
    ".__. ", /*P*/
    "__._ ", /*Q*/
    "._. ", /*R*/
    "... ", /*S*/
    "_ ", /*T*/
    ".._ ", /*U*/
    "..._ ", /*V*/
    ".__ ", /*W*/
    "_.._ ", /*X*/
    "_.__ ", /*Y*/
    "__.. ", /*Z*/
    "_____ ",/*0*/
    ".____ ", /*1*/
    "..___ ",/*2*/
    "...__ ",/*3*/
    "...._ ",/*4*/
    "..... ",/*5*/
    "_.... ",/*6*/
    "__... ",/*7*/
    "___.. ",/*8*/
    "____. ",/*9*/
  };
  //si el caracter recibido es un espacio devuelve un espacio tambien
  if(c == ' '){
    return " ";
  }
  //en otro caso devuelve el valor del caracter en la tabla
  else{
    return morse_ch[c - 'a'];
  }

}
/******************************************************************************
 * FunctionName : str2morse
 * Description  : changes the characteres string to a morse string and calls the morse_send
                  function to send the message using the ESP8266s' LED
 * Parameters   : n, string
 * Returns      : none
*******************************************************************************/
int str2morse(int n, const char* str){
  const char* msg;
  int i;
  //recorre cada letra del string y la reproduce con la funcion morse_send letra a letra
  for(i=0;i<n;i++){
    msg = morse(*str);
    morse_send(msg);
    str++;
  }
}

/******************************************************************************
 * FunctionName : morse_send
 * Description  : send the morse message with the boards' led
 * Parameters   : message
 * Returns      : none
*******************************************************************************/

void morse_send(const char* msg){
  switch(*msg){
    case '.':
      GPIO_OUTPUT_SET(2,0);
      vTaskDelay(250/portTICK_RATE_MS);
      GPIO_OUTPUT_SET(2,1);
      vTaskDelay(250/portTICK_RATE_MS);
      break;
    case '_':
      GPIO_OUTPUT_SET(2,0);
      vTaskDelay(750/portTICK_RATE_MS);
      GPIO_OUTPUT_SET(2,1);
      vTaskDelay(250/portTICK_RATE_MS);
      break;
    case ' ':
      GPIO_OUTPUT_SET(2,1);
      vTaskDelay(500/portTICK_RATE_MS);
      break;
    case '\0':
      GPIO_OUTPUT_SET(2,1);
      return;
  }
  morse_send(++msg);
}
/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    xTaskCreate(&task_blink, "startup", 2048, NULL, 1, NULL);
}
