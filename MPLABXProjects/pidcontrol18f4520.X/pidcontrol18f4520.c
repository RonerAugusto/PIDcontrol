/*
 * File:   pidcontrol18f4520.c
 * Author: Roner Augusto Benites
 *
 * Created on 29 de Agosto de 2017, 23:32
 */


#define _XTAL_FREQ 12000000
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "configfuze.h"




/*definitions*/

#define setPoint 100 ;  //set point sera 100 rpm


//prototip function
char readEncoder();
void lcdInit();
void lcdCmd(char cmd);
void lcdSend(char level, char data);
void interrupt usr();
void putch();
void setRpm(float x);
void lcdPositionxy(char x, char y);
void pidControl();
void lcdWrite(char* str);



/*hardware map*/




/*global variable*/
char buf[16];
int cycleCounter = 0x00; //100 in decimal maxima 0x64
unsigned int onesecond = 0x00;
unsigned int countDorRA0 = 0x00;
unsigned int countDorRA1 = 0x01;
unsigned char counter = 0x00;
float frequency = 0x00;
unsigned char aquartersencond = 0x00;

/*main code*/

void main(void) {

    // CMCON = 0x07; //desable comparator
    ADCON1 = 0x0f; // desable channel A/D


    TRISAbits.TRISA0 = 0x01; // RA2 AND RA3
    TRISAbits.TRISA1 = 0x01; // like  input
    TRISCbits.TRISC2 = 0x00; // like output
    
    
    PORTAbits.RA0 = 0x00; // start in zero volts
    PORTAbits.RA1 = 0x00; //
    PORTCbits.RC2 = 0x00;



    /*overflow timerzero = cycle machine*prescale*bitstimerzero
     overflow timerzerzo = 333E-9 * 32 * 2E8 = (2.7ms)   freq = 366 hz*/

    T0CON = 0xc4; //disable pull up  and prescale 1:4
    INTCONbits.GIE = 0x01; // enable glogal interrupt
    INTCONbits.PEIE = 0x01; // enable pheripherial interrupt
    INTCONbits.TMR0IE = 0x01; // enable timer zero interrupt
    TMR0 = 0x00; // timer zero start in zero
    ei(); //interrupts in general


    /*Configuration     PWM 
     period=(PR2 + 1) * machinecycle* preescale of timer02
     period = 256 * 333E-9 * 16
     period = 1.3 ms   
     */

    PR2 = 0xff; //load 255 to register
    T2CON = 0x06; // 00000111 turn on Timer02 and pre escale 1:16
    CCP1CON = 0x0C; // config to pwm mode 
    CCPR1L = 0x00; // pwm start in zero





    lcdInit();


    while (1) {


        lcdPositionxy(1, 3);
        printf("Posicao = %d ", counter);
        lcdPositionxy(2, 3); // lcdSend(0, 0xC3); // put cursor in line 2
        sprintf(buf, "rpm =%0.2f", frequency);
        lcdWrite(buf);



        __delay_ms(1);




    }
}

void pidControl() {

    /*time base equal one seconde Td=Kd/1   Ti=ki/1  because that isn't necessary  make Ti or Td*/

    /*set point is 100 rpm */
    static float upwm = 0x00; // function error 
    static float u1 = 0x00;
    static float ek = 0x00; //   erro current
    static float ek1 = 0x00; //   one step ago
    static float ek2 = 0x00; //    two ste ago


    ek = setPoint - frequency;

    static float const kp = 15;    //10
    static float const Ki = 2.0002;  // 0.3
    static float const kd = 0.0001;

    upwm = u1 + ek * kp + ek * kd + ek * Ki + ek1 * Ki - ek1 * kp - 2 * kd + ek2*kd; // function error 

    if (upwm >= 400) {

        setRpm(150); //more rotation possible
    }


    if (upwm <= 0) {


        setRpm(0); // stop rotarion  
    }



    setRpm(upwm);


    ek2 = ek1;
    ek1 = ek;
    u1 = upwm;







}

void setRpm(float x) {


    /*  PWM Duty Cycle = (CCPR X L:CCP X CON<5:4>) ?T OSC ? (TMR2 Prescale Value)
     *  PWM Duty Cycle = ccpr1L. * 83E-9 * 16;
     * PWM(IN %)     = [(CCPR1L * T osc * tmr2 prescale)/P ]* 100%   P is period  pwm 
     * PWM(IN %)     = [(CCPR1L *  83E-9 * 16) / 1.3E-3] * 100 
     * 
     * well making calcs and isolating the variable ccpr1l ...we'll have the value of ccpr1l
     * 
     * 
     * ccpr1l =[(dutycycle/100)*T /(Tosc * prescale)]
     * ccpr1l = 256  rpm= 400
     *  make an interpolation ...and  know the value of ccpr1l 
     */


    float yvalue = (x * 256) / 400;


    CCPR1L = (char) yvalue;
}










