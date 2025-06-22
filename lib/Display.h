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

void display_message(char *message); // Função para desenhar informações no display OLED

void display_network_info(char *ip, bool connection_status); // Função para exibir informações de rede no display OLED

void display_water_system_info(float currente_water_level, float max_limit, float min_limit, system_state_t current_system_state); // Função para exibir informações do sistema de água no display OLED

#endif