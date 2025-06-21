#include "Display.h" // Inclusão do cabeçalho com definições relacionadas ao display OLED

ssd1306_t ssd; // Estrutura que representa o display OLED SSD1306

// Função para configurar a comunicação I2C e inicializar o display OLED
void configure_display()
{
    // Inicializa o barramento I2C na porta e com frequência de 400 kHz
    i2c_init(I2C_PORT, 400 * 1000);

    // Define as funções dos pinos SDA e SCL como I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    // Ativa resistores de pull-up nos pinos SDA e SCL
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display com resolução 128x64, sem rotação, endereço I2C e porta definida
    ssd1306_init(&ssd, 128, 64, false, DISPLAY_ADDRESS, I2C_PORT);

    // Configura o display com parâmetros padrão
    ssd1306_config(&ssd);

    clear_display(); // Limpa o display OLED para iniciar com uma tela limpa
}

// Função para limpar o display OLED
void clear_display()
{
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// Função para desenhar informações no display OLED
void draw_info(char *string)
{
    // Exibe no display
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, string, 5, 5);
    ssd1306_send_data(&ssd);
}