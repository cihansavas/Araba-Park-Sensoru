
/*
 * kod asagidaki kaynaktan alinip sadelestirilmistir
 * https://e2e.ti.com/support/microcontrollers/stellaris_arm/f/471/t/367348
 */

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
#include "HC_SR04_1.h"


//For first ultrasonic sensor
unsigned long SENSOR_LIMIT_TIMER_VALUE1, PING_INTERVAL_TICKS1, TICKS_PER_US1, START_TIMER_VALUE1, MAX_END_TIMER_VALUE1;
unsigned long EndTime1, PingLimit1, PingSent1, PingValid1, PingLost1, PingInvalid1, PingMax1;
volatile unsigned long PingReceived1;
//signed long Inches1, TotalInches1;

// olcum sonucu
long int mm = 0;

//----------------------------------------------------------------------------------------------------------------------------------------

//For first ultrasonic sensor
void Ping_Init1() {

	//This should setup all the values the interrupt handlers and the most granular interrupt level disabled

	//Enable the GPIO Port
	SysCtlPeripheralEnable(SENSOR_PORT_ENABLE);

	// Enable processor interrupts.
	IntMasterEnable();

	//Enable Trigger and Echo Pins for the sensor
	GPIOPinTypeGPIOOutput(SENSOR_PORT, TRIG_PIN1); // PE1 out
	GPIOPinTypeGPIOInput(SENSOR_PORT, ECHO_PIN1); // PE2 in
	GPIOPinWrite(SENSOR_PORT, TRIG_PIN1, 0); // PE1'in cikis degerini sifirla

	// echo girisi icin kesme kurulumu
	//Setup Echo to cause a GPIO interrupt
	IntEnable(SENSOR_PORT_INT);
	GPIOPinIntClear(SENSOR_PORT, ECHO_PIN1); //Clear before enable as a general rule of thumb
	GPIOIntTypeSet(SENSOR_PORT, ECHO_PIN1, GPIO_FALLING_EDGE | GPIO_DISCRETE_INT); //The echo pulse starts at a fixed time after the trigger so we only need to watch for the end
	GPIOPortIntRegister(SENSOR_PORT, EchoPulseEndHandler1); //Register the ISR (can be done in startup_css also)
	GPIOPinIntDisable(SENSOR_PORT, ECHO_PIN1);

	// 1 mikrosaniyedeki timer tick sayisi
	//Calculate the number of timer ticks per microsecond based on clock being used
	TICKS_PER_US1 = SysCtlClockGet() / 1000000; //Matches MHZ setting (50 for 50MHZ)

	//Calculate the number of timer ticks in a Ping interval
	PING_INTERVAL_TICKS1 = PING_INTERVAL_US * TICKS_PER_US1;

	//The Timer value that the echo pulse starts at is the same every time, so calculate up front and always use
	START_TIMER_VALUE1 = PING_INTERVAL_TICKS1 - (SENSOR_DELAY_US * TICKS_PER_US1);

	//Values below this floor are the sensor registering no echo (not reliable on most devices so also plan for no end pulse)
	SENSOR_LIMIT_TIMER_VALUE1 = START_TIMER_VALUE1 - (SENSOR_MAX_PULSE_US * TICKS_PER_US1);

	//This is the lowest timer value that can be considered reliable
	MAX_END_TIMER_VALUE1 = START_TIMER_VALUE1 - (MAX_SENSOR_DISTANCE_IN * ROUNDTRIP_IN_US * TICKS_PER_US1); //Max distance in inches * us per inch * ticks /us = Total Ticks to wait

	//Enable Timer to schedule the ping and measure echo pulse
	SysCtlPeripheralEnable(TIMER_PERIPH1);
	//Configure for Periodic timer based on sensor refresh interval
	TimerConfigure(TIMER1, TIMER_CFG_PERIODIC); //Note this counts DOWN
	TimerLoadSet(TIMER1, TIMER_SUB1, PING_INTERVAL_TICKS1); //Start at the Ping interval
	TimerDisable(TIMER1, TIMER_SUB1);

	// Setup the interrupts for the timer timeouts.
	TimerIntClear(TIMER1, TIMER_EVENT1);
	TimerIntRegister(TIMER1, TIMER_SUB1, PingIntervalHandler1); //Register before enable
	TimerIntDisable(TIMER1, TIMER_EVENT1);
	IntEnable(TIMERT_INT1);
}


void Continual_Ping1(void) {
	PingLimit1 = 0; //PingLimit 0 means continual
	ResetCounts1();
	TriggerPulse1();
	while (PingReceived1 == 0) {
	}
}

