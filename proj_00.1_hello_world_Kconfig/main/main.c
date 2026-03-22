#include <stdio.h>
#include "sdkconfig.h" // Inclui as macros geradas

#include "driver/gpio.h"

void app_main(void)
{
    printf("Iniciando o projeto ESP32...\n");

#ifdef CONFIG_OPCAO_BOOLEANA
    printf("Funcionalidade X esta ativada!\n");
#else
    printf("Funcionalidade X esta desativada.\n");
#endif

printf("O valor da funcionalidade Y eh: %d\n", CONFIG_VALOR_INTEIRO);
}