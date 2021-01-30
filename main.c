/*
 * ZBD LCD driver demo for ZBD EPOP 900 devices
 *
 * Copyright 2021 Wenting Zhang <zephray@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <stdint.h>
#include <cc1110.h>
#include "cc1110-ext.h"

// SEG-only connection:
#define LCD_SEG_LP  P1_7
#define LCD_CP      P1_0
// COM-only connection:
#define LCD_COM_LP  P1_5
#define LCD_FLM     P2_2
// Common connection:
#define LCD_DISPON  P2_1
#define LCD_M       P1_6
// Data bus
#define LCD_DB      P0

// Power Control
#define LCD_LOGIC_PWR  P1_2
#define LCD_DCDC_PWR   P1_1

void delay(void) {
    NOP();
    NOP();
    NOP();
}

void delay_arb(int t) {
    volatile int x = t;
    while (x--);
}

void delay_long(void) {
    for (int x = 0; x < 1000; x++) {
        delay_arb(1000);
    }
}

void pwm_init() {
    // Tick freq / 1, interrupt disabled, free running mode
    T3CTL = T3CTL_DIV_1 | T3CTL_MODE_FREERUN;
    T3CCTL0 = T3C0_SET_CMP_UP_CLR_0 | (0x1 << 2);
    T3CC0 = 0x80;
    T3CCTL1 = T3C1_SET_CMP_UP_CLR_0 | (0x1 << 2);
    T3CC1 = 0x80;
    P1SEL |= (0x1 << 4) | (0x1 << 3); // Set P1_3, P1_4 to peripheral mode
    PERCFG &= ~PERCFG_T3CFG; // Timer 3 I/O at Alt 1 location
    P2SEL |= P2SEL_PRI2P1; // Timer 3 has priority over USART1
    T3CTL |= 0x10; // Enable Timer 3
}

void pwm_set_vh(uint8_t val) {
    T3CC0 = 0xff - val;
}

void pwm_set_vbias(uint8_t val) {
    T3CC1 = val;
}

void clear_screen(void) {
    pwm_set_vh(0x90);
    pwm_set_vbias(0xf0);
    // Wait for voltage to stable
    delay_arb(1000);

    LCD_DISPON = 1;

    for (int i = 0; i < 5; i++) {
        LCD_CP = 1;
        delay();
        LCD_CP = 0;
        delay();
    }
    for (int j = 0; j < 360/8; j++) {
        LCD_DB = 0xff;
        LCD_CP = 1;
        delay();
        LCD_CP = 0;
    }
    LCD_SEG_LP = 1;
    delay();
    LCD_SEG_LP = 0;
    delay();
    
    LCD_M = 1;
    LCD_FLM = 1;
    for (int i = 0; i < 484; i++) {
        delay_arb(3000);
        LCD_COM_LP = 1;
        delay();
        LCD_COM_LP = 0;
        delay();
        LCD_FLM = 0;
    }

    LCD_DISPON = 0;
    P1_4 = 1;
    delay_long();
}

void main(void)
{
    SLEEP &= ~SLEEP_OSC_PD;
    while(!(SLEEP & SLEEP_XOSC_S));

    CLKCON = CLKCON_OSC32 | CLKCON_OSC | TICKSPD_DIV_32 | CLKSPD_DIV_2;
    while (!(CLKCON & CLKCON_OSC));

    CLKCON = CLKCON_OSC32 | TICKSPD_DIV_1 | CLKSPD_DIV_1;



    P0 = 0x00;
    P1 = 0x00;
    P2 = 0x00;

    P0DIR = 0xff;
    P1DIR = 0xff;
    P2DIR = 0x04 | 0x02;

    LCD_DB = 0x00;
    LCD_LOGIC_PWR = 0;
    LCD_CP = 0;
    LCD_COM_LP = 0;
    LCD_M = 0;
    LCD_SEG_LP = 0;
    LCD_DISPON = 0;
    LCD_FLM = 0;

    pwm_init();

    // Enable LCD power
    LCD_LOGIC_PWR = 1; // Enable LCD logic power
    LCD_DCDC_PWR = 1; // Enable DCDC power supply

    clear_screen();

    pwm_set_vh(0x80);
    pwm_set_vbias(0x70);
    delay_arb(1000);

    LCD_FLM = 1;
    LCD_M = 0;
    LCD_COM_LP = 0;
    delay();
    LCD_DISPON = 1;

    for (int i = 0; i < 480; i++) {
        for (int i = 0; i < 5; i++) {
            LCD_CP = 1;
            delay();
            LCD_CP = 0;
            delay();
        }
        for (int j = 0; j < 360/8; j++) {
            if ((!(j & 0x4)) ^ (!(i & 0x20)))
                LCD_DB = 0xff;
            else
                LCD_DB = 0x00;
            
            LCD_CP = 1;
            delay();
            LCD_CP = 0;
        }

        if (i == 0) {
            LCD_SEG_LP = 1;
            delay();
            LCD_SEG_LP = 0;
        }
        else {
            LCD_COM_LP = 1;
            LCD_SEG_LP = 1;
            delay();
            LCD_COM_LP = 0;
            LCD_SEG_LP = 0;
            LCD_FLM = 0;
        }

        delay_arb(1500);
    }
    LCD_DISPON = 0;

    // Turn off everything
    delay_arb(1000);
    LCD_DCDC_PWR = 0; // Enable DCDC power supply
    LCD_LOGIC_PWR = 0; // Enable LCD logic power

    while(1);
}



