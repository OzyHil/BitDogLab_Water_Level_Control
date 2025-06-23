#include "Webserver.h" // Biblioteca geral do sistema

void init_cyw43()
{
    // Inicializa a arquitetura CYW43
    if (cyw43_arch_init())
    {
        display_message("Falha ao inicializar o Wi-Fi");
        sleep_ms(100);
    }

    // Coloca GPIO do módulo em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);
}

void connect_to_wifi()
{
    // Ativa o modo Station (cliente Wi-Fi)
    cyw43_arch_enable_sta_mode();

    // Tenta conectar ao Wi-Fi com timeout de 20 segundos
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        display_message("Falha ao conectar ao Wi-Fi");
        sleep_ms(100);
    }

    struct netif *netif = &cyw43_state.netif[0];
    while (netif_is_link_up(netif) == 0)
    {
        sleep_ms(100);
    }

}

struct tcp_pcb *init_tcp_server()
{
    // Cria o controle de protocolo TCP
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        ////////////// Falha ao criar o servidor TCP
        return NULL;
    }

    // Tenta vincular à porta 80
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        ///////////////// Falha ao vincular à porta
        return NULL;
    }

    // Coloca em modo de escuta e define a função de callback
    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);

    return server;
}

void run_tcp_server_loop()
{
    // Loop principal para manter o Wi-Fi ativo e escutar conexões
    while (true)
    {
        cyw43_arch_poll();
        vTaskDelay(100);
    }
}

