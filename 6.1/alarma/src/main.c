#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

#define PERIOD_TICK 100/portTICK_RATE_MS
#define REBOUND_TICK 50/portTICK_RATE_MS
#define timer 60000/portTICK_RATE_MS

#define ETS_GPIO_INTR_ENABLE() \
_xt_isr_unmask(1 << ETS_GPIO_INUM)
 #define ETS_GPIO_INTR_DISABLE() \
_xt_isr_mask(1 << ETS_GPIO_INUM)


int armed (fsm_t* );
void led_on (fsm_t* );
void led_off (fsm_t*);
int presence(fsm_t*);
int disarm(fsm_t*);
//void isr_gpio(void* );


enum fsm_state{ //inicializa el número y los nombres de los estados
  DISARMED,
  ARMED,
};

static fsm_trans_t interruptor[] = { //inicaliza los estados, input, siguiente estado, output
 { DISARMED, armed, ARMED, NULL},
 { ARMED, presence, ARMED, led_on},
 { ARMED, disarm, DISARMED, led_off},
 {-1, NULL, -1, NULL },
};

void isr_gpio(void* arg) {
 static portTickType xLastISRTick0 = 0;
 uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
 portTickType now = xTaskGetTickCount ();
 if (status & (BIT(0)||BIT(15))) {
   if (now > xLastISRTick0) {
     xLastISRTick0 = now + REBOUND_TICK;
     int done0 = 1;
    }
  }
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);
}
void inter2(void* ignore){
  GPIO_ConfigTypeDef io_conf, i15_conf;
  io_conf.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
  io_conf.GPIO_Mode = GPIO_Mode_Input;
  io_conf.GPIO_Pin = BIT(0);
  io_conf.GPIO_Pullup = GPIO_PullUp_EN;
  gpio_config(&io_conf);
  i15_conf.GPIO_IntrType = GPIO_PIN_INTR_POSEDGE;
  i15_conf.GPIO_Mode = GPIO_Mode_Input;
  i15_conf.GPIO_Pin = BIT(15);
  i15_conf.GPIO_Pullup = GPIO_PullUp_EN;
  gpio_config(&i15_conf);

 fsm_t* fsm = fsm_new(interruptor); //estados
 led_off(fsm); //inicial
 portTickType xLastWakeTime; //inicializa
 //Interrupt configuration before while(1)
 gpio_intr_handler_register((void*)isr_gpio, NULL);
 gpio_pin_intr_state_set(0, GPIO_PIN_INTR_NEGEDGE);
 gpio_pin_intr_state_set(15, GPIO_PIN_INTR_POSEDGE);
 // Other val: DISABLE, ANYEDGE, LOLEVEL, HILEVEL
 ETS_GPIO_INTR_ENABLE();
  while(true) {
      xLastWakeTime = xTaskGetTickCount();
      fsm_fire(fsm); //lanza el siguiente estado y las salidas si las tuviese
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK); //espera un tiempo para ahorrar
    }
  }

int armed (fsm_t *this) {
  GPIO_OUTPUT_SET(10,1); //armo aqui la alarma pero podría hacerlo mediante una entrada externa
  return 1;
}
void led_on (fsm_t *this) {
   GPIO_OUTPUT_SET(2, 0);
}

int disarm(fsm_t *this){
  if(GPIO_INPUT_GET(15)){
    GPIO_OUTPUT_SET(10,0);
    return 1;
  }
  else{
    return 0;
  }
}
void led_off (fsm_t *this) {
   GPIO_OUTPUT_SET(2, 1);
}

int presence(fsm_t *this){
  return !GPIO_INPUT_GET(0);
}
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
