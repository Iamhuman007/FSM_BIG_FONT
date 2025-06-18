// #include <cstdint>


#include <msp430.h>
#include <lib/i2c/i2c.h>
#include <lib/oled/ssd1306.h>

#include "input.h"
#include "ADC.h"
#include "intrinsics.h"
#include "msp430g2553.h"
#include "sys/cdefs.h"
#include <signal.h>
#include <stdint.h>
#include <stddef.h>

// #define single_press 1000
#define double_press 2400



typedef enum{
    STATE_MAIN_MENU,
    STATE_CHARGING_100,
    STATE_CHARGING_50,
    STATE_DISCHARGING_50
}state;

state CURRENT_STATE=STATE_MAIN_MENU, PREVIOUS_STATE=STATE_DISCHARGING_50;



// input fn variables
volatile int timer_count = 0;

volatile int current_option=-1;
volatile int task_A = 0, task_B = 0; 
volatile long wait_time=50;
uint8_t r=0;




const char* menu[]={"1  CH_100","2   CH_50","3   DISCH"};

void Exit(){
   
               if(task_A){
                draw12x16Str(10,47,"EXIT", 0);
                r++;
                task_B=0;
                task_A=0;
                
                }
               else if(task_B&&r){
                    task_B=0;
                
                    r=0;
                    CURRENT_STATE=STATE_MAIN_MENU;
                    
                }

}
void display_screen( int current){
    // int i;
    // for(i=0;i<opt_no;i++){
    //     if(i==current){
    //         draw5x7Str(20, 8*i+25,menu[i], 0);
    //     }
    //     else{
    //         draw5x7Str(20, 8*i+25,menu[i], 1);
    //     }
    // }
    draw12x16Str(0,35,"          ", 1);
    draw5x7Str(120, 35, " ", 1);
    draw12x16Str(0,46,"          ", 1);
    draw5x7Str(120, 46, " ", 1);   
    draw12x16Str(10,40, menu[current], 1);
}



   

