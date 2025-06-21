#ifndef GENERAL_H
#define GENERAL_H

// Inclusão das bibliotecas padrão e específicas do hardware
#include <stdio.h> // Biblioteca padrão para entrada/saída
#include <stdlib.h> // Biblioteca padrão para alocação de memória e conversões
#include <stdint.h> // Biblioteca padrão para tipos inteiros
#include <string.h> // Biblioteca manipular strings

#include <math.h> // Biblioteca para funções matemáticas

#include "pico/stdlib.h"     // Biblioteca principal para o Raspberry Pi Pico

#include "hardware/gpio.h" // Controle de GPIO (General Purpose Input/Output)
#include "hardware/pwm.h"    // Controle de PWM (Pulse Width Modulation)
#include "hardware/pio.h"    // Programação de E/S PIO (Programmable I/O)
#include "hardware/clocks.h" // Controle de clocks
#include "hardware/i2c.h"    // Comunicação I2C
#include "hardware/adc.h"    // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC

#include "pio_matrix.pio.h"  // Programa específico para controle da matriz de LEDs

#include "pico/cyw43_arch.h" // Biblioteca para arquitetura Wi-Fi da Pico com CYW43
#include "lwip/pbuf.h"  // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"   // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h" // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


// Definição de constantes e macros
#define MAX_WATER_CAPACITY 5000         // Capacidade máxima do reservatório de água em mililitros
#define MIN_WATER_LEVEL 500            // Nível mínimo de água em mililitros
#define MAX_WATER_LEVEL 4000           // Nível máximo de água em mililitros


#define WATER_PUMP_PIN 16              // Pino da bomba de água

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

#endif