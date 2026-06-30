#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t mac[6];
    int8_t rssi;
    uint8_t channel;
    uint8_t payload_len;
    uint8_t payload[32];
    bool is_ble;
} NRF24Packet;

typedef enum {
    NRF24ModeRx,
    NRF24ModeTx,
} NRF24Mode;

bool nrf24_init(void);
void nrf24_deinit(void);

bool nrf24_check_connection(void);

void nrf24_set_mode(NRF24Mode mode);
void nrf24_set_channel(uint8_t channel);
uint8_t nrf24_get_channel(void);
void nrf24_set_rx_address(const uint8_t* addr, uint8_t len);
void nrf24_set_tx_address(const uint8_t* addr, uint8_t len);
void nrf24_set_payload_size(uint8_t size);

bool nrf24_receive_packet(NRF24Packet* packet, uint32_t timeout_ms);
bool nrf24_send_packet(const uint8_t* data, uint8_t len, uint32_t timeout_ms);
bool nrf24_send_burst(const uint8_t* data, uint8_t len, uint8_t count, uint32_t interval_ms);

void nrf24_flush_rx(void);
void nrf24_flush_tx(void);
uint8_t nrf24_status(void);
uint8_t nrf24_fifo_status(void);

void nrf24_set_ble_adv_mode(bool enable);
bool nrf24_scan_channels(uint8_t start, uint8_t end, int8_t* rssi_out, uint32_t timeout_ms);

uint8_t nrf24_read_reg(uint8_t reg);
void nrf24_write_reg(uint8_t reg, uint8_t value);
void nrf24_write_buf(uint8_t reg, const uint8_t* data, uint8_t len);
void nrf24_read_buf(uint8_t reg, uint8_t* data, uint8_t len);

#ifdef __cplusplus
}
#endif
