#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

#define PERIOD_TICK 100/portTICK_RATE_MS
#define REBOUND_TICK 200/portTICK_RATE_MS

#define ETS_GPIO_INTR_ENABLE() \
_xt_isr_unmask(1 << ETS_GPIO_INUM)
 #define ETS_GPIO_INTR_DISABLE() \
_xt_isr_mask(1 << ETS_GPIO_INUM)


int armed (fsm_t* );
void led_on (fsm_t* );
void led_off (fsm_t*);
void clean_flags(fsm_t* );
int presence(fsm_t*);
int disarm1(fsm_t*);
int disarm2(fsm_t*);
int disarm3(fsm_t*);
int timeout(fsm_t*);
//void isr_gpio(void* );
volatile int done0 = 0, done1 = 0, tim = 0;

enum fsm_state{ //inicializa el número y los nombres de los estados
  DISARMED,
  ARMED,
  T1,
  T2,
};

static fsm_trans_t interruptor[] = { //inicaliza los estados, input, siguiente estado, output
 { DISARMED, armed, ARMED, NULL},
 { ARMED, presence, ARMED, led_on},
 { ARMED, disarm1, T1, clean_flags},
 { T1, disarm2, T2, clean_flags},
 { T1, timeout, ARMED, clean_flags},
 { T2, disarm3, DISARMED, led_off},
 { T2, timeout, ARMED, clean_flags},
 {-1, NULL, -1, NULL },
};

//RUTINA DE ATENCIÓN

void isr_gpio(void* arg) {
 static portTickType xLastISRTick0 = 0, xLastISRTick15 = 0;
 uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
 portTickType now = xTaskGetTickCount ();
 if (status & BIT(0)) { //un unico & se hace la comparación bit a bit (&& es el total)
   if (now > xLastISRTick0) {
     xLastISRTick0 = now + REBOUND_TICK;
     done0 = 1;
    }
  }
  if (status & BIT(15)) { //un unico & se hace la comparación bit a bit (&& es el total)
    if (now > xLastISRTick15) {
      xLastISRTick15 = now + REBOUND_TICK;
      done1 = 1;
     }
   }
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);
}

//PSEUDOMAIN

void inter2(void* ignore){
 fsm_t* fsm = fsm_new(interruptor); //estados
 led_off(fsm); //inicial
 portTickType xLastWakeTime; //inicializa
 //Interrupt configuration before while(1)

 PIN_FUNC_SELECT(GPIO_PIN_REG_15, FUNC_GPIO15); //necesario para usar el pin 15

 gpio_intr_handler_register((void*)isr_gpio, NULL); //registo interrupcion
 gpio_pin_intr_state_set(0, GPIO_PIN_INTR_NEGEDGE); //causas por las que quiero que salte la interrupcion
 gpio_pin_intr_state_set(15, GPIO_PIN_INTR_POSEDGE);
 // Other val: DISABLE, ANYEDGE, LOLEVEL, HILEVEL
 ETS_GPIO_INTR_ENABLE(); //la habilito las interrupciones
  while(true) {
      xLastWakeTime = xTaskGetTickCount();
      fsm_fire(fsm); //lanza el siguiente estado y las salidas si las tuviese
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK); //espera un tiempo para ahorrar
    }
  }

int armed (fsm_t *fsm) {
  tim=0;
  done0=0;
  done1=0;
  GPIO_OUTPUT_SET(10,1); //armo aqui la alarma pero podría hacerlo mediante una entrada externa
  return 1;
}
void led_on (fsm_t *fsm) {
  tim=0;
  done0=0;
  done1=0;
  GPIO_OUTPUT_SET(2, 0);
}

void clean_flags(fsm_t *fsm){
  tim=0;
  done0=0;
  done1=0;
}

int disarm1(fsm_t *fsm){
  return done1;
}

int disarm2(fsm_t *fsm){
  return done1;
}

int disarm3(fsm_t *fsm){
  return done1;
}

int timeout(fsm_t *fsm){
  tim++;
  if(tim > 10){
    return 1;
  }
  else{
    return 0;
  }

}
void led_off (fsm_t *fsm) {
  tim=0;
  done0=0;
  done1=0;
   GPIO_OUTPUT_SET(2, 1);
}

int presence(fsm_t *fsm){
  return done0;

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
