#include "ADC.h"
#include<msp430.h>
#include <lib/i2c/i2c.h>
#include <lib/oled/ssd1306.h>


// #include "intrinsics.h"
#include "msp430g2553.h"
#include<stdint.h>
#include <stddef.h>
#include <string.h>
// #include <stdbool.h>



char n[]={'0','0','0','0','0'};//eg 40.2




void int_to_chars(uint16_t number, char* n) {
    // Assumes number is like 428, for 42.8
    int hundreds = number / 100;          // 4
    int tens = (number / 10) % 10;        // 2
    int ones = number % 10;               // 8

    n[0] = '0' + hundreds;
    n[1] = '0' + tens;
    n[2] = '.';
    n[3] = '0' + ones;
    n[4] = '\0'; // Null-terminate
   
}

void converted(uint16_t ADCMEM) {// should take some arguments
    
    // float vref = 2.5; // Internal 2.5V reference
    float voltage;
    uint16_t volts;
    
    // Assuming you need to convert ADC value to voltage with reference
    voltage = (104.12 * (ADCMEM* vref / 1023.0) / 4.12);
    volts = (voltage * 10 + 0.5);  // 42.77 to 428
    
    // Convert voltage to characters
    int_to_chars(volts, n);
    
    
}

void display_voltage(){

        uint8_t i;
        uint16_t s=0;
        
        
        for(i=0;i<sample_no;i++){
            __delay_cycles(5000); 
            ADC10CTL0 |= ENC + ADC10SC; // Start conversion
            while (ADC10CTL1 & ADC10BUSY); // Wait for conversion
            s+=ADC10MEM;

        }
        s=s>>4;// divide by 16

        
         converted(s);
        //  draw12x16Str(0,0,"          ", 1);
        //  draw5x7Str(120, 35, " ", 1);
        //  draw12x16Str(0,15,"          ", 1);
        //  draw12x16Str(4,10,"__________", 1);
         draw12x16Str(5, 1,"V:", 1);
         draw12x16Str(29, 1, n, 1);
}

