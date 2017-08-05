
/*Trigger1= PE1
 Trigger2= PE4
 Echo1= PE2
 echo2= PE3
 */
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "inc/lm4f120h5qr.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "utils/uartstdio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"
#include "HC_SR04_1.h"
#include "stdlib.h"


#include "driverlib/uart.h"
#include "utils/uartstdio.h"

// secenekler: 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
#define BAUDRATE 9600

#define Hassasiyet 15// azaltinca hizlanir
int say=0;
unsigned long sayac;
void init_UARTstdio() {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(0x00000001);
    GPIOPinConfigure(0x00000401);
    GPIOPinTypeUART(0x40004000, 0x00000001 | 0x00000002);
    UARTConfigSetExpClk(0x40004000, SysCtlClockGet(), BAUDRATE,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));
    UARTStdioConfig(0, BAUDRATE, SysCtlClockGet());
}

void yak(unsigned long ulPeriod)
{
	 	 SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	 	 GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	 	 SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
	 	 TimerConfigure(TIMER3_BASE, TIMER_CFG_32_BIT_PER);
		 TimerLoadSet(TIMER3_BASE, TIMER_A, ulPeriod -1);
		 IntEnable(INT_TIMER3A);
		 TimerIntEnable(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
		 IntMasterEnable();
		 TimerEnable(TIMER3_BASE, TIMER_A);
}

int main(void) {
	FPULazyStackingEnable();

    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    init_UARTstdio();

    // PORTF ve PORTD'yi aktiflestir
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// PF 1,2,3 out (LEDler)
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	Ping_Init1();

	while (1) {
		Continual_Ping1();

		while (GetSent1() < 12.5)
		{

			int cm = get_mm()/10;
		    UARTprintf("%d cm\n", cm);
			sayac++;
			if(cm>99)
			{
				TimerIntDisable(TIMER3_BASE, TIMER_TIMA_TIMEOUT);  //timer kapatýlýyor
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0b00010);//kirmizi led
				GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0b00000);//buzzer hep kapali

			}

			if(cm<=100 && cm>5)
			{
				//if(cm>say+1|| cm<say-1){
				if(fabs(say-cm)>1){
					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0b00000);//kirmizi led kapalý
				UARTprintf("Timer Kuruldu-> %d\n",cm);//seri porta yaz
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0b01000); //timer ayarlandýkça yesil led yakýlýyor.
				yak(Hassasiyet*10000*cm); //buzzer yanma hýzý hesaplanýyor
				}
			}
			if(cm<=5){
				TimerIntDisable(TIMER3_BASE, TIMER_TIMA_TIMEOUT);  //timer kapatýlýyor
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0b00010);//kirmizi led
				GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0b00100); //buzzer hep acik
			}
			if(fabs(say-cm)>1){
			say=cm;
			}
            SysCtlDelay(SysCtlClockGet()/18);
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0b00000);
		}

	}
}

void Timer3IntHandler(void)
{
// Clear the timer interrupt
TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
// Read the current state of the GPIO pin and
// write back the opposite state
if(GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_2))
{
GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
}
else
{
GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 4);
}
}