//This executes the trigger pulse and starts the timer and GPIO pin interrupt to return the value
void TriggerPulse1(void) {

	//Reset the GPIO Interrupt
	GPIOPinWrite(SENSOR_PORT, TRIG_PIN1, TRIG_PIN1); //Set Trigger pin HIGH
	SysCtlDelay(TRIGGER_PULSE_US * TICKS_PER_US1 / 3); //THe SysCTLDelay is 3 ticks so divide required by 3

	GPIOPinWrite(SENSOR_PORT, TRIG_PIN1, 0); //Set Trigger LOW
	SysCtlDelay(TRIGGER_PULSE_US * TICKS_PER_US1 / 3); //THe SysCTLDelay is 3 ticks so divide required by 3

	EndTime1 = MAX_END_TIMER_VALUE1; //Set the End time now, that way if the interrupt never fires (Ping beyond max distance) the endtime is set at max
	++PingSent1;

	//Reset Timer and Enable the interrupt
	TimerLoadSet(TIMER1, TIMER_SUB1, PING_INTERVAL_TICKS1); //Reset the timer value
	TimerIntEnable(TIMER1, TIMER_EVENT1); //Enable the interrupt
	TimerEnable(TIMER1, TIMER_SUB1); //Start It
	GPIOPinIntClear(SENSOR_PORT, ECHO_PIN1); //Clear before enable as a best practice
	GPIOPinIntEnable(SENSOR_PORT, ECHO_PIN1); //Enable
}

void EchoPulseEndHandler1(void) //This ISR executes when it detects the end of the echo pulse (NOTE: Will not happen if distance is beyond the max range)
{
	GPIOPinIntClear(SENSOR_PORT, ECHO_PIN1); //Clear at the beginning of the ISR per TI
	EndTime1 = TimerValueGet(TIMER1, TIMER_SUB1); //Read the timer value
	GPIOPinIntDisable(SENSOR_PORT, ECHO_PIN1); //Turn off until the Trigger function re-enables, this probably isn't needed but the junk sensors end the no echo pulse at random times up to 200 ms after start
}

//This does the majority of the work to look at the Endtime and calculate the latest inches value
void PingIntervalHandler1(void) //Always send a trigger pulse prior to this being called
{
	signed long MicroSeconds1, LastInches1, Delta1;

	TimerIntClear(TIMER1, TIMER_EVENT1);
	//LastInches1 = Inches1; //Store the last value to apply optional filters

	//Depending on the time of the interrupt classify the response, lots of logic needed based on sensors that don't act like they should
	++PingReceived1;
	if (EndTime1 < SENSOR_LIMIT_TIMER_VALUE1) { // Means it fired at the max sensor time (which seems rare for knock-off devices)
		++PingMax1;
		//Inches1 = MAX_SENSOR_DISTANCE_IN;
		mm = MAX_SENSOR_DISTANCE_MM;
	} else if (EndTime1 < MAX_END_TIMER_VALUE1) { // This means the interrupt occurred beyond the limit of the sensor but the sensor didn't register it as max
		++PingInvalid1;
		//Inches1 = MAX_SENSOR_DISTANCE_IN;
		mm = MAX_SENSOR_DISTANCE_MM;
	} else if (EndTime1 == MAX_END_TIMER_VALUE1) { // This means the interrupt didn't trigger in the Ping interval
		++PingLost1;
		//Inches1 = MAX_SENSOR_DISTANCE_IN;
		mm = MAX_SENSOR_DISTANCE_MM;
	} else {
		++PingValid1;

		MicroSeconds1 = (START_TIMER_VALUE1 - EndTime1) / TICKS_PER_US1;
		//Inches1 = MicroSeconds1 / ROUNDTRIP_IN_US; // Convert us to inches

		// ses hizi saniyede 340 m/s = 340000 mm/s
		// gecen sure ses dalgasinin gidis+donus suresi
		// 340000/1000000
		// long mm = (MicroSeconds1 / 2 * 340000) / 1000000;
		// sadelestirirsek

		mm = MicroSeconds1 * 17 / 100;
	}

	//Either kick off the next cycle or end
	if ((PingReceived1 < PingLimit1) | (PingLimit1 == 0)) {
		//Start Next Measurement (Limit 0f 0 means continual)
		TriggerPulse1();
	} else {
		//If Ping Count has been hit disable the Timer and pin interrupt (Do this in case the run before didn't fire echo)
		Stop_Ping1();
	}
}

//Turn off all interrupts and the timer
void Stop_Ping1(void) {
	TimerDisable(TIMER1, TIMER_SUB1);
	TimerIntDisable(TIMER1, TIMER_EVENT1);
	GPIOPinIntClear(SENSOR_PORT, TRIG_PIN1);
	GPIOPinIntClear(SENSOR_PORT, ECHO_PIN1);
	GPIOPinIntDisable(SENSOR_PORT, ECHO_PIN1); //Only looking for one falling edge per ping interval so disable the interrupt when it is found (the next cycle will re-enable)
}

void ResetCounts1(void) {
	PingSent1 = 0;
	PingReceived1 = 0;
	PingLost1 = 0;
	PingMax1 = 0;
	PingValid1 = 0;
	PingInvalid1 = 0;
}


long int get_mm() {
	return mm;
}

signed int GetSent1(void) {
	return PingSent1;
}

signed int GetReceived1(void) {
	return PingReceived1;
}

signed int GetLost1(void) {
	return PingLost1;
}

signed int GetMax1(void) {
	return PingMax1;
}

signed int GetValid1(void) {
	return PingValid1;
}

signed int GetInvalid1(void) {
	return PingInvalid1;
}

