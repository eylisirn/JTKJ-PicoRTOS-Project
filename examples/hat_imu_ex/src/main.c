#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tkjhat/sdk.h"

// --- Pin configuration ---
#define LED_PIN     25

// --- Shared flag between interrupt and task ---
volatile bool button_pressed_flag = false;

// --- Interrupt Service Routine for button press ---
void button_isr(uint gpio, uint32_t events) {
    // Simple debounce: ignore presses within 200ms
    static uint32_t last_time = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_time > 200) {
        button_pressed_flag = true;
        last_time = now;
    }
}

// --- IMU task ---
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
        printf("Gyro: %d\n", _enablegyro);
        int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        printf("Gyro syöte: %d\n", _gyro);
        int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
        printf("Kiihtyvyys syöte: %d\n", _accel);
    } else {
        printf("Virhe! IMU-sensoria ei voitu alustaa!\n");
    }

    // Initialize LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1) {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            float abs_ax = fabsf(ax);
            float abs_ay = fabsf(ay);
            float abs_az = fabsf(az);

            char symbol = '\0';

            // Determine orientation symbol
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

            // If button was pressed, send current symbol over USB
            if (button_pressed_flag && symbol != '\0') {
                button_pressed_flag = false; // reset flag

                putchar_raw((int)symbol);  // Send to USB serial
                fflush(stdout);            // Ensure it's sent immediately

                // Flash LED to confirm send
                gpio_put(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(150));
                gpio_put(LED_PIN, 0);
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

    printf("USB yhdistetty.\n");

    // Initialize button with interrupt
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);
    gpio_pull_up(BUTTON1);  // active-low button
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &button_isr);

    // Initialize I2C and display
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

    // Create IMU FreeRTOS task
    TaskHandle_t hIMUTask = NULL;
    xTaskCreate(imu_task, "IMUTask", 2048, NULL, 2, &hIMUTask);

    // Start scheduler
    vTaskStartScheduler();

    return 0;
}