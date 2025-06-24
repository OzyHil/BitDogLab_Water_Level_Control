#include "General.h"
#include "Led.h"
#include "Buzzer.h"
#include "Button.h"
#include "Display.h"
#include "Led_Matrix.h"
#include "Webserver.h"
#include "Potentiometer.h"
#include "pico/bootrom.h"

system_state_t g_current_system_state = SYSTEM_DRAINING; // Variável global para armazenar o estado atual do sistema
uint water_level = 0;                                    // Nível de água atual
uint water_level_max_limit = MAX_WATER_LEVEL;            // Limite máximo de nível de água
uint water_level_min_limit = MIN_WATER_LEVEL;            // Limite mínimo de nível de água
bool pump_status = false;                                // Status da bomba de água (ligada/desligada)
uint historic_levels[MAX_READINGS] = {0};

// Semáforos para controle de acesso e sincronização
SemaphoreHandle_t DisplayModeSemaphore,
    xResetThresholds,
    xWaterLevelMutex,
    xWaterLimitsMutex,
    xStateMutex,
    xWaterPumpMutex;

// Variáveis para controle de tempo de debounce dos botões
uint last_time_button_A, last_time_button_B, last_time_button_J = 0;

// Função para sinalizar uma tarefa a partir de uma ISR
void signal_task_from_isr(SemaphoreHandle_t xSemaphore)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;                // Variável para verificar se uma tarefa de maior prioridade foi despertada
    xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken); // Libera o semáforo e verifica se uma tarefa de maior prioridade foi despertada
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                 // Se uma tarefa de maior prioridade foi despertada, realiza um yield para que ela possa ser executada imediatamente
}

// Função de tratamento de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t now = us_to_ms(get_absolute_time());

    if (gpio == BUTTON_A && (now - last_time_button_A > DEBOUNCE_TIME))
    {
        last_time_button_A = now;
        signal_task_from_isr(DisplayModeSemaphore);
    }
    if (gpio == BUTTON_B && (now - last_time_button_B > DEBOUNCE_TIME))
    {
        last_time_button_B = now;
        reset_usb_boot(0, 0);
    }
    else if (gpio == BUTTON_J && (now - last_time_button_J > DEBOUNCE_TIME))
    {
        last_time_button_J = now;
        signal_task_from_isr(xResetThresholds);
    }
}

void vTaskDisplay()
{
    display_mode_t current_display_mode = DISPLAY_WATER_SYSTEM; // Modo de exibição atual do display
    system_state_t current_state_copy;
    uint currente_water_level = 0;
    uint max_limit = 0;
    uint min_limit = 0;

    while (1)
    {
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
        {
            current_state_copy = g_current_system_state; // Atualiza a cópia do estado atual
            xSemaphoreGive(xStateMutex);                 // Libera o mutex do estado
        }

        if (xSemaphoreTake(xWaterLevelMutex, portMAX_DELAY) == pdTRUE)
        {
            currente_water_level = water_level;
            xSemaphoreGive(xWaterLevelMutex);
        }

        if (xSemaphoreTake(xWaterLimitsMutex, portMAX_DELAY) == pdTRUE)
        {
            max_limit = water_level_max_limit;
            min_limit = water_level_min_limit;
            xSemaphoreGive(xWaterLimitsMutex);
        }

        if (xSemaphoreTake(DisplayModeSemaphore, 0) == pdTRUE) // Espera pelo sinal do botão A
        {
            current_display_mode = (current_display_mode == DISPLAY_WATER_SYSTEM)
                                       ? DISPLAY_NETWORK_STATUS
                                       : DISPLAY_WATER_SYSTEM;
        }

        switch (current_display_mode) // Verifica o modo de exibição atual
        {
        case DISPLAY_WATER_SYSTEM:
            display_water_system_info(currente_water_level, max_limit, min_limit, current_state_copy);
            break;
        case DISPLAY_NETWORK_STATUS:
            display_network_info(ipaddr_ntoa(&netif_default->ip_addr), cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP);
            break;
        default:
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(400)); // Aguarda 500 ms antes de repetir
    }
}