void main(void) {
    
    
    
   // Configure WDT in interval timer mode
    WDTCTL = WDTPW + WDTTMSEL + WDTCNTCL + WDTSSEL + WDTIS0 ;

   // Set clock to 8MHz
    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL = CALDCO_8MHZ;

    BCSCTL3 |= LFXT1S_2;  // ACLK = VLO (~12kHz)
    UCB0BR0 = 20;           // set UCB0BR0 to 20, for fSCL to 400kHz i2c
    UCB0BR1 = 0;            //      if needed. 800kHz might be fine too

    IFG1 &= ~WDTIFG;
    IE1 |= WDTIE;

    // output pins config


    // Set directions: P1.1, P1.3, P1.4 as outputs (discharge, main, charge)
    P1DIR |= BIT1 | BIT3 | BIT4;// main switch , dischargin , charging

    P1OUT = 0x00;  // All P1 outputs low
    P1OUT |= BIT3;       // turn OFF discharge (P1.3 high) 
    P1OUT |=BIT1;    // MAIN SWITCH
    // P1OUT |=BIT4; // CHARGING ON

    
    ADCinit();// initializing ADC
    digital_init();// conf pin 1.2 as input
    LEDinit();

    // Timer: ACLK (~32.768kHz), up mode, stop for now// 
    // TA0CCTL0 = CCIE;
    TA0CCR0 = 16384;                // 0.5 sec
    TA0CTL = TASSEL_1 | MC_0 | ID0;       // ACLK, stop mode

    // Timer: 1A
    TA1CCTL0=CCIE;
    TA1CCR0=12000-1;
    TA1CTL=TASSEL1 |MC_0 |ID0;

    
    

    __enable_interrupt();           // Enable global interrupts

    char c[]="V:";
    
    // size_t Inv=0;
    _delay_cycles(50000);
    i2c_init();
    ssd1306_init();
    ssd1306_command(SSD1306_SETCONTRAST);                               // 0x81
    ssd1306_command(0xFF);
    ssd1306_command(SSD1306_DISPLAYON);
    ssd1306_clearDisplay();
    draw12x16Str(4,10,"__________", 1);
    draw5x7Str(120, 18, "__", 1); 
    draw5x7Str(120, 19, "__", 1);
    draw12x16Str(50,1, c, 1);
    

    
    
    while (1) {
      
        display_voltage();
        __low_power_mode_3();    
        switch(CURRENT_STATE){
            case STATE_MAIN_MENU:
                if(PREVIOUS_STATE!=CURRENT_STATE){
                    draw12x16Str(0,35,"          ", 1);
                    draw5x7Str(120, 35, " ", 1);
                    draw12x16Str(0,46,"          ", 1);
                    draw5x7Str(120, 46, " ", 1);                    
                    draw12x16Str(10,40,"Press_Btn", 1);
                    PREVIOUS_STATE=CURRENT_STATE;
                    }
                if(task_A){
                    current_option=(current_option+1)%opt_no;
                    display_screen(current_option);
                    task_A=0;
                    }
                else if(task_B){
                    CURRENT_STATE=(state)(current_option+1);
                    task_B=0;
                    }
        
                break;

            case STATE_CHARGING_100:
                if(PREVIOUS_STATE!=CURRENT_STATE){
                    P3OUT&=~BIT3;    // BLUE LED  ON
                    P1OUT|=BIT4;// CHARGING ON
                    draw12x16Str(0,33,"          ", 1);
                    draw5x7Str(120, 33, " ", 1);
                    draw12x16Str(0,46,"          ", 1);
                    draw5x7Str(120, 46, " ", 1);    
                    draw5x7Str(10, 33, "Charging to 100%", 1);
                    draw12x16Str(10,47,"EXIT", 1);
                    PREVIOUS_STATE=CURRENT_STATE;
                }

                Exit();// checks for button press to exit
                if(CURRENT_STATE==STATE_MAIN_MENU){
                    P3OUT|=BIT3;// BLUE LED OFF
                    P1OUT&=~BIT4; // CHARGING OFF
                }

                if(ADC10MEM>835){
                        //stop charging
                        P1OUT&=~BIT4; // CHARGING OFF
                        P3OUT|=BIT3;  // BLUE LED OFF
                        //start timer 
                        TA1CTL|=MC_1;
                        draw12x16Str(5,1,"2DY", 1);
                        CURRENT_STATE=STATE_MAIN_MENU;

                    }
                break;


            case STATE_CHARGING_50:
                if(PREVIOUS_STATE!=CURRENT_STATE){
                    P3OUT&=~BIT3;    // BLUE LED  ON
                    P1OUT|=BIT4;// CHARGING ON
                    draw12x16Str(0,33,"          ", 1);
                    draw5x7Str(120, 33, " ", 1);
                    draw12x16Str(0,46,"          ", 1);
                    draw5x7Str(120, 46, " ", 1);                     
                    draw5x7Str(10, 33, "Charging to 50%", 1);
                    draw12x16Str(10,47,"EXIT", 1);
                    PREVIOUS_STATE=CURRENT_STATE;
                }

                Exit();// checks for button press to exit
                if(CURRENT_STATE==STATE_MAIN_MENU){
                    P3OUT|=BIT3;// BLUE LED OFF
                    P1OUT&=~BIT4; // CHARGING OFF
                }

                if(ADC10MEM>835){
                    //stop charging
                    P1OUT&=~BIT4; // CHARGING OFF
                    P3OUT|=BIT3;  // BLUE LED OFF
                    CURRENT_STATE=STATE_MAIN_MENU;
                    // stop the microcontroller
                }

                break;


            case STATE_DISCHARGING_50:       // OPTION 3
                if(PREVIOUS_STATE!=CURRENT_STATE){
                    P3OUT&=~BIT0;    // RED LED  ON
                    P1OUT&=~BIT3;    // DISCHARING ON
                    draw12x16Str(0,33,"          ", 1);
                    draw5x7Str(120, 33, " ", 1);
                    draw12x16Str(0,46,"          ", 1);
                    draw5x7Str(120, 46, " ", 1);  
                    draw5x7Str(5, 33, "Discharging to 50%", 1);
                    draw12x16Str(10,47,"EXIT", 1);
                    PREVIOUS_STATE=CURRENT_STATE;
                }

                Exit();// checks for button press to exit
                if(CURRENT_STATE==STATE_MAIN_MENU){
                    P3OUT|=BIT0; // RED LED OFF
                    P1OUT|=BIT3;// DISCHARING OFF
                }

                if(ADC10MEM>835){
                    // stop discharging
                    P1OUT|=BIT3;// DISCHARING OFF
                    P3OUT|=BIT0;
                    CURRENT_STATE=STATE_MAIN_MENU;
                }
                break;             
        

        }
    }





        
        
        
        
    


    
        
        
}


#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR(void) {
           
         __low_power_mode_off_on_exit();  // Wake main loop
}

// // Port 1 ISR (Button)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {
if(!(P1IES&BIT2)){
    P1IES|=BIT2;    // high to low transistion
    // P1IES&=~BIT2;// low to high transition
    
    TA0R=0;      // reset timer
    TA0CTL|=MC_1;// up mode
    P1IFG&=~BIT2;// clearing flag
    
}
else if(!(P1IES&BIT2)){
      timer_count=TA0R;
      TA0CTL&= ~MC_3;   // stop timer
      
      P1IES&=~BIT2;// low to high transition
    // P1IES|=BIT2;    // high to low transistion

      if(timer_count>=double_press){
        task_B=1;
      }
      else //if(timer_count<=single_press){
        task_A=1;
      //}
      P1IFG&=~BIT2;// clearing flag
      __low_power_mode_off_on_exit();  // Wake main loop
}
    
    
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void timer1(void){
      
   if(wait_time==0){
    wait_time=0;
    CURRENT_STATE=STATE_DISCHARGING_50;
    TA1CTL&=~MC_3;
    draw12x16Str(5,5,"   ", 1);
                    

   }
   else{

    wait_time--;

   }

}






