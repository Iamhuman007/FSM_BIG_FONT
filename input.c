#include "input.h"
#include<msp430.h>
// #include "i2c.h"

#include "msp430g2553.h"
#include<stdint.h>
#include <stddef.h>
// #include <string.h>
// #include <stdbool.h>

void ADCinit(){
    // Configure ADC
    ADC10CTL1 = INCH_5;         // Input from A5 (P1.5)
    ADC10CTL0 = SREF_1 + REFON + REF2_5V + ADC10SHT_3 + ADC10ON; // V+ = AVcc, V- = AVss, 64 x ADC10CLKs
    __delay_cycles(30000); 


    // Configure P1.5 as analog input (A5)
    P1DIR &= ~ANALOG_IP;             // Make P1.5 input (optional)
    P1SEL |= ANALOG_IP;              // Select analog function for P1.5

}

void LEDinit(){
    // Set P3.2 P3.3 and P3.0 as outputs
    P3DIR |= BIT2 + BIT0 + BIT3;
    P3OUT = 0xFF;  // LED OFF
}

void digital_init(){
      // configure p1.2 as digital input
    // // Set P1.2 as input with pull-up resistor and interrupt
    // P1DIR &= ~DIGITAL_IP;                 // Input
    // P1REN |= DIGITAL_IP;                  // Enable pull-up/down
    // P1OUT |= DIGITAL_IP;                  // Pull-up
    // P1IE  |= DIGITAL_IP;                  // Interrupt enable
    // P1IES |= DIGITAL_IP;                  // High-to-low edge
    // P1IFG &= ~DIGITAL_IP;                 // Clear flag

    // // Set P1.2 as input with pull-down resistor and interrupt. THIS IS FOR RED BUTTON
    P1DIR &= ~DIGITAL_IP;                 // Input
    P1REN |= DIGITAL_IP;                  // Enable pull-up/down
    P1OUT &=~DIGITAL_IP;                  // Pull-down
    P1IE  |= DIGITAL_IP;                  // Interrupt enable
    P1IES &=~DIGITAL_IP;                  // low-to-high edge
    P1IFG &= ~DIGITAL_IP;                 // Clear flag

}

