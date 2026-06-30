#include "nrf24.h"
#include <furi.h>
#include <furi_hal_spi.h>
#include <furi_hal_gpio.h>
#include <furi_hal_resources.h>

#define NRF24_CE_PIN      &gpio_ext_pb2
#define NRF24_CSN_PIN     &gpio_ext_pa4
#define BLE_ADV_CH_START  37
#define BLE_ADV_CH_END    39
#define BLE_ADV_FREQ(ch)  (ch == 37 ? 2402 : ch == 38 ? 2426 : 2480)
#define NRF24_FREQ(ch)    (2400 + ch)

#define NRF24_CMD_R_REGISTER     0x00
#define NRF24_CMD_W_REGISTER     0x20
#define NRF24_CMD_R_RX_PAYLOAD   0x61
#define NRF24_CMD_W_TX_PAYLOAD   0xA0
#define NRF24_CMD_FLUSH_TX       0xE1
#define NRF24_CMD_FLUSH_RX       0xE2
#define NRF24_CMD_REUSE_TX_PL    0xE3
#define NRF24_CMD_NOP            0xFF

#define NRF24_REG_CONFIG         0x00
#define NRF24_REG_EN_AA          0x01
#define NRF24_REG_EN_RXADDR      0x02
#define NRF24_REG_SETUP_AW       0x03
#define NRF24_REG_SETUP_RETR     0x04
#define NRF24_REG_RF_CH          0x05
#define NRF24_REG_RF_SETUP       0x06
#define NRF24_REG_STATUS         0x07
#define NRF24_REG_OBSERVE_TX     0x08
#define NRF24_REG_RX_ADDR_P0     0x0A
#define NRF24_REG_RX_ADDR_P1     0x0B
#define NRF24_REG_TX_ADDR        0x10
#define NRF24_REG_RX_PW_P0       0x11
#define NRF24_REG_FIFO_STATUS    0x17
#define NRF24_REG_DYNPD          0x1C
#define NRF24_REG_FEATURE        0x1D

#define RX_DR_MASK   0x40
#define TX_DS_MASK   0x20
#define MAX_RT_MASK  0x10
#define RX_P_NO_MASK 0x0E
#define STATUS_TX_FULL 0x01

#define nrf24_cs_low()   furi_hal_gpio_write(NRF24_CSN_PIN, false)
#define nrf24_cs_high()  furi_hal_gpio_write(NRF24_CSN_PIN, true)
#define nrf24_ce_low()   furi_hal_gpio_write(NRF24_CE_PIN, false)
#define nrf24_ce_high()  furi_hal_gpio_write(NRF24_CE_PIN, true)

static bool nrf24_initialized = false;

static void nrf24_send_cmd(uint8_t cmd) {
    furi_hal_spi_bus_tx(&furi_hal_spi_bus_handle_external, &cmd, 1, 100);
}

static uint8_t nrf24_transfer(uint8_t byte) {
    uint8_t rx;
    furi_hal_spi_bus_trx(&furi_hal_spi_bus_handle_external, &byte, &rx, 1, 100);
    return rx;
}

bool nrf24_init(void) {
    if(nrf24_initialized) return true;

    furi_hal_gpio_init(NRF24_CE_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_init(NRF24_CSN_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);

    furi_hal_spi_bus_handle_init(&furi_hal_spi_bus_handle_external);
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);

    nrf24_ce_low();
    nrf24_cs_high();
    furi_delay_ms(5);

    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_FLUSH_RX);
    nrf24_cs_high();

    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_FLUSH_TX);
    nrf24_cs_high();

    uint8_t config = nrf24_read_reg(NRF24_REG_CONFIG);
    config |= 0x0C;
    config &= ~0x02;
    nrf24_write_reg(NRF24_REG_CONFIG, config);
    nrf24_write_reg(NRF24_REG_EN_AA, 0x00);
    nrf24_write_reg(NRF24_REG_EN_RXADDR, 0x01);
    nrf24_write_reg(NRF24_REG_SETUP_AW, 0x03);
    nrf24_write_reg(NRF24_REG_SETUP_RETR, 0x00);
    nrf24_write_reg(NRF24_REG_RF_CH, 0x02);
    nrf24_write_reg(NRF24_REG_RF_SETUP, 0x26);
    nrf24_write_reg(NRF24_REG_DYNPD, 0x00);
    nrf24_write_reg(NRF24_REG_FEATURE, 0x00);

    uint8_t rx_addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    nrf24_write_buf(NRF24_REG_RX_ADDR_P0, rx_addr, 5);
    nrf24_write_buf(NRF24_REG_TX_ADDR, rx_addr, 5);

    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
    nrf24_initialized = true;
    return true;
}

