#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <httpd/httpd.h>
#include "wifi.h"
#include <task.h>
#include <string.h>
#include <espressif/esp_common.h>

static void wifi_init() {
    struct sdk_station_config wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
    };

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();
}

enum {
    SSI_UPTIME,
    SSI_FREE_HEAP
};

const char *pcConfigSSITags[] = {
    "uptime", // SSI_UPTIME
    "heap",   // SSI_FREE_HEAP
};

int32_t ssi_handler(int32_t iIndex, char *pcInsert, int32_t iInsertLen)
{
    switch (iIndex) {
        case SSI_UPTIME:
            snprintf(pcInsert, iInsertLen, "%d",
                    xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
            break;
        case SSI_FREE_HEAP:
            snprintf(pcInsert, iInsertLen, "%d", (int) xPortGetFreeHeapSize());
            break;
        default:
            snprintf(pcInsert, iInsertLen, "N/A");
            break;
    }

    /* Tell the server how many characters to insert */
    return (strlen(pcInsert));
}

char *index_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    return "/index.ssi";
}

void restart_task() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sdk_system_restart();
}

char *actions_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  for (int i = 0; i < iNumParams; i++) {
    if (strcmp(pcParam[i], "action") == 0) {
      if (strcmp(pcValue[i], "reboot") == 0) {
        xTaskCreate(&restart_task, "sys-restart", 128, NULL, 2, NULL);
        return "/redir_reboot.html";
      }
      break;
    }
  }
  return "/redir.html";
}

char *delete_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    return "/redir.html";
}

char *new_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    return "/redir.html";
}


void httpd_task(void *pvParameters)
{
    tCGI pCGIs[] = {
        {"/", (tCGIHandler) index_cgi_handler},
        {"/actions.cgi", (tCGIHandler) actions_cgi_handler},
        {"/delete-blind.cgi", (tCGIHandler) delete_cgi_handler},
        {"/new-blind.cgi", (tCGIHandler) new_cgi_handler},
    };

    /* register handlers and start the server */
    http_set_cgi_handlers(pCGIs, sizeof (pCGIs) / sizeof (pCGIs[0]));
    http_set_ssi_handler((tSSIHandler) ssi_handler, pcConfigSSITags,
            sizeof (pcConfigSSITags) / sizeof (pcConfigSSITags[0]));
    httpd_init();

    for (;;);
}


void user_init(void) {
    uart_set_baud(0, 115200);
    wifi_init();

    xTaskCreate(&httpd_task, "HTTP Daemon", 128, NULL, 2, NULL);
    printf("user init done\n");

}
