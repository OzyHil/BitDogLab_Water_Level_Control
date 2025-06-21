#ifndef DISPLAY_H
#define DISPLAY_H

// Inclusão das bibliotecas necessárias
#include "General.h"     // Definições gerais do sistema
#include "ssd1306.h"     // Controle do display OLED

// Definição de pinos e endereço do display I2C
#define I2C_PORT i2c1            // Porta I2C para comunicação
#define I2C_SDA 14               // Pino SDA para I2C
#define I2C_SCL 15               // Pino SCL para I2C
#define DISPLAY_ADDRESS 0x3C     // Endereço do display OLED

typedef enum {
    DISPLAY_WATER_SYSTEM,   // Show water level and pump status
    DISPLAY_NETWORK_STATUS  // Show network connection info
} display_mode_t;


void configure_display(); // Função para configurar o display

void clear_display(); // Função para limpar o display OLED

void draw_info(char *string); // Função para desenhar informações no display OLED

#endif