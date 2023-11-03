#ifndef INC_WIFI_HANDLER_H_
#define INC_WIFI_HANDLER_H_

#define SSID "Veni"
#define PSK "yatangaki7"

#define HTTP_HOST "192.168.78.158"
#define HTTP_PORT "5000"

void wifi_connect(void);
void wifi_status(void);
void init_wifi();
void response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data);

#endif