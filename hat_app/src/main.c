#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tkjhat/sdk.h"

// Button flägit - Otto
volatile bool button1_pressed_flag = false;
volatile bool button2_pressed_flag = false;

// Morse bufferi - Otto 
// (AI2) katso tekoäly.txt - osa vastauksen koodista hyodynnettiin myös nappien toiminnoissa
#define MORSE_BUFFER_SIZE 64
char morse_buffer[MORSE_BUFFER_SIZE];
uint8_t morse_index = 0;

// Uusi flägi viestin lähetykselle - Yhteinen
volatile bool message_ready = false;

// Interruptit napeille - Eemeli, Otto
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

// IMU Sensori — pohjattu imu_hat_ex tiedostosta - Eemeli
void imu_task(void* pvParameters) {
    (void)pvParameters;

    float ax, ay, az, gx, gy, gz, t;

    // Sensorin käynnistys - Eemeli
    if (init_ICM42670() == 0) {
        ICM42670_enable_accel_gyro_ln_mode();
        ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
    }
    else {
        printf("IMU-sensori ei käynnistynyt.\n");
    }

    morse_index = 0;
    morse_buffer[0] = '\0';

    while (1) {
        char symbol = '\0';

        /// Päätetään asento, valitaan kirjain - Eemeli
        /// (AI-1) katso tekoäly.txt — vastausta on ajan kanssa muokattu reilusti
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

            // Päivitä LCD-näyttö - Otto
            clear_display();
            if (morse_index > 0) {
                write_text(morse_buffer);
            }
            else {
                write_text(" ");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));

        // Napit - Otto
        // Nappi 1 eli lisää merkki (- tai .)
        if (button1_pressed_flag && symbol != '\0') {
            button1_pressed_flag = false;

            if (!message_ready && morse_index < MORSE_BUFFER_SIZE - 1) {
                morse_buffer[morse_index++] = symbol;
                morse_buffer[morse_index] = '\0';
            }
        }

        // Nappi 2 eli lisää tyhjä väli
        if (button2_pressed_flag) {
            button2_pressed_flag = false;

            if (!message_ready && morse_index < MORSE_BUFFER_SIZE - 1) {
                morse_buffer[morse_index++] = ' ';
                morse_buffer[morse_index] = '\0';
            }
        }

        // Tarkista loppuuko bufferi kolmeen väliin - Yhteinen
        if (!message_ready && morse_index >= 3) {
            if (morse_buffer[morse_index - 1] == ' ' &&
                morse_buffer[morse_index - 2] == ' ' &&
                morse_buffer[morse_index - 3] == ' ') {

                if (morse_index > 3) {
                    morse_buffer[morse_index - 3] = '\0';
                    message_ready = true;
                }
            }
        }
    }
}

// Morse-viestin lähetys taski - Antti
void morse_task(void* pvParameters) {
    (void)pvParameters;

    while (1) {

        if (message_ready) {

            printf("%s  \n", morse_buffer);
            stdio_flush();

            // Tyhjennä bufferi
            morse_index = 0;
            morse_buffer[0] = '\0';

            // Vapauta tila seuraavaa viestiä varten
            message_ready = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Main - Yhteinen
int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }

    printf("USB yhdistetty.\n");

    //  Nappi konfiguraatio - Antti, Otto
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);
    gpio_pull_up(BUTTON1);

    gpio_init(BUTTON2);
    gpio_set_dir(BUTTON2, GPIO_IN);
    gpio_pull_up(BUTTON2);

    // Yhdistä interruptit - Otto
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &button_interrupt);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_FALL, true, &button_interrupt);

    // Käynnistä I2C - Eemeli/Yhteinen
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

    // FreeRTOS taskit - Eemeli/Yhteinen
    TaskHandle_t hIMUTask = NULL;
    TaskHandle_t hMorseTask = NULL;

    xTaskCreate(imu_task, "IMU", 2048, NULL, 2, &hIMUTask);
    xTaskCreate(morse_task, "MORSE", 1024, NULL, 1, &hMorseTask);

    vTaskStartScheduler();

    return 0;
}