void nrf24_deinit(void) {
    if(!nrf24_initialized) return;
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    nrf24_ce_low();
    nrf24_cs_high();
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
    furi_hal_spi_bus_handle_deinit(&furi_hal_spi_bus_handle_external);
    nrf24_initialized = false;
}

bool nrf24_check_connection(void) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    uint8_t reg = nrf24_read_reg(NRF24_REG_CONFIG);
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
    return reg != 0xFF && reg != 0x00;
}

uint8_t nrf24_read_reg(uint8_t reg) {
    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_R_REGISTER | reg);
    uint8_t val = nrf24_transfer(NRF24_CMD_NOP);
    nrf24_cs_high();
    return val;
}

void nrf24_write_reg(uint8_t reg, uint8_t value) {
    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_W_REGISTER | reg);
    nrf24_transfer(value);
    nrf24_cs_high();
}

void nrf24_write_buf(uint8_t reg, const uint8_t* data, uint8_t len) {
    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_W_REGISTER | reg);
    for(uint8_t i = 0; i < len; i++) nrf24_transfer(data[i]);
    nrf24_cs_high();
}

void nrf24_read_buf(uint8_t reg, uint8_t* data, uint8_t len) {
    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_R_REGISTER | reg);
    for(uint8_t i = 0; i < len; i++) data[i] = nrf24_transfer(NRF24_CMD_NOP);
    nrf24_cs_high();
}

void nrf24_set_mode(NRF24Mode mode) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    uint8_t config = nrf24_read_reg(NRF24_REG_CONFIG);
    if(mode == NRF24ModeRx) {
        config |= 0x01;
        nrf24_write_reg(NRF24_REG_CONFIG, config);
        furi_delay_ms(1);
        nrf24_ce_high();
    } else {
        config &= ~0x01;
        nrf24_write_reg(NRF24_REG_CONFIG, config);
        nrf24_ce_low();
    }
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
}

void nrf24_set_channel(uint8_t channel) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    nrf24_write_reg(NRF24_REG_RF_CH, channel < 125 ? channel : 0);
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
}

uint8_t nrf24_get_channel(void) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    uint8_t ch = nrf24_read_reg(NRF24_REG_RF_CH);
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
    return ch;
}

void nrf24_set_rx_address(const uint8_t* addr, uint8_t len) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    nrf24_write_buf(NRF24_REG_RX_ADDR_P0, addr, len);
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
}

void nrf24_set_tx_address(const uint8_t* addr, uint8_t len) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    nrf24_write_buf(NRF24_REG_TX_ADDR, addr, len);
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
}

void nrf24_set_payload_size(uint8_t size) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    nrf24_write_reg(NRF24_REG_RX_PW_P0, size > 32 ? 32 : size);
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
}

void nrf24_flush_rx(void) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_FLUSH_RX);
    nrf24_cs_high();
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
}

void nrf24_flush_tx(void) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_FLUSH_TX);
    nrf24_cs_high();
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
}

uint8_t nrf24_status(void) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    nrf24_cs_low();
    uint8_t status = nrf24_transfer(NRF24_CMD_NOP);
    nrf24_cs_high();
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
    return status;
}

uint8_t nrf24_fifo_status(void) {
    return nrf24_read_reg(NRF24_REG_FIFO_STATUS);
}

