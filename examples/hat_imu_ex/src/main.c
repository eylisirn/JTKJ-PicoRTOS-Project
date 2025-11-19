#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tkjhat/sdk.h"

// --- Button flägit ---
volatile bool button1_pressed_flag = false;
volatile bool button2_pressed_flag = false;

// --- Morse bufferi ---
#define MORSE_BUFFER_SIZE 64
char morse_buffer[MORSE_BUFFER_SIZE];
uint8_t morse_index = 0;

// --- Interruptit napeille ---
void button_interrupt(uint gpio, uint32_t events) {
    static uint32_t last_time1 = 0;
    static uint32_t last_time2 = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON1 && (now - last_time1 > 200)) {
        button1_pressed_flag = true;
        last_time1 = now;
    }
    else if (gpio == BUTTON2 && (now - last_time2 > 200)) {
        button2_pressed_flag = true;
        last_time2 = now;
    }
}

// --- Sensori taski ---
void imu_task(void* pvParameters) {
    (void)pvParameters;

    float ax, ay, az, gx, gy, gz, t;

    // Sensorin käynnistys
    if (init_ICM42670() == 0) {
        int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
        int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
    }
    else {
        printf("IMU-sensori ei käynnistynyt.\n");
    }

    // Puhdista morse-bufferi
    morse_index = 0;
    morse_buffer[0] = '\0';

    while (1) {
        char symbol = '\0';

        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            float abs_ax = fabsf(ax);
            float abs_ay = fabsf(ay);
            float abs_az = fabsf(az);

            if (abs_az > 0.95 && abs_az > abs_ax && abs_az > abs_ay) {
                symbol = '-';
            }
            else if (abs_ax > 0.10 || abs_ay > 0.40) {
                symbol = '.';
            }

            // --- Päivitä LCD-näyttö ---
            clear_display();
            if (morse_index > 0) {
                write_text(morse_buffer);
            }
            else {
                write_text(" ");
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        

        // --- Nappi 1 eli lisää merkki (- tai .) ---
        if (button1_pressed_flag && symbol != '\0') {
            button1_pressed_flag = false;

            // Lisää merkki bufferiin
            if (morse_index < MORSE_BUFFER_SIZE - 1) {
                morse_buffer[morse_index++] = symbol;
                morse_buffer[morse_index] = '\0';
            }
        }

        // --- Nappi 2 eli lisää tyhjä väli ---
        if (button2_pressed_flag) {
            button2_pressed_flag = false;

            if (morse_index < MORSE_BUFFER_SIZE - 1) {
                morse_buffer[morse_index++] = ' ';
                morse_buffer[morse_index] = '\0';
            }
        }

        // --- Tarkista loppuuko bufferi kolmeen väliin ja lähetä viesti ---
        if (morse_index >= 3 && morse_buffer[morse_index - 3] == ' ' && morse_buffer[morse_index - 2] == ' ' && morse_buffer[morse_index - 1] == ' ') {
            // --- Lähetä viesti jos lopussa on kolme väliä ---
            if (morse_index > 3) {
                morse_buffer[morse_index - 3] = '\0';  // Poista lisätyt välit
                printf("%s  \n", morse_buffer);  // Lähetä serial clientiin, mutta lisää tunnistukseen tarvittavat välit
                stdio_flush();

                // Puhdista bufferi
                morse_index = 0;
                morse_buffer[0] = '\0';
            }
        }
    }
}

// --- Main ---
int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }

    printf("USB yhdistetty.\n");

    // --- Nappi konfiguraatio ---
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);
    gpio_pull_up(BUTTON1);
    gpio_init(BUTTON2);
    gpio_set_dir(BUTTON2, GPIO_IN);
    gpio_pull_up(BUTTON2);

    // Yhdistä interruptit
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &button_interrupt);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_FALL, true, &button_interrupt);

    // --- Käynnistä I2C ---
    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(12, GPIO_FUNC_I2C);
    gpio_set_function(13, GPIO_FUNC_I2C);
    gpio_pull_up(12);
    gpio_pull_up(13);
    sleep_ms(300);

    init_display();
    clear_display();
    write_text("Valamista!");
    printf("Valamista!\n");

    // --- Aloita FreeRTOS IMU Taski ---
    TaskHandle_t hIMUTask = NULL;
    xTaskCreate(imu_task, "IMUTask", 2048, NULL, 2, &hIMUTask);

    vTaskStartScheduler();

    return 0;
}

