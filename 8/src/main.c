#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"
//////////////////////////////////////////////////////////////////////////////
#define PERIOD_TICK 100/portTICK_RATE_MS
#define REBOUND_TICK 200/portTICK_RATE_MS
#define TIMER 1000/portTICK_RATE_MS
#define ETS_GPIO_INTR_ENABLE() \
_xt_isr_unmask(1 << ETS_GPIO_INUM)
 #define ETS_GPIO_INTR_DISABLE() \
_xt_isr_mask(1 << ETS_GPIO_INUM)

//////////////////////////////////////////////////////////////////////////////
int codigo_correcto(fsm_t*);
int codigo_incorrecto(fsm_t*);
int mirar_flag_valido(fsm_t*);
int timeout_valid(fsm_t*);
int presencia(fsm_t*);
void limpiar_flag(fsm_t*);
void aumenta_digito(fsm_t*);
void siguiente_digito(fsm_t*);
void led_off(fsm_t*);
void led_on(fsm_t*);
volatile int pressed = 0, presence = 0;
volatile int in = 0;
int code_inserted[] = {0, 0, 0};
int right_code[] = {1, 2, 3}; //codigo correcto
portTickType nextTimeout;
//////////////////////////////////////////////////////////////////////////////
enum fsm_state{ //inicializa el número y los nombres de los estados
  DISARMED,
  ARMED
};
//////////////////////////////////////////////////////////////////////////////
static fsm_trans_t interruptor[] = { //inicaliza los estados, input, siguiente estado, output
 {DISARMED, codigo_correcto, ARMED, limpiar_flag},
 {DISARMED, mirar_flag_valido, DISARMED, aumenta_digito},
 {DISARMED, timeout_valid, DISARMED, siguiente_digito},
 {DISARMED, codigo_incorrecto, DISARMED, limpiar_flag},
 {ARMED, codigo_correcto, DISARMED, led_off},
 {ARMED, mirar_flag_valido, ARMED, aumenta_digito},
 {ARMED, timeout_valid, ARMED, siguiente_digito},
 {ARMED, codigo_incorrecto, ARMED, limpiar_flag},
 {ARMED, presencia, ARMED, led_on},
 {-1, NULL, -1, NULL },
};
//////////////////////////////////////////////////////////////////////////////
//RUTINAS DE COMPROBACIÓN
int codigo_correcto(fsm_t* fsm){
    if(in>2){
      if((code_inserted[0]==right_code[0])||(code_inserted[1]==right_code[1])||(code_inserted[2]==right_code[2])){
        return 1;
      }
      else{return 0;}
    }
    else{
      return 0;
    }
  }
//////////////////////////////////////////////////////////////////////////////
int codigo_incorrecto(fsm_t* fsm){
  if(in>2){
    if((code_inserted[0]!=right_code[0])||(code_inserted[1]!=right_code[1])||(code_inserted[2]!=right_code[2])){
      return 1;
    }
    else{
      return 0;
    }
  }
  else{return 0;}
}
//////////////////////////////////////////////////////////////////////////////
int mirar_flag_valido(fsm_t* fsm){
  return pressed;

}
//////////////////////////////////////////////////////////////////////////////
int timeout_valid(fsm_t* fsm){
  portTickType now = xTaskGetTickCount();
  if(nextTimeout < now){
    return 1;
  }
  else{
    return 0;
  }
}
//////////////////////////////////////////////////////////////////////////////
int presencia(fsm_t* fsm){
    return presence;
}
//////////////////////////////////////////////////////////////////////////////
//ACCIONES
void limpiar_flag(fsm_t* fsm){
  int i;
  for(i=0;i<3;i++){
    code_inserted[i] = 0;
  }
  GPIO_OUTPUT_SET(2,1);
  in = 0;
  presence = 0;
}
//////////////////////////////////////////////////////////////////////////////
void aumenta_digito(fsm_t* fsm){
  code_inserted[in]++;
  pressed = 0;
  nextTimeout = xTaskGetTickCount() + TIMER;
  portTickType xLastWakeTime = xTaskGetTickCount();
  GPIO_OUTPUT_SET(2, 0);
  vTaskDelayUntil(&xLastWakeTime, 10);
  GPIO_OUTPUT_SET(2,1);
}
//////////////////////////////////////////////////////////////////////////////
void siguiente_digito(fsm_t* fsm){
  if(code_inserted[in]!= 0){
      GPIO_OUTPUT_SET(2,0);
      in++;
      nextTimeout = 0xFFFFFFFF;
  }
}
//////////////////////////////////////////////////////////////////////////////
void led_off(fsm_t* fsm){
  GPIO_OUTPUT_SET(2,1);
}
//////////////////////////////////////////////////////////////////////////////
void led_on(fsm_t* fsm){
  GPIO_OUTPUT_SET(2,0);
  presence=0;
}
//////////////////////////////////////////////////////////////////////////////
//RUTINA DE ATENCIÓN (interrupcion)
void isr_gpio(void* arg) {
  static portTickType xLastISRTick0 = 0;
  static portTickType xLastISRTick15 = 0;

   uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);          //READ STATUS OF INTERRUPT

   portTickType now = xTaskGetTickCount ();
   if (status & BIT(0)) {
     if (now > xLastISRTick0) {
       xLastISRTick0 = now + REBOUND_TICK;
       pressed = 1;
     }
   }
   if (status & BIT(15)) {
     if (now > xLastISRTick15) {
        xLastISRTick15 = now + REBOUND_TICK;
        presence = 1;
     }
   }


   //should not add print in interruption, except that we want to debug something
   //printf("in io intr: 0X%08x\r\n",status);                    //WRITE ON SERIAL UART0
   GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);       //CLEAR THE STATUS IN THE W1 INTERRUPT REGISTER
}
//////////////////////////////////////////////////////////////////////////////
//PSEUDOMAIN
void inter2(void* ignore){
 fsm_t* fsm = fsm_new(interruptor); //estados
 led_off(fsm); //inicial
 portTickType xLastWakeTime; //inicializa
 PIN_FUNC_SELECT(GPIO_PIN_REG_15, FUNC_GPIO15); //necesario para usar el pin 15
 //Interrupt configuration before while(1)
 gpio_intr_handler_register((void*)isr_gpio, NULL); //registo interrupcion
 gpio_pin_intr_state_set(0, GPIO_PIN_INTR_NEGEDGE); //causas por las que quiero que salte la interrupcion
 gpio_pin_intr_state_set(15, GPIO_PIN_INTR_POSEDGE); //causas por las que quiero que salte la interrupcion

 // Other val: DISABLE, ANYEDGE, LOLEVEL, HILEVEL
 ETS_GPIO_INTR_ENABLE(); //la habilito las interrupciones
  while(true) {
      xLastWakeTime = xTaskGetTickCount();
      fsm_fire(fsm); //lanza el siguiente estado y las salidas si las tuviese
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK); //espera un tiempo para ahorrar
    }
  }








//////////////////////////////////////////////////////////////////////////////
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
void user_init(void)
{
    xTaskCreate(&inter2, "startup", 2048, NULL, 1, NULL);
}
