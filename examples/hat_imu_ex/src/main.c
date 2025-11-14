#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tkjhat/sdk.h"

// --- Pin configuration ---
#define LED_PIN     25

// --- Shared flags ---
volatile bool button1_pressed_flag = false;
volatile bool button2_pressed_flag = false;
volatile bool send_message_flag    = false;

// --- Double-press tracking ---
static uint32_t last_button2_press_time = 0;
static bool button2_first_press = false;

// --- Message buffer ---
#define MESSAGE_BUFFER_SIZE 128
char message_buffer[MESSAGE_BUFFER_SIZE];
size_t message_index = 0;

// --- ISR for both buttons ---
void button_isr(uint gpio, uint32_t events) {
    static uint32_t last_button1_time = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON1 && (now - last_button1_time > 200)) {
        button1_pressed_flag = true;
        last_button1_time = now;
    } 
    else if (gpio == BUTTON2) {
        if (now - last_button2_press_time < 400 && button2_first_press) {
            // Detected double-press → send message
            send_message_flag = true;
            button2_first_press = false;
        } else {
            // Single press → add space
            button2_first_press = true;
            button2_pressed_flag = true;
            last_button2_press_time = now;
        }
    }
}

// --- IMU + Message Task ---
void imu_task(void* pvParameters) {
    (void)pvParameters;

    float ax, ay, az, gx, gy, gz, t;

    // Initialize IMU
    if (init_ICM42670() == 0) {
        printf("Valamista! IMU-sensori alustettiin!\n");
        if (ICM42670_start_with_default_values() != 0) {
            printf("Virhe! IMU-sensorin gyroskooppia tai kiihtyvyysanturia ei voitu alustaa!\n");
        }
        int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
        int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);

        ICM42670_enable_accel_gyro_ln_mode();
        ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
    } else {
        printf("Virhe! IMU-sensoria ei voitu alustaa!\n");
    }

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1) {
        char symbol = '\0';

        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            float abs_ax = fabsf(ax);
            float abs_ay = fabsf(ay);
            float abs_az = fabsf(az);

            if (abs_az > 0.95 && abs_az > abs_ax && abs_az > abs_ay) {
                symbol = '-';
            } else if (abs_ax > 0.10 || abs_ay > 0.40) {
                symbol = '.';
            }

            // Always update display
            clear_display();
            if (symbol != '\0') {
                char text[2] = {symbol, '\0'};
                write_text(text);
            } else {
                write_text(" ");
            }
        }

        // --- Button 1: Add current symbol ---
        if (button1_pressed_flag && symbol != '\0') {
            button1_pressed_flag = false;

            if (message_index < MESSAGE_BUFFER_SIZE - 1) {
                message_buffer[message_index++] = symbol;
                message_buffer[message_index] = '\0';
                printf("Added symbol: %c (buffer: %s)\n", symbol, message_buffer);
            }

            gpio_put(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN, 0);
        }

        // --- Button 2: Add space ---
        if (button2_pressed_flag) {
            button2_pressed_flag = false;

            if (message_index < MESSAGE_BUFFER_SIZE - 1) {
                message_buffer[message_index++] = ' ';
                message_buffer[message_index] = '\0';
                printf("%c\n", "");
            }

            // LED double blink for space
            for (int i = 0; i < 2; i++) {
                gpio_put(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(80));
                gpio_put(LED_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(80));
            }
        }

        // --- Double-press (send message) ---
        if (send_message_flag) {
            send_message_flag = false;

            if (message_index > 0) {
                printf("%s\n", message_buffer);

                // Send the whole buffered message
                printf("%s\n", message_buffer);  // <-- includes newline
                fflush(stdout);

                // LED triple blink for send
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
                printf("(Buffer empty, nothing to send)\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// --- Main entry point ---
int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }

    printf("USB connected.\n");

    // --- Configure buttons ---
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);
    gpio_pull_up(BUTTON1);

    gpio_init(BUTTON2);
    gpio_set_dir(BUTTON2, GPIO_IN);
    gpio_pull_up(BUTTON2);

    // Register interrupts
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &button_isr);
    gpio_set_irq_enabled(BUTTON2, GPIO_IRQ_EDGE_FALL, true);

    // --- Initialize I2C and display ---
    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(12, GPIO_FUNC_I2C);
    gpio_set_function(13, GPIO_FUNC_I2C);
    gpio_pull_up(12);
    gpio_pull_up(13);
    sleep_ms(300);

    init_led();
    init_display();
    clear_display();
    write_text("Valamista!");
    printf("Valamista!\n");

    // --- Start IMU task ---
    TaskHandle_t hIMUTask = NULL;
    xTaskCreate(imu_task, "IMUTask", 2048, NULL, 2, &hIMUTask);

    vTaskStartScheduler();
    return 0;
}