//functions in general

void putch(char c) {

    lcdSend(1, c);

}

void lcdInit() {

    char i;

    TRISD = 0x00; //  portb like output
    PORTD = 0x00;

    TRISEbits.TRISE0 = 0x00; // PIN RS
    TRISEbits.TRISE1 = 0x00; //PIN E
    PORTEbits.RE0 = 0x00; // RS
    PORTEbits.RE1 = 0x00;

    __delay_ms(1);



    for (i = 0; i <= 2; i++) {

        lcdCmd(0x30); //send three times 0x30

        __delay_ms(1);
    }

    lcdCmd(0x02); //lcd will work with 4 bits
    __delay_us(40);
    lcdCmd(0x28); // 4 bits comunication , display 2 row and matrix 7x5
    __delay_us(40);
    lcdCmd(0x01); //clear screen
    __delay_ms(1);
    lcdCmd(0x0C); //turn on display without cursor
    __delay_us(40);
    lcdCmd(0x06); // cursor goes to the right after the new character
    __delay_us(40);




}

void lcdCmd(char cmd) {

    PORTD = cmd & 0xf0;
    //
    //    PORTDbits.RD4 = setBit(cmd, 0b00010000);
    //    PORTDbits.RD5 = setBit(cmd, 0b00100000);
    //    PORTDbits.RD6 = setBit(cmd, 0b01000000);
    //    PORTDbits.RD7 = setBit(cmd, 0b10000000);

    PORTEbits.RE1 = 0x01; // PORT E display
    PORTEbits.RE1 = 0x00; //

    __delay_ms(1);


    PORTD = (cmd << 4) & 0xf0;


    PORTEbits.RE1 = 0x01; // PORT E display
    PORTEbits.RE1 = 0x00; //





}

void lcdSend(char level, char data) {

    PORTEbits.RE0 = level; //enable comand write
    __delay_us(100);
    PORTEbits.RE1 = 0x00; // enable write E lcd
    lcdCmd(data);





}

void lcdWrite(char *str) {



    unsigned char tamBuffer = 16; // control maximo size 16


    while (tamBuffer--) { // while tamBuffer > 0 

        lcdSend(1, *str); //  write character


        str++;




    }
}

void lcdPositionxy(char x, char y) {

    char address;

    if (x != 1)

        address = 0xc0; // put in second line

    else

        address = 0x80; // put in firt line

    address = (address + y) - 1;

    lcdSend(0, address);







}

char readEncoder() {

    static unsigned char states[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0}; // -1,0,1 possibles value
    static unsigned char oldposition = 0;


    // char doubt = 0b00000010;
    oldposition <<= 2;
    oldposition |= PORTA & 0x03; // 0000 0011
    return states[oldposition & 0x0f]; // 0000 1111



}

/*each 2.7 ms read bits in RA0 and RA1   it give a good resolution for 150 rpm the encoder*/

void interrupt usr() {


    /*there was interrupt timer zero*/

    if (INTCONbits.TMR0IF) {

        INTCONbits.TMR0IF = 0x00; //clear flag
        TMR0 = 0x00; // reload value timer zero

        onesecond++;
        

        char dados = readEncoder(); //dates off encoder



        if (dados) {

            counter += dados; // there is data increment

            cycleCounter++;


            if (PORTAbits.RA0) {

                countDorRA0++;

            }
            if (PORTAbits.RA1) {


                countDorRA1++;
            }


        }










        /*each one second come in here end give us the frequency in Hz*/


        if (onesecond == 370) {// 2.7E-3 *  370= 1 Second   





            /*here is necessary to know who will give me pulse por revolution soma or cycleCounter*/

            int soma = (countDorRA0 + countDorRA1);

            frequency = ((float) soma / 100)*60; // hz  ....100 pulse per revolution


            
            
            pidControl();   // function pid 
            
            
             /*reload variable*/
            onesecond = 0x00; // clear
            cycleCounter = 0x00; //clear
            countDorRA0 = 0x00;
            countDorRA1 = 0x00;
            soma = 0x00;






        }


        



    }


}






