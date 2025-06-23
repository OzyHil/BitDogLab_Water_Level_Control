#include "Potentiometer.h" // Inclusão do cabeçalho com definições do potenciômetro

// Função para configurar o potenciômetro utilizando ADC
void configure_potentiometer()
{
    // Inicializa o ADC no pino do potenciômetro (POTENTIOMETER_A) com valor de "wrap" especificado
    adc_gpio_init(POTENTIOMETER_PIN);
}

// Função para ler a posição do potenciômetro e retornar o valor
uint read_potentiometer() {
    uint sum = 0.0f;
    adc_select_input(2);

    for (int i = 0; i < 100; i++)
    {
        sum += adc_read();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    uint average = sum / 100.0f;
    
    printf("adc: %d\n", average);

    return average;
}

// Função para mapear o valor lido do potenciômetro para um intervalo específico
uint map_reading(uint value, uint in_min, uint in_max, uint out_min, uint out_max) {
    // Verifica se o valor está dentro do intervalo de entrada
    if (value < in_min || value > in_max) {
        return -1; // Retorna -1 se o valor estiver fora do intervalo
    }
    
    // Mapeia o valor do intervalo de entrada para o intervalo de saída
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}