void deinit_cyw43()
{
    // Finaliza a arquitetura CYW43
    cyw43_arch_deinit();
    vTaskDelete(NULL); // Deleta a tarefa atual
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Tratamento do request do usuário
void user_request(char **request_ptr, struct tcp_pcb *tpcb)
{
    char *request = *request_ptr;

    uint level;
    bool pump_on;
    uint min;
    uint max;
    uint history[MAX_READINGS];

    if (strstr(request, "GET /data"))
    {
        // Geração do JSON de resposta (manual)
        char json[512];

        if (xSemaphoreTake(xWaterLevelMutex, portMAX_DELAY) == pdTRUE)
        {
            level = water_level;
            memcpy(history, historic_levels, sizeof(uint) * MAX_READINGS);
            xSemaphoreGive(xWaterLevelMutex);
        }
        if (xSemaphoreTake(xWaterPumpMutex, portMAX_DELAY) == pdTRUE)
        {
            pump_on = pump_status;
            xSemaphoreGive(xWaterPumpMutex);
        }

        if (xSemaphoreTake(xWaterLimitsMutex, portMAX_DELAY) == pdTRUE)
        {
            min = water_level_min_limit;
            max = water_level_max_limit;
            xSemaphoreGive(xWaterLimitsMutex);
        }

        char json_history_levels[1024];
        char *ptr = json_history_levels;

        *ptr++ = '[';
        for (int i = 0; i < MAX_READINGS; i++)
        {
            ptr += sprintf(ptr, "%u%s", history[i], (i < MAX_READINGS - 1) ? "," : "");
        }
        *ptr++ = ']';
        *ptr = '\0';

        snprintf(json, sizeof(json),
                 "{\"nivel\":%d,\"bomba_ligada\":%s,\"min_config\":%d,\"max_config\":%d,\"historico\":%s}",
                 level,
                 pump_on ? "true" : "false",
                 min,
                 max,
                 json_history_levels);

        // Monta resposta HTTP
        char response[512];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Content-Length: %d\r\n"
                 "Connection: close\r\n\r\n"
                 "%s",
                 strlen(json), json);

        tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
    }
    else if (strstr(request, "POST /config"))
    {
        char *body = strstr(request, "\r\n\r\n");
        if (body)
        {
            body += 4; // pular os headers

            // Exemplo de corpo esperado: {"min":30,"max":80}
            int min = -1, max = -1;

            // Encontrar os números manualmente
            char *min_str = strstr(body, "\"min\"");
            char *max_str = strstr(body, "\"max\"");

            if (min_str)
                min = atoi(min_str + 6); // pula '"min":'
            if (max_str)
                max = atoi(max_str + 6); // pula '"max":'

            if (min >= 0 && max >= 0 && min < max && max <= MAX_WATER_CAPACITY)
            {
                if (xSemaphoreTake(xWaterLimitsMutex, portMAX_DELAY) == pdTRUE)
                {
                    water_level_min_limit = (float)min;
                    water_level_max_limit = (float)max;
                    xSemaphoreGive(xWaterLimitsMutex);
                }

                tcp_write(tpcb, "HTTP/1.1 200 OK\r\n\r\n", 19, TCP_WRITE_FLAG_COPY);
            }
            else
            {
                tcp_write(tpcb, "HTTP/1.1 400 Bad Request\r\n\r\n", 28, TCP_WRITE_FLAG_COPY);
            }

            tcp_output(tpcb);
        }
    }
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinâmica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    // Tratamento de request - Controle dos LEDs
    user_request(&request, tpcb);

    char html[7144];

    snprintf(html, sizeof(html),
             "<!DOCTYPE html>\r\n"
             "<html lang=\"pt-br\">\r\n"
             "<head>\r\n"
             "<meta charset=\"UTF-8\">\r\n"
             "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
             "<title>Controle de Nível de Água</title>\r\n"
             "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\r\n"
             "<style>\r\n"
             "body { font-family: 'Segoe UI', sans-serif; background: #eef1f5; color: #333; margin: 0; padding: 20px; }\r\n"
             "h1 { text-align: center; margin-bottom: 30px; }\r\n"
             ".card { background: #fff; padding: 20px; margin: 20px auto; border-radius: 10px; box-shadow: 0 2px 6px rgba(0, 0, 0, 0.1); max-width: 700px; text-align: center; }\r\n"
             ".card h2 { margin-bottom: 20px; color: #1e88e5; }\r\n"
             ".label, table td { font-weight: bold; margin-bottom: 5px; color: #555; }\r\n"
             ".valor { font-size: 1.4em; font-weight: bold; color: #000; }\r\n"
             "#estadoBomba.ligada { color: #4caf50; }\r\n"
             "#estadoBomba.desligada { color: #f44336; }\r\n"
             ".barra { width: 100%%; background: #ccc; border-radius: 12px; overflow: hidden; height: 25px; margin: 10px 0; border: 1px solid #aaa; }\r\n"
             ".preenchimento { height: 100%%; background: linear-gradient(90deg,rgb(235, 251, 255), #1e88e5); transition: width 0.4s ease; width: 0%%; }\r\n"
             "input[type='number'] { font-size: 16px; padding: 8px; width: 100px; text-align: center; border: 1px solid #ccc; border-radius: 4px; }\r\n"
             "button { font-size: 16px; padding: 12px 25px; border: none; border-radius: 6px; background: #1e88e5; color: white; box-shadow: 0 2px 5px rgba(0, 0, 0, 0.15); transition: background 0.3s; cursor: pointer; margin-top: 20px; }\r\n"
             "button:hover { background: #1565c0; }\r\n"
             "canvas { max-width: 100%%; height: auto; margin-top: 20px; }\r\n"
             ".tabela-limites { width: 100%%; border-collapse: collapse; margin-top: 10px; }\r\n"
             ".tabela-limites td { padding: 12px; border-bottom: 1px solid #ddd; text-align: center; font-size: 1.1em; }\r\n"
             "form>div { margin-bottom: 20px; }\r\n"
             "</style>\r\n"
             "</head>\r\n"
             "<body>\r\n"
             "<h1>Controle de Nível de Água</h1>\r\n"

             "<div class=\"card\">\r\n"
             "<h2>Dados Atuais</h2>\r\n"
             "<div style=\"display: flex; justify-content: center; gap: 40px; flex-wrap: wrap;\">\r\n"
             "  <p class=\"label\">Quantidade: <span id=\"nivelValor\" class=\"valor\">--</span>ml</p>\r\n"
             "  <p class=\"label\">Nível: <span id=\"nivelPorcentagem\" class=\"valor\">--</span>%%</p>\r\n"
             "</div>\r\n"
             "<div class=\"barra\"><div id=\"barraNivel\" class=\"preenchimento\"></div></div>\r\n"
             "<p class=\"label\">Estado da Bomba: <span id=\"estadoBomba\" class=\"valor desligada\">--</span></p>\r\n"
             "</div>\r\n"

             "<div class=\"card\">\r\n"
             "<h2>Histórico de Nível</h2>\r\n"
             "<canvas id=\"graficoNivel\" width=\"600\" height=\"300\"></canvas>\r\n"
             "</div>\r\n"

             "<div class=\"card\">\r\n"
             "<h2>Limites Configurados</h2>\r\n"
             "<table class=\"tabela-limites\">\r\n"
             "<tr><td>Capacidade Máxima</td><td><span class=\"valor\">%d</span> ml</td></tr>\r\n"
             "<tr><td>Mínimo Configurado</td><td><span id=\"minConfigurado\" class=\"valor\">--</span> ml</td></tr>\r\n"
             "<tr><td>Máximo Configurado</td><td><span id=\"maxConfigurado\" class=\"valor\">--</span> ml</td></tr>\r\n"
             "</table>\r\n"
             "</div>\r\n"

             "<div class=\"card\">\r\n"
             "<h2>Configurar Novos Limites</h2>\r\n"
             "<form id=\"configForm\" onsubmit=\"salvarConfiguracao(event)\">\r\n"
             "<div><label for=\"inputMin\" class=\"label\">Nível Mínimo (ml):</label><br>\r\n"
             "<input type=\"number\" id=\"inputMin\" required></div>\r\n"
             "<div><label for=\"inputMax\" class=\"label\">Nível Máximo (ml):</label><br>\r\n"
             "<input type=\"number\" id=\"inputMax\" required></div>\r\n"
             "<button type=\"submit\">Salvar Novos Limites</button>\r\n"
             "</form>\r\n"
             "</div>\r\n"

             "<script>\r\n"
             "let grafico = null;\r\n"
             "function inicializarGrafico() {\r\n"
             "  const ctx = document.getElementById('graficoNivel').getContext('2d');\r\n"
             "  grafico = new Chart(ctx, {\r\n"
             "    type: 'line',\r\n"
             "    data: {\r\n"
             "      labels: [],\r\n"
             "      datasets: [{\r\n"
             "        label: 'Nível (ml)',\r\n"
             "        data: [],\r\n"
             "        backgroundColor: 'rgba(33, 150, 243, 0.2)',\r\n"
             "        borderColor: 'rgba(33, 150, 243, 1)',\r\n"
             "        borderWidth: 2,\r\n"
             "        tension: 0.3,\r\n"
             "        fill: true\r\n"
             "      }]\r\n"
             "    },\r\n"
             "    options: {\r\n"
             "      responsive: true,\r\n"
             "      scales: {\r\n"
             "        y: {\r\n"
             "          beginAtZero: true,\r\n"
             "          suggestedMax: %d\r\n"
             "        }\r\n"
             "      }\r\n"
             "    }\r\n"
             "  });\r\n"
             "}\r\n"

             "function atualizarDados() {\r\n"
             "  fetch('/data')\r\n"
             "    .then(res => res.json())\r\n"
             "    .then(data => {\r\n"
             "      document.getElementById('nivelValor').innerText = data.nivel;\r\n"
             "      const porcentagem = (data.nivel * 100) / %d;\r\n"
             "      document.getElementById('barraNivel').style.width = porcentagem + '%%';\r\n"
             "      document.getElementById('nivelPorcentagem').innerText = porcentagem;\r\n"
             "      const estadoBombaEl = document.getElementById('estadoBomba');\r\n"
             "      estadoBombaEl.innerText = data.bomba_ligada ? 'Ligada' : 'Desligada';\r\n"
             "      estadoBombaEl.className = 'valor ' + (data.bomba_ligada ? 'ligada' : 'desligada');\r\n"
             "      document.getElementById('minConfigurado').innerText = data.min_config;\r\n"
             "      document.getElementById('maxConfigurado').innerText = data.max_config;\r\n"
             "      if (grafico && Array.isArray(data.historico)) {\r\n"
             "        grafico.data.labels = data.historico.map((_, i) => i + 1);\r\n"
             "        grafico.data.datasets[0].data = data.historico;\r\n"
             "        grafico.update();\r\n"
             "      }\r\n"
             "    });\r\n"
             "}\r\n"

             "function salvarConfiguracao(event) {\r\n"
             "  event.preventDefault();\r\n"
             "  const min = document.getElementById('inputMin').value;\r\n"
             "  const max = document.getElementById('inputMax').value;\r\n"
             "  fetch('/config', {\r\n"
             "    method: 'POST',\r\n"
             "    headers: { 'Content-Type': 'application/json' },\r\n"
             "    body: JSON.stringify({ min: Number(min), max: Number(max) })\r\n"
             "  })\r\n"
             "  .then(response => {\r\n"
             "    if (response.ok) {\r\n"
             "      alert('Limites salvos com sucesso!');\r\n"
             "      atualizarDados();\r\n"
             "    } else {\r\n"
             "      alert('Erro ao salvar os limites.');\r\n"
             "    }\r\n"
             "  })\r\n"
             "  .catch(error => console.error('Erro ao salvar configuração:', error));\r\n"
             "}\r\n"

             "  inicializarGrafico();\r\n"
             "  atualizarDados();\r\n"
             "  setInterval(atualizarDados, 3000);\r\n"
             "</script>\r\n"
             "</body>\r\n"
             "</html>\r\n",

             MAX_WATER_CAPACITY,
             MAX_WATER_CAPACITY,
             MAX_WATER_CAPACITY);

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    // libera memória alocada dinamicamente
    free(request);

    // libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}