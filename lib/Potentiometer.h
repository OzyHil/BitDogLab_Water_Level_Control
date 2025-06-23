#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include "General.h"

#define POTENTIOMETER_PIN 28 // Pino do potenciômetro
#define MAX_ADC_VALUE 4090 // Valor máximo do ADC (12 bits)
#define MIN_ADC_VALUE 3860 // Valor mínimo do potenciômetro

// Função para configurar o potenciômetro
void configure_potentiometer();

// Função para ler o valor do potenciômetro
uint read_potentiometer();

// Função para mapear o valor lido do potenciômetro para um intervalo específico
uint map_reading(uint value, uint in_min, uint in_max, uint out_min, uint out_max);

#endif