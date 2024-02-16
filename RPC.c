/*******************************************************
This program was created by the
CodeWizardAVR V3.12 Advanced
Automatic Program Generator
© Copyright 1998-2014 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : RPC
Version : 1.00
Date    : 8/4/2019
Author  : Sajjadi
Company : ASA eTrading
Comments: 


Chip type               : ATmega32A
Program type            : Application
AVR Core Clock frequency: 11.059200 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 512
*******************************************************/

#asm
    .equ __w1_port=0x18
    .equ __w1_bit=2
#endasm

#include <mega32a.h>
#include <stdio.h>
#include <delay.h>
#include <string.h>
#include <stdlib.h>
#include <alcd.h>
#include <ctype.h>
#include <1wire.h>
#include <ds18b20.h>

#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE (1<<RXC)
#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR (1<<UPE)
#define DATA_OVERRUN (1<<DOR)

#define     PSW         PIND.7
#define     VL1         read_adc(7)
#define     VL2         read_adc(6)
#define     VL3         read_adc(5)
#define     REL1_OFF    PORTD.6 = 0
#define     REL1_ON     PORTD.6 = 1
#define     REL2_OFF    PORTD.5 = 0
#define     REL2_ON     PORTD.5 = 1
#define     REL3_OFF    PORTD.4 = 0
#define     REL3_ON     PORTD.4 = 1
#define     REL4_OFF    PORTD.2 = 0
#define     REL4_ON     PORTD.2 = 1
#define     REL5_OFF    PORTD.3 = 0
#define     REL5_ON     PORTD.3 = 1

#define     REL1        PORTD.6
#define     REL2        PORTD.5
#define     REL3        PORTD.4
#define     REL4        PORTD.2
#define     REL5        PORTD.3

#define     no_of_cols  320
#define     ADC_VREF_TYPE 0x40

// Declare your global variables here
float dc;

char r = 0;
char s = 0;
char t = 0;
char cr = 0;

int y = 0;
int col = 0;

bit pir1, por1 = 1;
bit pir2, por2 = 1;
bit pir3, por3 = 1;
bit received = 0;
bit start = 0;

char temp[5];
char sms[1024];
char vline1[8];
char vline2[8];
char vline3[8];
char sender[11];

unsigned int data = 0;
unsigned int t1, t2;
 
