#ifndef RELAY_H
#define RELAY_H

typedef struct{
    int pin;           //Pino associado ao relé
    int state;         //Estado do relé

}Relay;

//Protótipos da função de inicialização do relé

void relay_init(Relay *relay,int pin);

void relay_set_on(Relay *relay);

void relay_set_off(Relay *relay);

int relay_get_status(Relay *relay);


#endif
