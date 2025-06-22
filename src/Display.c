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

void display_message(char *message)
{
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, message, 5, 5);
    ssd1306_send_data(&ssd);
}

void display_network_info(char *ip, bool connection_status)
{
    ssd1306_fill(&ssd, false); // Limpa o display

    ssd1306_rect(&ssd, 0, 0, 128, 64, true, false); // Desenha a borda retângular

    //============= (Seção 1) =============//
    ssd1306_draw_string(&ssd, "Server IP", 30, 7);
    ssd1306_draw_string(&ssd, ip, 12, 20);
    
    ssd1306_hline(&ssd, 1, 127, 33, true); // Linha vertical que divide as seções 1 e 2
    
    //============= (Seção 2) =============//
    ssd1306_draw_string(&ssd, "Wi-Fi Status", 17, 39);
    ssd1306_draw_string(&ssd, connection_status == true ? "Connected" : "Disconnected" , 17, 51);

    ssd1306_send_data(&ssd); // Atualiza o display
}

void display_water_system_info(float currente_water_level, float max_limit, float min_limit, system_state_t current_system_state)
{
    char current_level_str[10];
    char max_limit_str[10];
    char min_limit_str[10];

    snprintf(current_level_str, sizeof(current_level_str), "%.fmL", currente_water_level);
    snprintf(max_limit_str, sizeof(max_limit_str), "%.fmL", max_limit);
    snprintf(min_limit_str, sizeof(min_limit_str), "%.fmL", min_limit);

    ssd1306_fill(&ssd, false); // Limpa o display

    // ssd1306_rect(&ssd, 0, 0, 128, 64, true, false); // Desenha a borda retângular

    //============= (Seção 1) =============//
    ssd1306_draw_string(&ssd, "Level", 10, 7);
    ssd1306_draw_string(&ssd, "Max", 10, 22);
    ssd1306_draw_string(&ssd, "Min", 10, 37);
    ssd1306_draw_string(&ssd, "Pump", 10, 51);

    ssd1306_vline(&ssd, 57, 1, 63, true); // Linha vertical que divide as seções 1 e 2

    //============= (Seção 2) =============//
    ssd1306_draw_string(&ssd, current_level_str, 68, 7);
    ssd1306_draw_string(&ssd, max_limit_str, 68, 22);
    ssd1306_draw_string(&ssd, min_limit_str, 68, 37);
    ssd1306_draw_string(&ssd, current_system_state == SYSTEM_DRAINING ? "Off" : "On", 68, 51);

    ssd1306_send_data(&ssd); // Atualiza o display
}
