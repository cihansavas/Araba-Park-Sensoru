
/*
 * kod asagidaki kaynaktan alinip sadelestirilmistir
 * https://e2e.ti.com/support/microcontrollers/stellaris_arm/f/471/t/367348
 */

#define MAX_SENSOR_DISTANCE_IN 200 // Maximum reliable sensor distance in Inches
#define MAX_SENSOR_DISTANCE_MM 4000
#define ROUNDTRIP_IN_US 148       // Microseconds (uS) it takes to travel round-trip 1 inch
#define TRIGGER_PULSE_US 10		//10 microseconds (from datasheet)
#define SENSOR_DELAY_US 450  	// SR04 echo goes high ~450us after the end of the trigger signal
#define SENSOR_MAX_PULSE_US 40000 //In cases of the device not getting an echo it should go low 36ms later - Note most of mine don't reliably pull the line down on a lost echo until 200ms later
#define MAX_CHANGE_POS 10 		//Max Change allowed in one Ping Interval, up
#define MAX_CHANGE_NEG -10 		//Max Change allowed in one Ping Interval, down
#define INITIAL_COUNT  2		//Intervals to measure before applying max change
#define PING_INTERVAL_US  80000	//Cycle Interval in milliseconds don't go lower than 40000 per datasheet

//Define the port/pins for the trig/echo
#define SENSOR_PORT  		GPIO_PORTE_BASE
#define SENSOR_PORT_INT  	INT_GPIOE
#define SENSOR_PORT_ENABLE 	SYSCTL_PERIPH_GPIOE
#define TRIG_PIN1    		GPIO_PIN_1
#define ECHO_PIN1 		 	GPIO_PIN_2
//#define TEST_PIN 		 	GPIO_PIN_3

//Define the timer used to calculate distance and kick off the trigger pulses
//You can use a single 16 bit timer with prescaler as long as you count down - this uses a 32 bit
#define TIMER1 				TIMER0_BASE
#define TIMER_SUB1 			TIMER_A
#define TIMER_PERIPH1		SYSCTL_PERIPH_TIMER0
#define TIMERT_INT1  		INT_TIMER0A
#define TIMER_EVENT1			TIMER_TIMA_TIMEOUT

#define TIMER2 				TIMER1_BASE
#define TIMER_SUB2 			TIMER_A
#define TIMER_PERIPH2		SYSCTL_PERIPH_TIMER1
#define TIMERT_INT2  		INT_TIMER1A
#define TIMER_EVENT2			TIMER_TIMA_TIMEOUT


//External functions
void Ping_Init1(void);
signed long Single_Ping1(void);
signed long Multi_Ping1(int count, tBoolean average);
void Continual_Ping1(void);
void Stop_Ping1(void);
signed int GetSent1(void);
signed int GetLost1(void);
signed int GetReceived1(void);
signed int GetMax1(void);
signed int GetValid1(void);
signed int GetInvalid1(void);

//Internal functions
void TriggerPulse1(void);
void PingIntervalHandler1(void);
void EchoPulseEndHandler1(void);
void ResetCounts1(void);