void nrf24_set_ble_adv_mode(bool enable) {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    if(enable) {
        uint8_t addr_ble[5] = {0x8E, 0x89, 0xBE, 0xD6, 0x42};
        nrf24_write_buf(NRF24_REG_RX_ADDR_P0, addr_ble, 5);
        nrf24_write_buf(NRF24_REG_TX_ADDR, addr_ble, 5);
        nrf24_write_reg(NRF24_REG_CONFIG, 0x0F);
        nrf24_write_reg(NRF24_REG_RF_SETUP, 0x46);
        nrf24_write_reg(NRF24_REG_EN_AA, 0x00);
        nrf24_write_reg(NRF24_REG_EN_RXADDR, 0x01);
    } else {
        nrf24_write_reg(NRF24_REG_CONFIG, 0x0C);
        nrf24_write_reg(NRF24_REG_RF_SETUP, 0x26);
    }
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
}

bool nrf24_receive_packet(NRF24Packet* packet, uint32_t timeout_ms) {
    if(!packet) return false;

    uint32_t start = furi_get_tick();
    while(furi_get_tick() - start < timeout_ms) {
        uint8_t status;
        furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
        nrf24_cs_low();
        status = nrf24_transfer(NRF24_CMD_NOP);
        nrf24_cs_high();

        if(status & RX_DR_MASK) {
            nrf24_write_reg(NRF24_REG_STATUS, RX_DR_MASK);

            uint8_t pipe = (status & RX_P_NO_MASK) >> 1;
            if(pipe <= 5) {
                nrf24_cs_low();
                nrf24_send_cmd(NRF24_CMD_R_RX_PAYLOAD);
                packet->payload_len = 0;
                uint8_t byte;
                for(uint8_t i = 0; i < 32; i++) {
                    byte = nrf24_transfer(NRF24_CMD_NOP);
                    if(packet->payload_len < 32) {
                        packet->payload[packet->payload_len++] = byte;
                    }
                }
                nrf24_cs_high();
                furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
                return true;
            }
        }
        furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
        furi_delay_ms(1);
    }
    return false;
}

bool nrf24_send_packet(const uint8_t* data, uint8_t len, uint32_t timeout_ms) {
    if(!data || len > 32) return false;

    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
    nrf24_ce_low();

    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_FLUSH_TX);
    nrf24_cs_high();

    nrf24_cs_low();
    nrf24_send_cmd(NRF24_CMD_W_TX_PAYLOAD);
    for(uint8_t i = 0; i < len; i++) nrf24_transfer(data[i]);
    nrf24_cs_high();

    nrf24_ce_high();
    furi_delay_ms(1);
    nrf24_ce_low();
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);

    uint32_t start = furi_get_tick();
    while(furi_get_tick() - start < timeout_ms) {
        uint8_t status = nrf24_status();
        if(status & (TX_DS_MASK | MAX_RT_MASK)) {
            nrf24_write_reg(NRF24_REG_STATUS, TX_DS_MASK | MAX_RT_MASK);
            return (status & TX_DS_MASK) != 0;
        }
        furi_delay_ms(1);
    }
    return false;
}

bool nrf24_send_burst(const uint8_t* data, uint8_t len, uint8_t count, uint32_t interval_ms) {
    for(uint8_t i = 0; i < count; i++) {
        if(!nrf24_send_packet(data, len, 100)) return false;
        if(interval_ms > 0) furi_delay_ms(interval_ms);
    }
    return true;
}

bool nrf24_scan_channels(uint8_t start, uint8_t end, int8_t* rssi_out, uint32_t timeout_ms) {
    if(!rssi_out || start > end || end > 125) return false;

    for(uint8_t ch = start; ch <= end; ch++) {
        nrf24_set_channel(ch);
        nrf24_set_mode(NRF24ModeRx);
        furi_delay_ms(5);

        NRF24Packet pkt = {0};
        if(nrf24_receive_packet(&pkt, timeout_ms)) {
            rssi_out[ch] = pkt.rssi;
        } else {
            uint8_t cd;
            furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
            cd = nrf24_read_reg(0x09);
            furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
            rssi_out[ch] = cd ? -50 : -100;
        }
    }
    return true;
}
