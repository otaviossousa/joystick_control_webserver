#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lib/joystick.h"
#include "lwipopts.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

// Configurações de Wi-Fi
#define WIFI_SSID "MudeAqui" // Nome da rede Wi-Fi (SSID)
#define WIFI_PASSWORD "MudeAqui" // Senha da rede Wi-Fi

char html[1024]; // Buffer para armazenar o conteúdo HTML da página

// Declaração de funções auxiliares
void build_html_content(float joystick_x, float joystick_y, const char* joystick_direction);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

int main() {
    stdio_init_all();    // Inicializa a entrada e saída padrão
    init_joystick();     // Inicializa o joystick

    // Tenta inicializar o módulo Wi-Fi
    while (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100); // Aguarda 100ms antes de tentar novamente
        return -1; // Sai do programa em caso de falha
    }

    // Inicializa o Wi-Fi em modo STA (cliente)
    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");

    // Conecta ao Wi-Fi com timeout de 20 segundos
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    
    printf("Conectado ao Wi-Fi\n");
    
    // Exibe o endereço IP do dispositivo, se disponível
    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Cria um novo PCB (Protocol Control Block) para o servidor TCP
    struct tcp_pcb *server = tcp_new(); 
    
    if (!server) { 
        printf("Falha ao criar servidor TCP\n");
        return -1; // Sai do programa em caso de falha
    }
    
    // Associa o servidor TCP à porta 80
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) { 
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1; // Sai do programa em caso de falha
    }

    // Coloca o servidor em modo de escuta para conexões TCP
    server = tcp_listen(server);
    
    // Configura o callback para aceitar conexões TCP com o servidor
    tcp_accept(server, tcp_server_accept);

    printf("Servidor ouvindo na porta 80\n");

    while (true) {
        // Processa eventos de rede
        cyw43_arch_poll();

        // Lê a posição do joystick
        float joystick_x = read_joystick_x();
        float joystick_y = read_joystick_y();
        const char* joystick_direction = get_joystick_direction(joystick_x, joystick_y);

        // Atualiza o conteúdo HTML com os dados do joystick
        build_html_content(joystick_x, joystick_y, joystick_direction);

        // Aguarda 1 segundo antes de repetir o loop
        sleep_ms(1000); 
    }
}

// Função para construir o HTML 
void build_html_content(float joystick_x, float joystick_y, const char* joystick_direction) {
    snprintf(html, sizeof(html),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "\r\n"
    "<!DOCTYPE html>\n"
    "<html lang=\"pt-BR\">\n"
    "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<title>Posição Joystick</title>\n"
    "</head>\n"
    "<body bgcolor=\"#f4f4f4\">\n"
        "<table border=\"0\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\">\n"
            "<tr><td height=\"250\"></td></tr>\n"
            "<tr>\n"
                "<td align=\"center\">\n"
                    "<h1 style=\"color:#320e74;\">Painel Joystick</h1>\n"
                    "<table border=\"1\" cellpadding=\"10\" cellspacing=\"0\" width=\"400\">\n"
                        "<tr bgcolor=\"#320e74\" style=\"color:white;\">\n"
                            "<th>Propriedade</th>\n"
                            "<th>Valor</th>\n"
                        "</tr>\n"
                        "<tr bgcolor=\"#ffffff\">\n"
                            "<td>Joystick X</td>\n"
                            "<td>%.2f</td>\n"
                        "</tr>\n"
                        "<tr bgcolor=\"#ffffff\">\n"
                            "<td>Joystick Y</td>\n"
                            "<td>%.2f</td>\n"
                        "</tr>\n"
                        "<tr bgcolor=\"#ffffff\">\n"
                            "<td>Direção</td>\n"
                            "<td>%s</td>\n"
                        "</tr>\n"
                    "</table>\n"
                    "<p style=\"font-size:20px;color:#666;\">&copy; 2025 EmbarcaTech</p>\n"
                "</td>\n"
            "</tr>\n"
        "</table>\n"
        "<script>\n"
            "setTimeout(() => { window.location.href = \"/\"; }, 1000);\n"
        "</script>\n"
    "</body>\n"
    "</html>\n", joystick_x, joystick_y, joystick_direction);
}

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    // Verifica se houve erro na recepção de dados
    if (!p) {  
        // Fecha a sessão se for verdadeiro
        tcp_close(tpcb);
        // Retorna sucesso 
        return ERR_OK; 
    }

    // Lê a posição do joystick
    float joystick_x = read_joystick_x();
    float joystick_y = read_joystick_y();
    const char* joystick_direction = get_joystick_direction(joystick_x, joystick_y);

    // Formata o HTML com os dados atuais
    build_html_content(joystick_x, joystick_y, joystick_direction);

    // Envia o HTML formatado para o cliente
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    // Envia os dados para o cliente
    tcp_output(tpcb); 
    // Libera o buffer de recepção
    pbuf_free(p); 
    // Retorna sucesso
    return ERR_OK; 
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    // 
    tcp_recv(newpcb, tcp_server_recv);
    // Retorna sucesso
    return ERR_OK;
}
