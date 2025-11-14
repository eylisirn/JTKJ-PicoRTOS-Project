#include <stdio.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <math.h>
#include <tkjhat/sdk.h>

// Flags set by interrupts
volatile bool button1_flag = false;
volatile bool button2_flag = false;
volatile bool send_flag = false;

// Double-press detection for BUTTON2
static uint32_t last_btn2_time = 0;
static bool btn2_first = false;

// Message buffer
#define MAX_MSG 256
char message_buffer[MAX_MSG];
int message_index = 0;

/*
    INTERRUPT HANDLER for both buttons.
    - BUTTON1 → simply sets a flag (add a symbol)
    - BUTTON2 → checks for single press vs double press
*/
void button_isr(uint gpio, uint32_t events) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON1) {
        // Symbol capture request
        button1_flag = true;
    }
    else if (gpio == BUTTON2) {
        // Check double-press timing (< 400 ms)
        if (btn2_first && (now - last_btn2_time < 400)) {
            // Second press → send the full message
            send_flag = true;
            btn2_first = false;
            button2_flag = false;
        }
        else {
            // First press
            btn2_first = true;
            button2_flag = true;     // Single press → insert space
            last_btn2_time = now;
        }
    }
}

/*
    MAIN IMU TASK:
    - Reads orientation from ICM42670
    - Button 1 → Adds "." or "-"
    - Button 2 → Adds space
    - Double-press Button 2 → Sends whole buffer via USB
*/
void imu_task(void* p) {
    float ax, ay, az, gx, gy, gz, t;

    // Initialize IMU
    init_ICM42670();
    ICM42670_start_with_default_values();
    ICM42670_enable_accel_gyro_ln_mode();
    ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
    ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);

    // LED for signaling (blinks when message is sent)
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    for (;;) {
        // Read accelerometer + gyroscope
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {

            /*
                BUTTON 1 PRESSED:
                Determine whether user wants "." or "-"
                based on device tilt.
            */
            if (button1_flag) {
                button1_flag = false;

                float aax = fabsf(ax);
                float aay = fabsf(ay);
                float aaz = fabsf(az);

                char symbol = 0;

                // Flat on table → "-"
                if (aaz > 0.95 && aaz > aax && aaz > aay) {
                    symbol = '-';
                }
                // Tilt in X or Y → "."
                else if (aax > 0.10 || aay > 0.40) {
                    symbol = '.';
                }

                // Add symbol to buffer
                if (symbol && message_index < MAX_MSG - 1) {
                    message_buffer[message_index++] = symbol;
                    message_buffer[message_index] = '\0';
                    printf("%c", symbol);   // Live output
                    fflush(stdout);
                }
            }

            /*
                BUTTON 2 SINGLE PRESS:
                Insert a space character.
            */
            if (button2_flag) {
                button2_flag = false;

                if (message_index < MAX_MSG - 1) {
                    message_buffer[message_index++] = ' ';
                    message_buffer[message_index] = '\0';
                    printf(" ");
                    fflush(stdout);
                }
            }

            /*
                BUTTON 2 DOUBLE PRESS:
                Send the entire buffered message via USB.
            */
            if (send_flag) {
                send_flag = false;

                if (message_index > 0) {
                    // Send full message with newline
                    printf("\n%s\n", message_buffer);
                    fflush(stdout);

                    // Blink LED 3 times to confirm send
                    for (int i = 0; i < 3; i++) {
                        gpio_put(LED_PIN, 1);
                        vTaskDelay(pdMS_TO_TICKS(100));
                        gpio_put(LED_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }

                    // Clear buffer
                    message_index = 0;
                    message_buffer[0] = '\0';
                }
                else {
                    printf("\n");
                    fflush(stdout);
                }
            }
        }

        // Slight delay for stability
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

int main() {
    // Initialize USB + stdio
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(10);

    // Setup Button 1
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);
    gpio_pull_up(BUTTON1);

    // Setup Button 2
    gpio_init(BUTTON2);
    gpio_set_dir(BUTTON2, GPIO_IN);
    gpio_pull_up(BUTTON2);

    // Attach interrupts to both buttons
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &button_isr);
    gpio_set_irq_enabled(BUTTON2, GPIO_IRQ_EDGE_FALL, true);

    // Start IMU task
    TaskHandle_t h;
    xTaskCreate(imu_task, "IMU", 2048, NULL, 2, &h);

    // Start FreeRTOS scheduler
    vTaskStartScheduler();
    return 0;
}
