#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/http/client.h>
#include <errno.h>

#include "../inc/wifi_handler.h"


static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface);
static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb);
static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb);
static void handle_ipv4_result(struct net_if *iface);

void init_wifi() {

    net_mgmt_init_event_callback(
			&wifi_cb, 
			wifi_mgmt_event_handler,
			NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT
		);

	net_mgmt_init_event_callback(
			&ipv4_cb, 
			wifi_mgmt_event_handler, 
			NET_EVENT_IPV4_ADDR_ADD
		);
}

void response_cb(struct http_response *rsp,
                        enum http_final_call final_data,
                        void *user_data)
{
    if (final_data == HTTP_DATA_MORE) {
        printk("Partial data received (%zd bytes)", rsp->data_len);
    } else if (final_data == HTTP_DATA_FINAL) {
        printk("All the data received (%zd bytes)", rsp->data_len);
    }

    printk("Response status %s", rsp->http_status);
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event)
		{
		case NET_EVENT_WIFI_CONNECT_RESULT:
			handle_wifi_connect_result(cb);
			break;
		case NET_EVENT_WIFI_DISCONNECT_RESULT:
			handle_wifi_disconnect_result(cb);
			break;
		case NET_EVENT_IPV4_ADDR_ADD:
			handle_ipv4_result(iface);
			break;
		default:
			break;
	}
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
const struct wifi_status *status = (const struct wifi_status *)cb->info;
    if (status->status)
    {
        printk("Connection request failed (%d)\n", status->status);
    }
    else
    {
        printk("Connected\n");
        //k_sem_give(&wifi_connected);
    }
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;
    if (status->status)
    {
        printk("Disconnection request (%d)\n", status->status);
    }
    else
    {
        printk("Disconnected\n");
        //k_sem_take(&wifi_connected, K_NO_WAIT);
    }
}

static void handle_ipv4_result(struct net_if *iface)
{
    int i = 0;
    for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
        char buf[NET_IPV4_ADDR_LEN];
        if (iface->config.ip.ipv4->unicast[i].addr_type != NET_ADDR_DHCP) {
            continue;
        }
        printk("IPv4 address: %s\n",
        net_addr_ntop(AF_INET,
            &iface->config.ip.ipv4->unicast[i].address.in_addr,
            buf, sizeof(buf)));
        printk("Subnet: %s\n", net_addr_ntop(AF_INET,
            &iface->config.ip.ipv4->netmask,
            buf, sizeof(buf)));
        printk("Router: %s\n",
            net_addr_ntop(AF_INET,
            &iface->config.ip.ipv4->gw,
            buf, sizeof(buf)));
    }
    //k_sem_give(&ipv4_address_obtained);
}

void wifi_connect(void)
{
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params wifi_params = {0};
    wifi_params.ssid = SSID;
    wifi_params.psk = PSK;
    wifi_params.ssid_length = strlen(SSID);
    wifi_params.psk_length = strlen(PSK);
    wifi_params.channel = WIFI_CHANNEL_ANY;
    wifi_params.security = WIFI_SECURITY_TYPE_PSK;
    wifi_params.band = WIFI_FREQ_BAND_2_4_GHZ;
    wifi_params.mfp = WIFI_MFP_OPTIONAL;
    printk("Connecting to SSID: %s\n", wifi_params.ssid);
    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(struct wifi_connect_req_params))) {
        printk("WiFi Connection Request Failed\n");
    }
}

void wifi_status(void)
{
    struct net_if *iface = net_if_get_default();
    struct wifi_iface_status status = {0};
    if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(struct wifi_iface_status))){
        printk("WiFi Status Request Failed\n");
    }
    printk("\n");
    if (status.state >= WIFI_STATE_ASSOCIATED){
        printk("SSID: %-32s\n", status.ssid);
        printk("Channel: %d\n", status.channel);
        printk("RSSI: %d\n", status.rssi);
    }
}