eeprom char dev1,dev2;
eeprom char L1,L2,L3;
eeprom int LD1,LD2,LD3;
//---------------------------------------------------------------
// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input)
{
    #asm("wdr")
    ADMUX=adc_input | (ADC_VREF_TYPE & 0xff);
    // Delay needed for the stabilization of the ADC input voltage
    delay_us(10);
    // Start the AD conversion
    ADCSRA|=0x40;
    // Wait for the AD conversion to complete
    while ((ADCSRA & 0x10)==0);
    ADCSRA|=0x10;
    return ADCW;
}
//---------------------------------------------------------------
// I/O Functions:
#pragma used+
//---------------------------------------------------------------
void putflash( char flash *str )
{
    char k;   #asm("wdr")
    while ( k = *str++ ) { putchar( k ); delay_ms(1); }
    #asm("wdr")   putchar( 13 );
}
//---------------------------------------------------------------
void putfl( char flash *str )
{
    char k;   #asm("wdr")
    while ( k = *str++ ) { putchar( k ); delay_ms(1); }
    #asm("wdr")
}
//---------------------------------------------------------------
void putreg( char *str )
{
    char k;   #asm("wdr")
    while ( k = *str++ ) putchar( k );
    #asm("wdr")
}
//---------------------------------------------------------------
int hex2int( char* Hex )
{
    int d;  int nDigitMult = 1;  int nResult = 0;
    #asm("wdr")
    for( d = 3 ; d >= 0 ; d-- )
    {
        char ch = Hex[ d ];
        if( '0' <= ch && ch <= '9' )
        nResult += (ch - '0') * nDigitMult;
        else if( 'a' <= ch && ch <= 'f' )
        nResult += (ch - 'a' + 10) * nDigitMult;
        else if( 'A' <= ch && ch <= 'F' )
        nResult += (ch - 'A' + 10) * nDigitMult;
        nDigitMult *= 16;
        #asm("wdr")
    }
    return nResult;
}
//---------------------------------------------------------------
void config( void )
{    
    delay_ms(200);   #asm("wdr")
    if ( PSW ) return;
    lcd_gotoxy(0,0);
    lcd_putsf( "Configuration is running" );
    #asm("wdr")                         delay_ms(500);            
    putflash( "AT" );                   delay_ms(500);
    putflash( "AT" );                   delay_ms(500);
    putflash( "AT" );                   delay_ms(500);
    putflash( "AT" );                   delay_ms(500);
    putflash( "AT" );                   delay_ms(500);
    putflash( "AT&F" );                 delay_ms(10000);   // Set all current parameters to manufacturer 
    putflash( "AT+IPR=9600" );          delay_ms(1000);    // Set fixed local rate
    putflash( "ATE0" );                 delay_ms(1000);    // Disable command echo
    putflash( "ATV0" );                 delay_ms(1000);    // Set result code format mode
    putflash( "AT+CMGF=1" );            delay_ms(1000);    // Select Text Mode SMS Message 
    putflash( "AT+CNMI=2,2,0,0,0" );    delay_ms(1000);    // Auto read of new massage
    putflash( "AT&W" );                 delay_ms(1000);    // Stores current configuration
    putflash( "AT+CMGD=1,4" );          delay_ms(5000);    // Delete All Messages  
    lcd_gotoxy(0,2); 
    lcd_putsf( "Configuration done successfully" ); 
    delay_ms(5000);                     lcd_clear();  
    #asm("wdr")
    #asm ( "jmp 0x00" )   
}
//---------------------------------------------------------------
// SMS Reception Function
void query( char c )
{
    #asm("wdr")
    if ( c == '+' ) start = 1;
    if ( (c == 13) && (start) ) cr++;
    if ( start ) 
    {
        sms[y] = c; 
        y++;
    } 
    if ( (cr >= 2 ) && (start) )
    { 
        received = 1;
        start = 0;
        cr = 0;
        y = 0;
    } 
}
//---------------------------------------------------------------
// SMS Extraction Function
void extract( void )
{
    int j = 0;  int k = 0;     
    char b[5];  char sm[67];   char matn[270];
    UCSRB=0x08; // Disable RX
    #asm("wdr")
    
    if( strncmpf( sms, "+CMT:", 5 ) == 0 )
    {
        for( j = 0 ; j <= 9 ; j++ ) sender[j] = sms[j+10];
        sender[10] = 0;   j = 0;
    }
    else goto DEL;
    do
    {
        sms[j] = 0;
        j++;
        #asm("wdr")        
    }
    while( sms[j] != 10 );
    j++;
    do
    {
        matn[k] = sms[j];
        sms[j] = 0;
        j++;    k++;
        #asm("wdr")    
    }
    while( sms[j] != 13 );
    matn[k] = 0;    k = 0;
    while( matn[k] )
    {
        for( j = 0 ; j <= 3 ; j++ )
        {
            b[j] = matn[k];
            k++;
        }
        sm[col] = hex2int( b ); 
        col++;
        #asm("wdr")
    }
    sm[col] = 0; 
        
    if( strncmpf( sm, "Status", col ) == 0 )
    {
        putflash("AT");   #asm("wdr")       delay_ms(300);
        putfl("AT+CMGS=\"0");               delay_ms(100);
        putreg(sender);                     delay_ms(100);
        putchar(34);      putchar(13);      delay_ms(999);                
        putfl( "Line 1: " );
        putreg( vline1 ); putchar( 'v' );   putchar( 13 );
        putfl( "Line 2: " );
        putreg( vline2 ); putchar( 'v' );   putchar( 13 );
        putfl( "Line 3: " );
        putreg( vline3 ); putchar( 'v' );   putchar( 13 );
        if( REL1 ) putfl( "L1 is on " );    else putfl( "L1 is off " );      putchar( 13 );
        if( REL2 ) putfl( "L2 is on " );    else putfl( "L2 is off " );      putchar( 13 );
        if( REL3 ) putfl( "L3 is on " );    else putfl( "L3 is off " );      putchar( 13 );
        if( REL4 ) putfl( "D1 is on " );    else putfl( "D1 is off " );      putchar( 13 );
        if( REL5 ) putfl( "D2 is on " );    else putfl( "D2 is off " );      putchar( 13 );
        putfl( "L1 downtimes: " );          delay_ms(200);   
        itoa( LD1 , b );    putreg( b );    delay_ms(200);   LD1 = 0;       putchar( 13 );
        putfl( "L2 downtimes: " );          delay_ms(200);   
        itoa( LD2 , b );    putreg( b );    delay_ms(200);   LD2 = 0;       putchar( 13 );
        putfl( "L3 downtimes: " );          delay_ms(200);   
        itoa( LD3 , b );    putreg( b );    delay_ms(200);   LD3 = 0;       putchar( 13 );
        putfl( "Temperature: " );           delay_ms(200);
        putreg( temp );     #asm("wdr")     delay_ms(200);
        putchar( ' ' );  putchar( 'C' );    delay_ms(200);
        putchar(26);                        delay_ms(500);
    }
    // Turn All Lines ON
    else if( strncmpf( sm, "Lines on", col ) == 0 )
    {
        #asm("wdr")
        L1 = 1;  L2 = 1;  L3 = 1;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "All lines will be turned on after 1 minute" );
        putchar(26);                delay_ms(500);  
    }
    // Turn All Lines OFF
    else if( strncmpf( sm, "Lines off", col ) == 0 )
    {
        #asm("wdr")
        REL1_OFF; REL2_OFF; REL3_OFF;
        L1 = 0;  L2 = 0;  L3 = 0;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "All lines were turned off" );
        putchar(26);                delay_ms(500);  
    }
    // Reboot All Lines
    else if( strncmpf( sm, "Lines reset", col ) == 0 )
    {
        #asm("wdr")
        REL1_OFF; REL2_OFF; REL3_OFF;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "All lines were rebooted" );
        putchar(26);                delay_ms(2000);
        REL1_ON; REL2_ON; REL3_ON;  
    } 
    // Turn Line 1 ON
    else if( strncmpf( sm, "L1 on", col ) == 0 )
    {
        #asm("wdr")
        L1 = 1;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Line 1 will be turned on after 1 minute" );
        putchar(26);                delay_ms(500);  
    }
    // Turn Line 1 OFF
    else if( strncmpf( sm, "L1 off", col ) == 0 )
    {
        #asm("wdr")
        REL1_OFF;   L1 = 0;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Line 1 was turned off" );
        putchar(26);                delay_ms(500);  
    } 
    // Reboot Line 1
    else if( strncmpf( sm, "L1 reset", col ) == 0 )
    {
        #asm("wdr")
        REL1_OFF;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);;
        putflash( "Line 1 was rebooted" );
        putchar(26);                delay_ms(2000);
        REL1_ON;  
    }
    // Turn Line 2 ON
    else if( strncmpf( sm, "L2 on", col ) == 0 )
    {
        #asm("wdr")
        L2 = 1;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Line 2 will be turned on after 1 minute" );
        putchar(26);                delay_ms(500);  
    }
    // Turn Line 2 OFF
    else if( strncmpf( sm, "L2 off", col ) == 0 )
    {
        #asm("wdr")
        REL2_OFF;   L2 = 0;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Line 2 was turned off" );
        putchar(26);                delay_ms(500);  
    }
    // Reboot Line 2
    else if( strncmpf( sm, "L2 reset", col ) == 0 )
    {
        #asm("wdr")
        REL2_OFF;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Line 2 was rebooted" );
        putchar(26);                delay_ms(2000);
        REL2_ON;  
    }
    // Turn Line 3 ON
    else if( strncmpf( sm, "L3 on", col ) == 0 )
    {
        #asm("wdr")
        L3 = 1;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Line 3 will be turned on after 1 minute" );
        putchar(26);                delay_ms(500);  
    }
    // Turn Line 3 OFF
    else if( strncmpf( sm, "L3 off", col ) == 0 )
    {
        #asm("wdr")
        REL3_OFF;   L3 = 0;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Line 3 was turned off" );
        putchar(26);                delay_ms(500);  
    }
    // Reboot Line 3
    else if( strncmpf( sm, "L3 reset", col ) == 0 )
    {
        #asm("wdr")
        REL3_OFF;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Line 3 was turned off" );
        putchar(26);                delay_ms(2000);
        REL3_ON;  
    }
    // Turn Device 1 ON
    else if( strncmpf( sm, "D1 on", col ) == 0 )
    {
        #asm("wdr")
        REL4_ON;                    dev1 = 1;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Device 1 was turned on" );
        putchar(26);                delay_ms(500);  
    }
    // Turn Device 1 OFF
    else if( strncmpf( sm, "D1 off", col ) == 0 )
    {
        #asm("wdr")
        REL4_OFF;                   dev1 = 0;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Device 1 was turned off" );
        putchar(26);                delay_ms(500);  
    }
    // Reboot Device 1 OFF
    else if( strncmpf( sm, "D1 reset", col ) == 0 )
    {
        #asm("wdr")
        REL4_ON;                    delay_ms(500);
        REL4_OFF;                   delay_ms(300);
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Device 1 was rebooted" );
        putchar(26);                delay_ms(500);  
    }
    // Turn Device 2 ON
    else if( strncmpf( sm, "D2 on", col ) == 0 )
    {
        #asm("wdr")
        REL5_ON;                    dev2 = 1;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Device 2 was turned on" );
        putchar(26);                delay_ms(500);  
    }
    // Turn Turn Device 2 OFF
    else if( strncmpf( sm, "D2 off", col ) == 0 )
    {
        #asm("wdr")
        REL5_OFF;                   dev2 = 0;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Device 2 was turned off" );
        putchar(26);                delay_ms(500);  
    }
    // Put Device 2 in Auto Mode 
    else if( strncmpf( sm, "D2 auto", col ) == 0 )
    {
        #asm("wdr")
        REL5_ON;                    dev2 = 2;
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Device 2 is in auto mode" );
        putchar(26);                delay_ms(500);  
    }
    // Shows Thhe Help
    else if( strncmpf( sm, "Help", col ) == 0 )
    {
        #asm("wdr")
        putflash("AT");             delay_ms(300);
        putfl("AT+CMGS=\"0");       delay_ms(100);
        putreg(sender);             delay_ms(100);
        putchar(34);  putchar(13);  delay_ms(999);
        putflash( "Status" );
        putflash( "Lines on" );
        putflash( "Lines off" );
        putflash( "Lines reset" );
        putflash( "L1 on" ); 
        putflash( "L1 off" ); 
        putflash( "L1 reset" ); 
        putflash( "L2 on" );  
        putflash( "L2 off" );  
        putflash( "L2 reset" );
        putflash( "L3 on" );  
        putflash( "L3 off" );    
        putflash( "L3 reset" );  
        putflash( "D1 on" );     
        putflash( "D1 off" );
        putflash( "D1 reset" );    
        putflash( "D2 on" ); 
        putflash( "D2 off" );
        putflash( "D2 auto" );     
        putflash( "Help" );         
        putchar(26);                delay_ms(500);  
    }
    DEL:
    #asm("wdr")    delay_ms(1000);
    putflash("AT+CMGD=1,1"); 
    delay_ms(1000);
    col = 0;    received = 0;       
    UCSRB=0x98; //Enable RX
}
//---------------------------------------------------------------
#pragma used-
//---------------------------------------------------------------
// External Interrupt 2 service routine
interrupt [EXT_INT2] void ext_int2_isr(void)
{
    #asm("wdr")
    if( MCUCSR > 0 )
    {
        MCUCSR = 0x00;
        t2 = TCNT1;
        TCNT1 = 0x0000;
    }
    else 
    {
        MCUCSR = 0x40;
        t1 = TCNT1;
        TCNT1 = 0x0000; 
    }
}
//---------------------------------------------------------------
// USART Receiver interrupt service routine
interrupt [USART_RXC] void usart_rx_isr(void)
{
    char status,data;
    status=UCSRA;
    data=UDR;
    if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0) query( data ); 
}
//---------------------------------------------------------------

