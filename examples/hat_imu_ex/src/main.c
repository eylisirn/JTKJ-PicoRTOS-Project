#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Button pin (use your real pin)
#define BUTTON_PIN 2

// Morse version of "HELLO!"
const char* MORSE_MESSAGE =
".... "   // H
". "      // E
".-.. "   // L
".-.. "   // L
"--- "    // O
"-.-.--"  // !
"\n";     // End with newline

int main() {
    stdio_init_all();
    sleep_ms(500);  // Let USB CDC connect

    // Initialize button
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Active low

    printf("Ready. Press button to send HELLO! in Morse.\n");

    while (1) {
        // Button pressed? (active LOW)
        if (!gpio_get(BUTTON_PIN)) {
            printf("%s", MORSE_MESSAGE);
            sleep_ms(500); // Debounce + avoid spamming
            while (!gpio_get(BUTTON_PIN)); // Wait for release
        }
    }
}