void vTaskResetThresholds()
{
    while (1)
    {
        if (xSemaphoreTake(xResetThresholds, portMAX_DELAY) == pdTRUE) // Espera pelo sinal do botão J
        {
            if (xSemaphoreTake(xWaterLimitsMutex, portMAX_DELAY) == pdTRUE) // Espera pelo sinal do botão J
            {
                water_level_max_limit = MAX_WATER_LEVEL;
                water_level_min_limit = MIN_WATER_LEVEL;
                xSemaphoreGive(xWaterLimitsMutex);
            }
        }
    }
}

void vTaskLedMatrix()
{
    uint new_water_level = 0; // Novo nível de água a ser definido

    while (1)
    {
        if (xSemaphoreTake(xWaterLevelMutex, portMAX_DELAY) == pdTRUE)
        {
            new_water_level = water_level;    // Obtém o nível de água atual
            xSemaphoreGive(xWaterLevelMutex); // Libera o mutex da matriz de LEDs

            update_matrix_from_level(new_water_level, MAX_WATER_CAPACITY); // Chama a função da tarefa da matriz de LEDs
        }
        vTaskDelay(pdMS_TO_TICKS(225)); // Aguarda 225 ms antes de repetir
    }
}

void vTaskBuzzer()
{
    system_state_t current_state_copy;
    uint currente_water_level = 0;
    uint max_limit = 0;

    while (1)
    {
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
        {
            current_state_copy = g_current_system_state;
            xSemaphoreGive(xStateMutex);
        }

        if (xSemaphoreTake(xWaterLevelMutex, portMAX_DELAY) == pdTRUE)
        {
            currente_water_level = water_level;
            xSemaphoreGive(xWaterLevelMutex);
        }

        if (xSemaphoreTake(xWaterLimitsMutex, portMAX_DELAY) == pdTRUE)
        {
            max_limit = water_level_max_limit;
            xSemaphoreGive(xWaterLimitsMutex);
        }

        if (current_state_copy == SYSTEM_FILLING) // Verifica se o sistema está enchendo
        {
            double_beep(); // Emite um sinal sonoro de dois
        }
        else if (currente_water_level > max_limit) // Verifica se o sistema está drenando
        {
            single_beep();
        }

        vTaskDelay(pdMS_TO_TICKS(110));
    }
}

void vTaskControlSystem()
{
    system_state_t current_state_copy = SYSTEM_DRAINING; // Cópia do estado atual do sistema
    uint new_water_level = 0;                            // Novo nível de água a ser definido
    float adc_value = 0;                                  // Valor lido do potenciômetro

    while (1)
    {
        /* -------------- LEITURA DA BOIA ---------------  */
        adc_value = read_potentiometer();
        new_water_level = map_reading(adc_value, MIN_ADC_VALUE, MAX_ADC_VALUE, 0, MAX_WATER_CAPACITY);        // Convertendo valor ADC para nível de água;

        /* -------------- ATUALIZAÇÃO DO NÍVEL DA ÁGUA ---------------  */
        if (xSemaphoreTake(xWaterLevelMutex, portMAX_DELAY) == pdTRUE)
        {
            water_level = new_water_level;
            add_reading(currente_water_level, historic_levels); // Adiciona o nível de água atual ao histórico
            xSemaphoreGive(xWaterLevelMutex);
            printf("INFO: Water level: %d \n", new_water_level);
        }

        /* -------------- ATUALIZAÇÃO DO ESTADO DO SISTEMA ---------------  */
        if (xSemaphoreTake(xWaterLimitsMutex, portMAX_DELAY) == pdTRUE)
        {
            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
            {
                if (new_water_level < water_level_min_limit)
                {
                    g_current_system_state = SYSTEM_FILLING;     // Muda o estado do sistema para enchimento
                    current_state_copy = g_current_system_state; // Atualiza a cópia do estado atual
                    printf("INFO: System filling\n");
                }
                else if (new_water_level > water_level_max_limit)
                {
                    g_current_system_state = SYSTEM_DRAINING;    // Muda o estado do sistema para enchimento
                    current_state_copy = g_current_system_state; // Atualiza a cópia do estado atual
                    printf("INFO: System draining\n");
                }
                xSemaphoreGive(xStateMutex); // Libera o mutex do estado
            }
            xSemaphoreGive(xWaterLimitsMutex);
        }

        switch (current_state_copy)
        {
        case SYSTEM_DRAINING:     // Estado de drenagem
            set_led_color(GREEN); // Liga o LED verde

            if (xSemaphoreTake(xWaterPumpMutex, portMAX_DELAY) == pdTRUE)
            {
                pump_status = false;
                xSemaphoreGive(xWaterPumpMutex);
            }

            gpio_put(WATER_PUMP_PIN, 0); // Desliga a bomba de água
            break;
        case SYSTEM_FILLING:       // Estado de enchimento
            set_led_color(ORANGE); // Liga o LED laranja

            if (xSemaphoreTake(xWaterPumpMutex, portMAX_DELAY) == pdTRUE)
            {
                pump_status = true;
                xSemaphoreGive(xWaterPumpMutex);
            }

            gpio_put(WATER_PUMP_PIN, 1); // Liga a bomba de água
            break;
        default: // Estado desconhecido
            printf("WARNING: Unknow state detected! Resetting system to draining state...\n");
            set_led_color(DARK); // Desliga o LED

            if (xSemaphoreTake(xWaterPumpMutex, portMAX_DELAY) == pdTRUE)
            {
                pump_status = false;
                xSemaphoreGive(xWaterPumpMutex);
            }
            
            gpio_put(WATER_PUMP_PIN, 0); // Desliga a bomba de água
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200)); // Aguarda 500 ms antes de repetir
    }
}

