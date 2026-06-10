#ifndef HTTP_CLIENT_H 
#define HTTP_CLIENT_H

#include "esp_err.h"

esp_err_t http_client_request();  // Initialize the wifi station
void http_client_task(void *pvParameters);   // Task to be run on the core 0

#endif