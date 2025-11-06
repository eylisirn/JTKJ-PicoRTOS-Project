#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>

void imu_task(void* pvParameters) {
    (void)pvParameters;

    float ax, ay, az, gx, gy, gz, t;
    // IMU-sensorin asetelma
    if (init_ICM42670() == 0) {
        printf("ICM-42670P initialized successfully!\n");
        if (ICM42670_start_with_default_values() != 0) {
            printf("ICM-42670P could not initialize accelerometer or gyroscope");
        }
        int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
        printf("Enable gyro: %d\n", _enablegyro);
        int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        printf("Gyro return:  %d\n", _gyro);
        int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
        printf("Accel return:  %d\n", _accel);
    }
    else {
        printf("Failed to initialize ICM-42670P.\n");
    }
    const uint LED_PIN = 25; // LED-asetelme
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (1) {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            // Keskitä orientaatio
            float abs_ax = (ax > 0) ? ax : -ax;
            float abs_ay = (ay > 0) ? ay : -ay;
            float abs_az = (az > 0) ? az : -az;

            // Akselin valinta
            if (abs_az > 7.0 && abs_az > abs_ax && abs_az > abs_ay) {
                // Z-akseli, Viiva (-)
                printf("-\n");
                gpio_put(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(600));  // Dash = long signal
                gpio_put(LED_PIN, 0);
            }
            else if (abs_ax > 5.0 || abs_ay > 5.0) {
                // X- tai Y-akseli, Piste (.)
                printf(".\n");
                gpio_put(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(200));
                gpio_put(LED_PIN, 0);
            }
            else {
                // Viive
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        else {
            printf("IMU-sensorin lukeminen epäonnistui.a\n");
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }
    i2c_init(i2c0, 400 * 1000); // I2C asetelma
    gpio_set_function(12, GPIO_FUNC_I2C);
    gpio_set_function(13, GPIO_FUNC_I2C);
    gpio_pull_up(12);
    gpio_pull_up(13);
    init_hat_sdk();
    sleep_ms(300);
    init_led();
    printf("Aloita testi\n");

    TaskHandle_t hIMUTask = NULL;

    xTaskCreate(imu_task, "IMUTask", 1024, NULL, 2, &hIMUTask);

    // FreeRTOS-vuorontaja
    vTaskStartScheduler();

    return 0;
}