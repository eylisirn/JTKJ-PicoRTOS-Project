#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>

#include <tkjhat/sdk.h>   // IMU library header

void init_sw1(void) {
    gpio_init(SW1_PIN);
    gpio_set_dir(SW1_PIN, GPIO_IN);
    gpio_pull_up(SW1_PIN);   // Button active-low
}

void imu_task(void* pvParameters) {
    float ax, ay, az, gx, gy, gz, t;

    if (init_ICM42670() == 0) {
        ICM42670_start_with_default_values();
        ICM42670_enable_accel_gyro_ln_mode();
        ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
        printf("IMU ready.\n");
    }
    else {
        printf("IMU init failed!\n");
    }

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1) {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            float abs_ax = fabsf(ax), abs_ay = fabsf(ay), abs_az = fabsf(az);
            char symbol = '\0';
            if (abs_az > 0.95 && abs_az > abs_ax && abs_az > abs_ay)
                symbol = '-';
            else if (abs_ax > 0.10 || abs_ay > 0.40)
                symbol = '.';

            if (!gpio_get(SW1_PIN) && symbol != '\0') {   // button pressed
                putchar_raw(symbol);  // send pure char, no newline conversion
                fflush(stdout);
                gpio_put(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_put(LED_PIN, 0);
                // Simple debounce
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(10);

    init_sw1();
    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(12, GPIO_FUNC_I2C);
    gpio_set_function(13, GPIO_FUNC_I2C);
    gpio_pull_up(12);
    gpio_pull_up(13);

    TaskHandle_t hIMU = NULL;
    xTaskCreate(imu_task, "IMU", 2048, NULL, 2, &hIMU);
    vTaskStartScheduler();
    return 0;
}