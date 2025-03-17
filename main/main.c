#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/rtc.h"

#define TRIGGER_PIN 15
#define ECHO_PIN 14

#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_ID uart0
#define BAUD_RATE 115200

volatile uint64_t start_time = 0;
volatile uint64_t stop_time = 0;
volatile bool new_data = false;
volatile bool reading_active = false;

void echo_isr(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        start_time = time_us_64();  // Captura o tempo quando o Echo sobe
    } 
    if (events & GPIO_IRQ_EDGE_FALL) {
        stop_time = time_us_64();   // Captura o tempo quando o Echo desce
        new_data = true;            // Indica que há uma nova leitura
    }
}

void start_reading() {
    reading_active = true;
    printf("Leitura iniciada.\n");
}

void stop_reading() {
    reading_active = false;
    printf("Leitura parada.\n");
}

void init_uart() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

void init_sensor() {
    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
    gpio_put(TRIGGER_PIN, 0);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_isr);
}

void trigger_sensor() {
    gpio_put(TRIGGER_PIN, 1);
    sleep_us(10);
    gpio_put(TRIGGER_PIN, 0);
}

void print_time() {
    datetime_t t;
    rtc_get_datetime(&t);
    printf("%02d:%02d:%02d - ", t.hour, t.min, t.sec);
}

void read_distance() {
    trigger_sensor();
    sleep_ms(60);

    if (new_data) {
        uint64_t pulse_duration = stop_time - start_time;
        float distance_cm = pulse_duration / 58.0;  // Conversão do tempo para cm
        print_time();
        printf("%.2f cm\n", distance_cm);
        new_data = false;
    } else {
        print_time();
        printf("Falha\n");
    }
}

void process_terminal() {
    int caracter = getchar_timeout_us(1000);
    if (caracter == 's') {
        start_reading();
    } else if (caracter == 'p') {
        stop_reading();
    }
}

int main() {
    stdio_init_all();
    init_uart();
    init_sensor();

    rtc_init();
    datetime_t t = {.year = 2025, .month = 3, .day = 17, .dotw = 1, .hour = 22, .min = 0, .sec = 0};
    rtc_set_datetime(&t);

    printf("Sistema pronto. Pressione 's' para iniciar ou 'p' para parar.\n");

    while (1) {
        process_terminal();
        if (reading_active) {
            read_distance();
        }
        sleep_ms(1000);
    }

    return 0;
}