// IMU-SENSORI (PLACEHOLDER KOODI, REVITTY ESIMERKKEJÄ "hat_imu_ex" TIEDOSTOSTA
#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>

//#define I2C_PORT i2c0

void imu_task(void* pvParameters) {
    (void)pvParameters;

    float gx, gy, gz;
    // Setting up the sensor. 
    if (init_ICM42670() == 0) {
        printf("ICM-42670P initialized successfully!\n");
        if (ICM42670_start_with_default_values() != 0) {
            printf("ICM-42670P could not initialize gyroscope");
        }
        int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
        printf ("Enable gyro: %d\n",_enablegyro);
        int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        printf ("Gyro return:  %d\n", _gyro);
    }
    else {
        printf("Failed to initialize ICM-42670P.\n");
    }
    // Start collection data here. Infinite loop. 
    while (1)
    {
        if (ICM42670_read_sensor_data(&gx, &gy, &gz) == 0) {

            printf("Gyro: X=%f, Y=%f, Z=%f\n",gx, gy, gz);

        }
        else {
            printf("Failed to read imu data\n");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }
    //i2c_init(I2C_PORT, 400 * 1000);
    //gpio_set_function(12, GPIO_FUNC_I2C);
    //gpio_set_function(13, GPIO_FUNC_I2C);
    //gpio_pull_up(12);
    //gpio_pull_up(13);
    init_hat_sdk();
    sleep_ms(300); // Wait some time so initialization of USB and hat is done.
    init_led();
    printf("Start test\n");

    TaskHandle_t hIMUTask = NULL;

    xTaskCreate(imu_task, "IMUTask", 4096, NULL, 2, &hIMUTask);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;
}