void vTaskTCPServer()
{
    run_tcp_server_loop(); // Inicia o loop do servidor TCP
    deinit_cyw43();        // Finaliza a arquitetura CYW43
}

int main()
{
    init_system_config(); // Função para inicializar a configuração do sistema

    configure_leds();       // Configura os LEDs
    configure_buzzer();     // Configura o buzzer
    configure_display();    // Configura o display OLED
    configure_led_matrix(); // Configura a matriz de LEDs

    configure_button(BUTTON_A); // Configura os botões
    configure_button(BUTTON_B);
    configure_button(BUTTON_J);

    gpio_init(WATER_PUMP_PIN);              // Inicializa o pino da bomba de água
    gpio_set_dir(WATER_PUMP_PIN, GPIO_OUT); // Configura o pino da bomba de água como saída
    gpio_put(WATER_PUMP_PIN, 0);            // Desliga a bomba de água inicialmente

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_J, GPIO_IRQ_EDGE_FALL, true);

    DisplayModeSemaphore = xSemaphoreCreateBinary(); // Semáforo binário para o botão J
    xResetThresholds = xSemaphoreCreateBinary();     // Semáforo binário para o botão J

    // Mutexs para controle de acesso e sincronização
    xWaterPumpMutex = xSemaphoreCreateMutex();
    xWaterLevelMutex = xSemaphoreCreateMutex();
    xWaterLimitsMutex = xSemaphoreCreateMutex();
    xStateMutex = xSemaphoreCreateMutex();

    display_message("Iniciando...");            // Exibe a mensagem inicial no display
    init_cyw43();                               // Inicializa a arquitetura CYW43
    display_message("Conectando a rede...");    // Exibe a mensagem inicial no display
    connect_to_wifi();                          // Conecta ao Wi-Fi
    display_message("Iniciando servidor...");   // Exibe a mensagem inicial no display
    struct tcp_pcb *server = init_tcp_server(); // Inicializa o servidor TCP

    // Criação das tarefas
    xTaskCreate(vTaskDisplay, "Task Display", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskLedMatrix, "Task LED Matrix", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskControlSystem, "Task Control System", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskResetThresholds, "Task Reset Thresholds", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskBuzzer, "Task Buzzer", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskTCPServer, "Task TCP Server", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    vTaskStartScheduler(); // Inicia o escalonador de tarefas

    panic_unsupported(); // Se o escalonador falhar, entra em pânico
}
