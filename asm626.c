#define SOLID true
#pragma GCC optimize ("O0")
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/divider.h"
#include "CMT.h"
//#include "hardware/pio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "asm626.h"
extern unsigned int TIMER1, TIMER2, TIMER3, TIMER4, TIMER5, TIMER6, TIMER7, TIMER8, TIMER9, TIMER10;
// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 9600

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// GPIO defines
// Example uses GPIO 2
#define GPIO 2


#define IR_RX_PIN 10


static inline uint8_t ir_level(void)
{
    return gpio_get(IR_RX_PIN) == 0; // LOW = IRあり = 1
}

int read_manchester_20bit(uint32_t *out)
{
    uint32_t data = 0;

        for (int i = 0; i < 20; i++) {
        uint8_t first  = ir_level();
        sleep_us(500);
        uint8_t second = ir_level();

        if (first == 0 && second == 1) {
            data = (data << 1) | 0;
        } else if (first == 1 && second == 0) {
            data = (data << 1) | 1;
        } else {
            *out = data;
            return 0;
        }

        sleep_us(500); // 次bit前半中央へ
    }

    *out = data & 0xFFFFF;
    return 1;
}

uint8_t irBit(void)
{
    // OSRB38C9AAは反転出力
    if (gpio_get(IR_RX_PIN) == 0)  // LOW = IRあり
        return(1);
    else
        return (0); // bit後半で反転しているか確認
}


int waitIRStart1(void)
{
    absolute_time_t t0;
    while (gpio_get(IR_RX_PIN))
        tight_loop_contents();
    return 1;
}

int waitIRStart(void)
{
    absolute_time_t t0;

    //====================================================
    // 1. 開始後20ms以内にLOWが来なければタイムアウト
    //====================================================
    t0 = get_absolute_time();

    while (gpio_get(IR_RX_PIN)) {   // HIGHの間待つ
        if (absolute_time_diff_us(t0, get_absolute_time()) >= 20000)
            return 0;
    }

    //====================================================
    // 2. LOW後、10ms以上連続HIGHにならなければタイムアウト
    //====================================================
    while (1) {

        // HIGH待ち
        while (gpio_get(IR_RX_PIN) == 0) {
            if (absolute_time_diff_us(t0, get_absolute_time()) >= 50000)
                return 0;
        }

        // HIGH開始時刻
        absolute_time_t th = get_absolute_time();

        // HIGH継続監視
        while (gpio_get(IR_RX_PIN)) {

            // 10ms以上HIGHなら次のLOW待ちへ
            if (absolute_time_diff_us(th, get_absolute_time()) >= 10000)
                goto WAIT_LOW;

            // 全体タイムアウト
            if (absolute_time_diff_us(t0, get_absolute_time()) >= 50000)
                return 0;
        }

        // HIGHが10ms未満で終わったのでやり直し
    }

WAIT_LOW:

    //====================================================
    // 3. 10ms以上HIGHの後、LOWを検出したら成功
    //====================================================
    while (gpio_get(IR_RX_PIN))
        tight_loop_contents();
    return 1;
}


//#define check_PIN 11 relaybit
#define check_PIN 4  /* LED bit1 */
uint8_t p[20];
uint32_t readIR20()

{
    uint32_t d = 0;

    for (int i = 0; i < 15; i++) {
        int first, second;
        sleep_us(500);
        p[i]=irBit();
        d <<=1;
        if (p[i]) 
            d |=1;
        gpio_put(check_PIN, 1);                 
        gpio_put(check_PIN, 0);
        sleep_us(500);
    }

    return(d & 0xFFFFF);
}
uint8_t calcChecksum4(uint16_t data)
{
    return ((data >> 12) & 0xF)
         ^ ((data >> 8)  & 0xF)
         ^ ((data >> 4)  & 0xF)
         ^ ( data        & 0xF);
}


