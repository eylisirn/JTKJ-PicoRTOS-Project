#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BUTTON_PIN 2

int main() {
    stdio_init_all();
    sleep_ms(500);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    printf("READY\n");

    const char* morse = ".... . .-.. .-.. --- -.-.--\n";

    while (1) {
        if (!gpio_get(BUTTON_PIN)) {
            printf("%s", morse);
            sleep_ms(300);
            while (!gpio_get(BUTTON_PIN));
        }
    }
}
