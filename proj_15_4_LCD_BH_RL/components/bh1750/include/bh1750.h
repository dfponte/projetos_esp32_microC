#ifndef BH1750_H
#define BH1750_H

/**
 * @brief Inicializa o barramento I2C e configura o sensor BH1750.
 */
void bh1750_init(int sda_pin, int scl_pin);

/**
 * @brief Realiza a leitura de luminosidade do sensor.
 * @return Valor em LUX, ou -1 em caso de falha de comunicação.
 */
int bh1750_read_lux(void);

#endif // BH1750_H
