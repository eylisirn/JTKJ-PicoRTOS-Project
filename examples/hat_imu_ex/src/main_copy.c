/*#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>

void init_display(void);
void imu_task(void* pvParameters) {
    (void)pvParameters;

    float ax, ay, az, gx, gy, gz, t;
    // IMU-sensorin alustus
    if (init_ICM42670() == 0) {
        printf("Valamista! IMU-sensori alustettiin!\n");
        if (ICM42670_start_with_default_values() != 0) {
            printf("Virhe! IMU-sensorin gyroskooppia tai kiihtyvyysanturia ei voitu alustaa!");
        }
        int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
        printf("Gyro: %d\n", _enablegyro);
        int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        printf("Gyro syöte:  %d\n", _gyro);
        int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
        printf("Kiihtyvyys syöte:  %d\n", _accel);
    }
    else {
        printf("Virhe! IMU-sensoria ei voitu alustaa!\n");
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
            if (abs_az > 0.95 && abs_az > abs_ax && abs_az > abs_ay) {
                // Z-akseli, Viiva (-)
                clear_display();
                write_text("-");
                gpio_put(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(600));
                gpio_put(LED_PIN, 0);
            }
            else if (abs_ax > 0.10 || abs_ay > 0.40) {
                // X- tai Y-akseli, Piste (.)
                clear_display();
                write_text(".");
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
    sleep_ms(300);
    init_led();
    init_display();
    clear_display();
    write_text("Valamista!");
    printf("Valamista!\n");

    TaskHandle_t hIMUTask = NULL;

    xTaskCreate(imu_task, "IMUTask", 1024, NULL, 2, &hIMUTask);

    // FreeRTOS-vuorontaja
    vTaskStartScheduler();

    return 0;
}*\