int main()
{
    stdio_init_all();
sleep_ms(2000);

printf("START\n");
    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    

    // GPIO initialisation.
    // We will make this GPIO an input, and pull it up by default
    
    gpio_init_mask(0x7f0);              //
   	gpio_set_dir_masked(0x7f0,0xf2);  //gpio 4-7 LED out
    gpio_set_dir_in_masked(0x30d);  // input gpio8,9  gpio 1

    // Example of using the HW divider. The pico_divider library provides a more user friendly set of APIs 
    // over the divider (and support for 64 bit divides), and of course by default regular C language integer
    // divisions are redirected thru that library, meaning you can just use C level `/` and `%` operators and
    // gain the benefits of the fast hardware divider.
    int32_t dividend = 123456;
    int32_t divisor = -321;
    // This is the recommended signed fast divider for general use.
    divmod_result_t result = hw_divider_divmod_s32(dividend, divisor);
    printf("%d/%d = %d remainder %d\n", dividend, divisor, to_quotient_s32(result), to_remainder_s32(result));
    // This is the recommended unsigned fast divider for general use.
    int32_t udividend = 123456;
    int32_t udivisor = 321;
    divmod_result_t uresult = hw_divider_divmod_u32(udividend, udivisor);
    printf("%d/%d = %d remainder %d\n", udividend, udivisor, to_quotient_u32(uresult), to_remainder_u32(uresult));
/////////////
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    char hour;

    const uint RLY_PIN = 11;
    #define SETSW 0x100  //GPIO 8  manual time set
    #define LED 0xf0     //output for led port
    volatile uint32_t led, sw, sw1, sw2;
    // Start on Friday 5th of June 2020 15:45:00
    datetime_t t = {
        .year  = 2023,
        .month = 06,
        .day   = 28,
        .dotw  = 3, // 0 is Sunday, so 5 is Friday
        .hour  = 12,
        .min   = 57,
        .sec   = 00
    };

    rtc_init();
    gpio_init(RLY_PIN);
    gpio_set_dir(RLY_PIN, GPIO_OUT);

    if (watchdog_caused_reboot());
    else  //cold start
        rtc_set_datetime(&t);
    CMT0_initialize();
    watchdog_enable(5000, 1);

    TIMER1=100;
    // clk_sys is >2000x faster than clk_rtc, so datetime is not updated immediately when rtc_get_datetime() is called.
    // tbe delay is up to 3 RTC clock cycles (which is 64us with the default clock settings)
    sleep_us(64);


    // Print the time
    volatile uint32_t ir_data = 0;
    gpio_put_masked(LED,0xf0);  // led off
    TIMER4=20;  //LED enable timer 10sec
    while (true) {
        for(TIMER7 = 50,sw=0; TIMER7; ){
	  	    if(  sw != (sw1 = (gpio_get_all() & SETSW ))) { //20msec keyが持続
	            sw = sw1;
                TIMER7 = 50;
	        }
		    else if(sw==0)
		        break;
        }
        watchdog_update();  // WDT refresh
        rtc_get_datetime(&t);
        datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
        printf("\r%s      ", datetime_str);
        if (sw & SETSW){     ///  manual time set sw
            if(TIMER4){   // LED enable timer
                if((++t.hour) >=24)
                    t.hour=0;
                t.min=t.sec=0;
                rtc_set_datetime(&t);
            }
            TIMER4 =20;  ///LED enable time 10sec
        }
        if(t.hour == 6 & t.min ==03 &  t.sec <15)
#if SOLID
            gpio_put(RLY_PIN, 0);
#else
            gpio_put(RLY_PIN, 1);
#endif
        else
#if SOLID
            gpio_put(RLY_PIN, 1);
#else
            gpio_put(RLY_PIN, 0);
#endif
        if(t.hour > 12)
            led = (t.hour - 12)<<4;
        else
            led = t.hour <<4;

        if(TIMER4==0)  // LED enable time?
            led=0;
        gpio_put_masked(LED, led ^ 0xf0);
        
        if(t.hour>12 && (t.sec &1))
            gpio_put_masked(LED,0xf0);  // flic led
        for(TIMER7 = 100,sw=1; TIMER7; ){
	  	    if(  sw != (sw1 = (gpio_get_all() &  SETSW))) { //20msec keyが持続
	            sw = sw1;
	            TIMER7 = 100;
	        }
        }
 uint32_t c_hour, c_min;    
    if(waitIRStart()){
        ir_data=readIR20();
        c_hour=ir_data >>8 & 0x1f;
        c_min=ir_data & 0x3f;
        t.hour=c_hour;
        t.min=c_min;
        rtc_set_datetime(&t);
        TIMER4 = 20;   // Led display time 10sec
        __asm volatile("nop");   // ここにブレーク

    }
    }//////////////////
    puts("Hello, world!");
    return 0;
}