void main(void)
{
// Declare your local variables here

// Input/Output Ports initialization
// Port A initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRA=(0<<DDA7) | (0<<DDA6) | (0<<DDA5) | (0<<DDA4) | (0<<DDA3) | (0<<DDA2) | (0<<DDA1) | (0<<DDA0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTA=(0<<PORTA7) | (0<<PORTA6) | (0<<PORTA5) | (0<<PORTA4) | (0<<PORTA3) | (0<<PORTA2) | (0<<PORTA1) | (0<<PORTA0);

// Port B initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=Out Bit3=Out Bit2=In Bit1=Out Bit0=Out 
DDRB=(0<<DDB7) | (0<<DDB6) | (0<<DDB5) | (1<<DDB4) | (1<<DDB3) | (0<<DDB2) | (1<<DDB1) | (1<<DDB0);
// State: Bit7=T Bit6=T Bit5=T Bit4=0 Bit3=0 Bit2=T Bit1=0 Bit0=0 
PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);

// Port C initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=Out 
DDRC=(0<<DDC7) | (0<<DDC6) | (0<<DDC5) | (0<<DDC4) | (0<<DDC3) | (0<<DDC2) | (0<<DDC1) | (1<<DDC0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=0 
PORTC=(0<<PORTC7) | (0<<PORTC6) | (0<<PORTC5) | (0<<PORTC4) | (0<<PORTC3) | (0<<PORTC2) | (0<<PORTC1) | (0<<PORTC0);

// Port D initialization
// Function: Bit7=In Bit6=Out Bit5=Out Bit4=Out Bit3=Out Bit2=Out Bit1=In Bit0=In 
DDRD=(0<<DDD7) | (1<<DDD6) | (1<<DDD5) | (1<<DDD4) | (1<<DDD3) | (1<<DDD2) | (0<<DDD1) | (0<<DDD0);
// State: Bit7=P Bit6=0 Bit5=0 Bit4=0 Bit3=0 Bit2=0 Bit1=T Bit0=T 
PORTD=(1<<PORTD7) | (0<<PORTD6) | (0<<PORTD5) | (0<<PORTD4) | (0<<PORTD3) | (0<<PORTD2) | (0<<PORTD1) | (0<<PORTD0);

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: Timer 0 Stopped
// Mode: Normal top=0xFF
// OC0 output: Disconnected
TCCR0=(0<<WGM00) | (0<<COM01) | (0<<COM00) | (0<<WGM01) | (0<<CS02) | (0<<CS01) | (0<<CS00);
TCNT0=0x00;
OCR0=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: Timer1 Stopped
// Mode: Normal top=0xFFFF
// OC1A output: Disconnected
// OC1B output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=(0<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (0<<WGM10);
TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (0<<CS11) | (0<<CS10);
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: Timer2 Stopped
// Mode: Normal top=0xFF
// OC2 output: Disconnected
ASSR=0<<AS2;
TCCR2=(0<<PWM2) | (0<<COM21) | (0<<COM20) | (0<<CTC2) | (0<<CS22) | (0<<CS21) | (0<<CS20);
TCNT2=0x00;
OCR2=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=(0<<OCIE2) | (0<<TOIE2) | (0<<TICIE1) | (0<<OCIE1A) | (0<<OCIE1B) | (0<<TOIE1) | (0<<OCIE0) | (0<<TOIE0);

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
// INT2: On
// INT2 Mode: Rising Edge
GICR|=0x20;
MCUCR=0x00;
MCUCSR=0x40;
GIFR=0x020;

// USART initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART Receiver: On
// USART Transmitter: On
// USART Mode: Asynchronous
// USART Baud Rate: 9600
UCSRA=(0<<RXC) | (0<<TXC) | (0<<UDRE) | (0<<FE) | (0<<DOR) | (0<<UPE) | (0<<U2X) | (0<<MPCM);
UCSRB=(1<<RXCIE) | (0<<TXCIE) | (0<<UDRIE) | (1<<RXEN) | (1<<TXEN) | (0<<UCSZ2) | (0<<RXB8) | (0<<TXB8);
UCSRC=(1<<URSEL) | (0<<UMSEL) | (0<<UPM1) | (0<<UPM0) | (0<<USBS) | (1<<UCSZ1) | (1<<UCSZ0) | (0<<UCPOL);
UBRRH=0x00;
UBRRL=0x47;

// Analog Comparator initialization
// Analog Comparator: Off
// The Analog Comparator's positive input is
// connected to the AIN0 pin
// The Analog Comparator's negative input is
// connected to the AIN1 pin
ACSR=(1<<ACD) | (0<<ACBG) | (0<<ACO) | (0<<ACI) | (0<<ACIE) | (0<<ACIC) | (0<<ACIS1) | (0<<ACIS0);

// ADC initialization
// ADC Clock frequency: 691.200 kHz
// ADC Voltage Reference: AVCC pin
ADMUX=ADC_VREF_TYPE & 0xff;
ADCSRA=0x84;

// SPI initialization
// SPI disabled
SPCR=(0<<SPIE) | (0<<SPE) | (0<<DORD) | (0<<MSTR) | (0<<CPOL) | (0<<CPHA) | (0<<SPR1) | (0<<SPR0);

// TWI initialization
// TWI disabled
TWCR=(0<<TWEA) | (0<<TWSTA) | (0<<TWSTO) | (0<<TWEN) | (0<<TWIE);

// Watchdog Timer initialization
// Watchdog Timer Prescaler: OSC/2048k
WDTCR=(0<<WDTOE) | (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);

// Alphanumeric LCD initialization
// Connections are specified in the
// Project|Configure|C Compiler|Libraries|Alphanumeric LCD menu:
// RS - PORTC Bit 1
// RD - PORTC Bit 2
// EN - PORTC Bit 3
// D4 - PORTC Bit 4
// D5 - PORTC Bit 5
// D6 - PORTC Bit 6
// D7 - PORTC Bit 7
// Characters/line: 20
lcd_init(20);

// 1 Wire Bus initialization
// 1 Wire Data port: PORTB
// 1 Wire Data bit: 2
// Note: 1 Wire port settings are specified in the
// Project|Configure|C Compiler|Libraries|1 Wire menu.
w1_init();

lcd_gotoxy(0,0);    lcd_putsf("       Remote       ");
lcd_gotoxy(0,1);    lcd_putsf("    Power Control   ");
delay_ms(3000);
lcd_gotoxy(0,2);    lcd_putsf("Designed by:        ");
lcd_gotoxy(0,3);    lcd_putsf("MOHAMMAD L. SAJJADI ");
delay_ms(3000);
lcd_clear();
  
#asm("wdr")

// New Module Configuration
if ( PSW == 0 ) config();

lcd_gotoxy(0,0); lcd_putsf("VL1:");
lcd_gotoxy(0,1); lcd_putsf("VL2:");
lcd_gotoxy(0,2); lcd_putsf("VL3:");
lcd_gotoxy(0,3); lcd_putsf("Temperature:");

if( dev1 ) REL4_ON;
if( dev2 ) REL5_ON;

putflash( "AT" );           delay_ms(500);
putflash( "AT+CMGD=1,4" );  delay_ms(5000);    // Delete All Messages

// Global enable interrupts
#asm("sei")

while (1)
    {
        #asm("wdr")
        data = VL1; 
        if ( data < 690 || data > 935 || L1 == 0 ) {REL1_OFF;  por1 = 1;   pir1 = 0;}  else pir1 = 1;    
        if( pir1 && por1 ) 
        {
            delay_ms(1000);   r++;
            if( r == 15 ) {REL1_ON; r = 0;  por1 = 0;   LD1++;}
            #asm("wdr")
        }
        dc = (float)data / 3.54 ;
        ftoa( dc , 0, vline1 );
        lcd_gotoxy(6,0); lcd_putsf("  v");
        lcd_gotoxy(5,0); lcd_puts(vline1); 
        
        #asm("wdr")
        data = VL2; 
        if ( data < 715 || data > 970 || L2 == 0 ) {REL2_OFF;  por2 = 1;   pir2 = 0;}  else pir2 = 1;    
        if( pir2 && por2 ) 
        {
            delay_ms(1000);   s++;
            if( s == 15 ) {REL2_ON; s = 0;  por2 = 0;   LD2++;}
            #asm("wdr")
        }
        dc = (float)data / 3.68 ;
        ftoa( dc , 0, vline2 );
        lcd_gotoxy(6,1); lcd_putsf("  v");
        lcd_gotoxy(5,1); lcd_puts(vline2); 
        
        #asm("wdr")
        data = VL3;
        if ( data < 700 || data > 950 || L3 == 0 ) {REL3_OFF;  por3 = 1;   pir3 = 0;}  else pir3 = 1;    
        if( pir3 && por3 ) 
        {
            delay_ms(1000);   t++;
            if( t == 15 ) {REL3_ON; t = 0;  por3 = 0;   LD3++;}
            #asm("wdr")
        }
        dc = (float)data / 3.61 ;
        ftoa( dc , 0, vline3 );
        lcd_gotoxy(6,2); lcd_putsf("  v");
        lcd_gotoxy(5,2); lcd_puts(vline3);
        #asm("wdr")
        
        dc = ds18b20_temperature(0);
        if( dc != (float)(-9999) )
        {
            ftoa( dc , 0 , temp );
            if( dev2 == 2 )
            { 
                if( dc < (float)(16) ) REL5_OFF;            
                else if( dc > (float)(30) ) REL5_ON;
            }
            if( (dc > (float)(45)) && (L1 || L2 || L3) )
            {
                #asm("wdr")
                UCSRB=0x08; // Disable RX
                REL1_OFF; REL2_OFF; REL3_OFF;
                L1 = 0;  L2 = 0;  L3 = 0;
                putflash("AT");                     delay_ms(300);
                putfl("AT+CMGS=\"09395240097");     delay_ms(100);
                putchar(34);  putchar(13);          delay_ms(999);
                putflash( "High Temperature!" );
                putflash( "All lines were turned off" );
                putchar(26);                        delay_ms(999);
                UCSRB=0x98; //Enable RX            
            }
        } 
        lcd_gotoxy(13,3);   lcd_puts(temp);
        lcd_putchar('C ');
        #asm("wdr")    
                
        if ( received ) extract();
        delay_ms(500);   
    }
}