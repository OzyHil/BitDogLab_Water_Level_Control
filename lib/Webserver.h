#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "General.h"
#include "Display.h"

#define LED_PIN CYW43_WL_GPIO_LED_PIN // GPIO do CI CYW43

#define WIFI_SSID "TSUNAMI_EVERALDO"   // Nome da rede Wi-Fi
#define WIFI_PASSWORD "amizade5560" // Senha da rede Wi-Fi

void init_cyw43(); // Inicializa a arquitetura CYW43

void connect_to_wifi(); // Conecta ao Wi-Fi

struct tcp_pcb *init_tcp_server(); // Inicializa o servidor TCP e vincula à porta 80

void run_tcp_server_loop(); // Loop principal para manter o Wi-Fi ativo e escutar conexões

void deinit_cyw43(); // Finaliza a arquitetura CYW43

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err); // Função de callback ao aceitar conexões TCP

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err); // Função de callback para processar requisições HTTP

void user_request(char **request, struct tcp_pcb *tpcb); // Tratamento do request do usuário

#endif