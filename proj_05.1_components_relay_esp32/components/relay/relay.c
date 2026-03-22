#include <stdio.h>
#include "relay.h"
#include "driver/gpio.h"

//Funções de inicialização do relé



void relay_init(Relay *relay,int pin){
   relay->pin = pin;
   relay->state = false;
   gpio_set_direction(relay->pin,GPIO_MODE_OUTPUT);
   gpio_set_level(relay->pin,relay->state);
   
}

void relay_set_on(Relay *relay){
  relay->state = true;
  gpio_set_level(relay->pin,relay->state);
}

void relay_set_off(Relay *relay){
   relay->state = false;
   gpio_set_level(relay->pin,relay->state);
}

int relay_get_status(Relay *relay){
  return relay->state;
}