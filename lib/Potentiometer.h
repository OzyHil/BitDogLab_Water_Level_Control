#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include "General.h"

#define POTENTIOMETER_PIN 28 // Pino do potenciômetro

// Função para configurar o potenciômetro
void configure_potentiometer();

// Função para ler o valor do potenciômetro
uint read_potentiometer();

// Função para mapear o valor lido do potenciômetro para um intervalo específico
uint map_reading(uint value, uint in_min, uint in_max, uint out_min, uint out_max);

#endif