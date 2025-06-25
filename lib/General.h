#ifndef GENERAL_H
#define GENERAL_H

// Inclusão das bibliotecas padrão e específicas do hardware
#include <stdio.h>  // Biblioteca padrão para entrada/saída
#include <stdlib.h> // Biblioteca padrão para alocação de memória e conversões
#include <stdint.h> // Biblioteca padrão para tipos inteiros
#include <string.h> // Biblioteca manipular strings

#include <math.h> // Biblioteca para funções matemáticas

#include "pico/stdlib.h" // Biblioteca principal para o Raspberry Pi Pico

#include "hardware/gpio.h"   // Controle de GPIO (General Purpose Input/Output)
#include "hardware/pwm.h"    // Controle de PWM (Pulse Width Modulation)
#include "hardware/pio.h"    // Programação de E/S PIO (Programmable I/O)
#include "hardware/clocks.h" // Controle de clocks
#include "hardware/i2c.h"    // Comunicação I2C
#include "hardware/adc.h"    // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC

#include "pio_matrix.pio.h" // Programa específico para controle da matriz de LEDs

#include "pico/cyw43_arch.h" // Biblioteca para arquitetura Wi-Fi da Pico com CYW43
#include "lwip/pbuf.h"       // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"        // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h"      // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Definição de constantes e macros
#define MAX_WATER_CAPACITY 1000 // Capacidade máxima do reservatório de água em mililitros
#define MIN_WATER_LEVEL 150     // Nível mínimo de água em mililitros (experimental - ajuste de acordo com a necessidade)
#define MAX_WATER_LEVEL 900    // Nível máximo de água em mililitros (experimental - ajuste de acordo com a necessidade)
#define MIN_ADC_VALUE 2133.0f      // Valor ADC em 0mL (experimental - ajuste de acordo com a necessidade)
#define MAX_ADC_VALUE 2512.0f      // Valor ADC em 1000mL (experimental - ajuste de acordo com a necessidade)
#define DEAD_ZONE 5


#define WATER_PUMP_PIN 16 // Pino da bomba de água

#define MAX_READINGS 20 // Pino da bomba de água

extern uint water_level;                  // Nível de água atual
extern uint water_level_max_limit;        // Limite máximo de nível de água
extern uint water_level_min_limit;        // Limite mínimo de nível de água
extern bool pump_status;                   // Status da bomba de água (ligada/desligada)
extern uint historic_levels[MAX_READINGS]; // Histórico de níveis de água

extern SemaphoreHandle_t xWaterPumpMutex, xWaterLevelMutex, xWaterLimitsMutex;

// Enum para definir os estados do sistema
typedef enum
{
    SYSTEM_FILLING,
    SYSTEM_DRAINING
} system_state_t;

// Função para inicializar a configuração do sistema (clocks, I/O, etc.)
void init_system_config();

// Função para inicializar o PWM em um pino específico com um valor de wrap
void init_pwm(uint gpio, uint wrap);

void add_reading(uint new_value, uint readings[]);

#endif