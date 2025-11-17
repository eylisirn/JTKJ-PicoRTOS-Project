#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tkjhat/sdk.h"

// --- Pin configuration ---
#define LED_PIN     25

// --- Shared flags for buttons ---
volatile bool button1_pressed_flag = false;
volatile bool button2_pressed_flag = false;

// --- Morse buffer ---
#define MORSE_BUFFER_SIZE 64
char morse_buffer[MORSE_BUFFER_SIZE];
uint8_t morse_index = 0;

// --- ISR for both buttons ---
void button_isr(uint gpio, uint32_t events) {
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
    }
    else {
        printf("Virhe! IMU-sensoria ei voitu alustaa!\n");
    }

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Clear Morse buffer
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

            // --- Update display ---
            clear_display();
            if (morse_index > 0) {
                write_text(morse_buffer);
            }
            else {
                write_text(" ");
            }
        }

        // --- Handle button 1: Add current symbol to buffer ---
        if (button1_pressed_flag && symbol != '\0') {
            button1_pressed_flag = false;

            // Append symbol to buffer
            if (morse_index < MORSE_BUFFER_SIZE - 1) {
                morse_buffer[morse_index++] = symbol;
                morse_buffer[morse_index] = '\0';
            }

            // Flash LED to indicate symbol added
            gpio_put(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(150));
            gpio_put(LED_PIN, 0);
        }

        // --- Handle button 2: Add a single space ---
        if (button2_pressed_flag) {
            button2_pressed_flag = false;

            if (morse_index < MORSE_BUFFER_SIZE - 1) {
                morse_buffer[morse_index++] = ' ';
                morse_buffer[morse_index] = '\0';
            }

            // Flash LED to indicate space added
            gpio_put(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN, 0);
        }

        // --- Check for two consecutive spaces at end to send message ---
        if (morse_index >= 2 &&
            morse_buffer[morse_index - 1] == ' ' &&
            morse_buffer[morse_index - 2] == ' ') {

            // Send the message excluding the two trailing spaces
            printf("%s\n", morse_index - 2, morse_buffer);
            stdio_flush();
            taskYIELD(); // let USB driver run

            // Optional LED blink to indicate send
            gpio_put(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_put(LED_PIN, 0);

            // Clear buffer
            morse_index = 0;
            morse_buffer[0] = '\0';
        }
    }
}

// --- Main entry point ---
int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }

    printf("USB yhdistetty.\n");

    // --- Configure buttons ---
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);
    gpio_pull_up(BUTTON1);  // active-low
    gpio_init(BUTTON2);
    gpio_set_dir(BUTTON2, GPIO_IN);
    gpio_pull_up(BUTTON2);

    // Attach interrupts
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &button_isr);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_FALL, true, &button_isr);

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

    // --- Start IMU FreeRTOS task ---
    TaskHandle_t hIMUTask = NULL;
    xTaskCreate(imu_task, "IMUTask", 2048, NULL, 2, &hIMUTask);

    vTaskStartScheduler();

    return 0;
}
