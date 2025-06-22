#include "Webserver.h" // Biblioteca geral do sistema

void init_cyw43()
{
    // Inicializa a arquitetura CYW43
    if (cyw43_arch_init())
    {
        //////////////////// Falha ao inicializar o Wi-Fi
        sleep_ms(100);
    }

    // Coloca GPIO do módulo em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);
}

void connect_to_wifi()
{
    // Ativa o modo Station (cliente Wi-Fi)
    cyw43_arch_enable_sta_mode();

    //////////////////// Conectaando

    // Tenta conectar ao Wi-Fi com timeout de 20 segundos
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        /////////////// Falha na conexão Wi-Fi
        sleep_ms(100);
    }
    
    struct netif *netif = &cyw43_state.netif[0];
    while (netif_is_link_up(netif) == 0) {
        sleep_ms(100);
    }

    // Mostra o IP no terminal serial
    const ip4_addr_t *ip = netif_ip4_addr(netif);
    printf("Wi-Fi conectado! IP: %s\n", ip4addr_ntoa(ip));
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

static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    tcp_close(tpcb); // fecha a conexão após o envio da resposta
    return ERR_OK;
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    tcp_sent(newpcb, tcp_sent_callback); // callback para após envio
    return ERR_OK;
}

// Tratamento do request do usuário
void user_request(char **request_ptr)
{
    char *request = *request_ptr;

    if (strstr(request, ""))
    {
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
    user_request(&request);

    char html[7144];

    snprintf(html, sizeof(html),
             "<!DOCTYPE html>\r\n"
    "<html lang=\"pt-br\">\r\n"
    "<head>\r\n"
    "  <meta charset=\"UTF-8\">\r\n"
    "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
    "  <title>Controle de Nível de Água</title>\r\n"
    "  <style>\r\n"
    "    body { font-family: sans-serif; text-align: center; padding: 10px; margin: 0; background: #f9f9f9; }\r\n"
    "    h1 { color: #333; }\r\n"
    "    .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }\r\n"
    "    .status, .config { margin-bottom: 25px; }\r\n"
    "    .label { font-weight: bold; margin-bottom: 5px; display: block; color: #555; }\r\n"
    "    .valor { font-size: 1.2em; color: #000; }\r\n"
    "    #estadoBomba.ligada { color: #4CAF50; }\r\n"
    "    #estadoBomba.desligada { color: #f44336; }\r\n"
    "    .barra { width: 80%%; background: #ddd; border-radius: 8px; overflow: hidden; margin: 5px auto 15px auto; height: 25px; border: 1px solid #ccc; }\r\n"
    "    .preenchimento { height: 100%%; background: #2196F3; transition: width 0.5s ease; }\r\n"
    "    form div { margin-bottom: 15px; }\r\n"
    "    input[type=\"number\"] { font-size: 16px; padding: 8px; width: 80px; text-align: center; border: 1px solid #ccc; border-radius: 4px; }\r\n"
    "    button { font-size: 18px; padding: 12px 35px; margin: 10px; border: none; border-radius: 8px; background: #007bff; color: white; cursor: pointer; transition: background 0.3s; }\r\n"
    "    button:hover { background: #0056b3; }\r\n"
    "  </style>\r\n"
    "  <script>\r\n"
    "    function atualizarDados() {\r\n"
    "      fetch('/data')\r\n"
    "        .then(res => res.json())\r\n"
    "        .then(data => {\r\n"
    "          document.getElementById('nivelValor').innerText = data.nivel;\r\n"
    "          document.getElementById('barraNivel').style.width = data.nivel + '%%';\r\n"
    "          const estadoBombaEl = document.getElementById('estadoBomba');\r\n"
    "          if (data.bomba_ligada) {\r\n"
    "            estadoBombaEl.innerText = 'Ligada';\r\n"
    "            estadoBombaEl.className = 'valor ligada';\r\n"
    "          } else {\r\n"
    "            estadoBombaEl.innerText = 'Desligada';\r\n"
    "            estadoBombaEl.className = 'valor desligada';\r\n"
    "          }\r\n"
    "          document.getElementById('minConfigurado').innerText = data.min_config;\r\n"
    "          document.getElementById('maxConfigurado').innerText = data.max_config;\r\n"
    "        })\r\n"
    "        .catch(error => console.error('Erro ao buscar dados:', error));\r\n"
    "    }\r\n"
    "    function salvarConfiguracao(event) {\r\n"
    "      event.preventDefault();\r\n"
    "      const min = document.getElementById('inputMin').value;\r\n"
    "      const max = document.getElementById('inputMax').value;\r\n"
    "      fetch('/config', {\r\n"
    "        method: 'POST',\r\n"
    "        headers: { 'Content-Type': 'application/json' },\r\n"
    "        body: JSON.stringify({ min: min, max: max })\r\n"
    "      })\r\n"
    "        .then(response => {\r\n"
    "          if (response.ok) {\r\n"
    "            alert('Limites salvos com sucesso!');\r\n"
    "            atualizarDados();\r\n"
    "          } else {\r\n"
    "            alert('Erro ao salvar os limites.');\r\n"
    "          }\r\n"
    "        })\r\n"
    "        .catch(error => console.error('Erro ao salvar configuração:', error));\r\n"
    "    }\r\n"
    "    setInterval(atualizarDados, 2000);\r\n"
    "    window.onload = atualizarDados;\r\n"
    "  </script>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "  <div class=\"container\">\r\n"
    "    <h1>Controle de Nível de Água</h1>\r\n"
    "    <div class=\"status\">\r\n"
    "      <p class=\"label\">Nível Atual: <span id=\"nivelValor\" class=\"valor\">--</span>%%</p>\r\n"
    "      <div class=\"barra\">\r\n"
    "        <div id=\"barraNivel\" class=\"preenchimento\"></div>\r\n"
    "      </div>\r\n"
    "      <p class=\"label\">Estado da Bomba: <span id=\"estadoBomba\" class=\"valor desligada\">--</span></p>\r\n"
    "      <p class=\"label\" style=\"margin-top:20px;\">Limites Atuais: Mínimo de <span id=\"minConfigurado\" class=\"valor\">--</span>%% | Máximo de <span id=\"maxConfigurado\" class=\"valor\">--</span>%%</p>\r\n"
    "    </div>\r\n"
    "    <hr>\r\n"
    "    <div class=\"config\">\r\n"
    "      <h2>Configurar Novos Limites</h2>\r\n"
    "      <form id=\"configForm\" onsubmit=\"salvarConfiguracao(event)\">\r\n"
    "        <div>\r\n"
    "          <label for=\"inputMin\" class=\"label\">Nível Mínimo (%%):</label>\r\n"
    "          <input type=\"number\" id=\"inputMin\" required>\r\n"
    "        </div>\r\n"
    "        <div>\r\n"
    "          <label for=\"inputMax\" class=\"label\">Nível Máximo (%%):</label>\r\n"
    "          <input type=\"number\" id=\"inputMax\" required>\r\n"
    "        </div>\r\n"
    "        <button type=\"submit\">Salvar Novos Limites</button>\r\n"
    "      </form>\r\n"
    "    </div>\r\n"
    "  </div>\r\n"
    "</body>\r\n"
    "</html>\r\n");

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