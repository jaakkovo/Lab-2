/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
 */

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <cstdlib>

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>
#define LONG_TIME 0xffff

// TODO: insert other include files here
SemaphoreHandle_t xMutex = NULL;
SemaphoreHandle_t xBinary = NULL;
QueueHandle_t xQueue1 = NULL;

char word[61];
char empty[61];
int indexi = 0;
char question[] = "?";
unsigned long longVar = 10UL;

/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

}


/* UART (or output) thread */
static void task1(void *pvParameters) {
	while (1) {
		if (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 17)){
			xSemaphoreTake(xMutex, portMAX_DELAY);
			{
				DEBUGOUT("SW1 pressed");
				xSemaphoreGive ( xMutex );
				Board_LED_Set(0, true);
			}
			while (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 17)) {
			}
			Board_LED_Set(0, false);

		}

		//DEBUGOUT("Tick: %d \r\n", tickCnt);

		/* About a 1s delay here */
		vTaskDelay(configTICK_RATE_HZ / 12);
	}
}

/* UART (or output) thread */
static void task2(void *pvParameters) {
	while (1) {

		if (!Chip_GPIO_GetPinState(LPC_GPIO, 1, 11)){
			xSemaphoreTake(xMutex, portMAX_DELAY);
			{
				DEBUGOUT("SW2 pressed");
				xSemaphoreGive ( xMutex );
				Board_LED_Set(1, true);
			}
			while (!Chip_GPIO_GetPinState(LPC_GPIO, 1, 11)) {
			}
			Board_LED_Set(1, false);

		}

		//DEBUGOUT("Tick: %d \r\n", tickCnt);

		/* About a 1s delay here */
		vTaskDelay(configTICK_RATE_HZ / 12);
	}
}


static void readChars(void *pvParameters) {
	int character;
	char buf[2];

	while (1) {
		character = Board_UARTGetChar();

		if (character != -1){
			//sprintf(buf, "%c", character);
			//Board_UARTPutSTR (buf);
			xSemaphoreGive( xBinary );
		}
	}
}

/* UART (or output) thread */
static void receivedChars(void *pvParameters) {
	while (1) {
		if(xSemaphoreTake(xBinary, portMAX_DELAY) == pdPASS)
		{
			Board_LED_Set(1, true);
			vTaskDelay(configTICK_RATE_HZ / 10);
			Board_LED_Set(1, false);
		}
	}
}

/* UART (or output) thread */
static void readChars2(void *pvParameters) {
	int character;
	char buf[1];
	int isThere = 0;

	while (1) {

		if(xQueue1 == NULL){
			xQueue1 = xQueueCreate( 10, sizeof( unsigned long ) );
		}

		character = Board_UARTGetChar();
		isThere = 0;
		if (character != -1){

			sprintf(buf, "%c", character);

			if (character != 10 && character != 13 && indexi < 60){
				Board_UARTPutSTR (buf);
				word[indexi] = buf[0];
				indexi++;
			}else{
				indexi = 0;
				Board_UARTPutSTR ("\r[You] ");
				Board_UARTPutSTR (word);
				Board_UARTPutSTR ("\r\n");

				vTaskDelay(configTICK_RATE_HZ / 12);

				for (int i = 0; i<61; i++){
					if (word[i] == '?'){
						isThere = 1;
					}
				}

				if(isThere == 1){

					xQueueSend( xQueue1, (void*)&longVar, ( TickType_t ) 10 );

					isThere = 0;
				}

				memset (word, 0, sizeof word);

			}
		}
	}
	vTaskDelay(configTICK_RATE_HZ / 12);
}

/* UART (or output) thread */
static void receivedChars2(void *pvParameters) {
	while (1) {
		if (xQueue1 != NULL){
			unsigned long *longVar2;
			if( xQueueReceive( xQueue1, & (longVar2), ( TickType_t ) 10 ) )
			{
				Board_UARTPutSTR ("[Oracle] Hmmm...\r\n");
				vTaskDelay(configTICK_RATE_HZ * 3);

				int x = rand() % 4;

				switch(x)
				{
				case 0:
					Board_UARTPutSTR ("[Oracle] I don't know!\r\n");
					break;

				case 1:
					Board_UARTPutSTR ("[Oracle] Thats a tricky one!\r\n");
					break;

				case 2:
					Board_UARTPutSTR ("[Oracle] Ask someone else!\r\n");
					break;

				case 3:
					Board_UARTPutSTR ("[Oracle] Go away!\r\n");
					break;
				}

				vTaskDelay(configTICK_RATE_HZ);
			}
		}
	}
}

int main(void) {

#if defined (__USE_LPCOPEN)
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
	// Set up and initialize all required blocks and
	// functions related to the board hardware
	Board_Init();
	// Set the LED to the state of "On"
#endif
#endif

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 17, IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 17);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 11, IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 1, 11);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 9, IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 1, 9);


	xMutex = xSemaphoreCreateMutex();
	xBinary = xSemaphoreCreateBinary();
	xQueue1 = NULL;

	/* EXERCISE 1 */

	//		/* LED1 toggle thread */
	//		xTaskCreate(task1, "task1",
	//				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//				(TaskHandle_t *) NULL);
	//
	//		/* LED2 toggle thread */
	//		xTaskCreate(task2, "task2",
	//				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//				(TaskHandle_t *) NULL);

	//	/* EXERCISE 2 */
	//
	/* UART output thread, simply counts seconds */
	xTaskCreate(readChars, "readChars",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* UART output thread, simply counts seconds */
	xTaskCreate(receivedChars, "receivedChars",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* EXERCISE 3 */

	//	/* UART output thread, simply counts seconds */
	//	xTaskCreate(readChars2, "readChars2",
	//			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//			(TaskHandle_t *) NULL);
	//
	//	/* UART output thread, simply counts seconds */
	//	xTaskCreate(receivedChars2, "receivedChars2",
	//			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//			(TaskHandle_t *) NULL);


	/* Start the scheduler */
	vTaskStartScheduler();

	// Force the counter to be placed into memory
	volatile static int i = 0 ;
	// Enter an infinite loop, just incrementing a counter
	while(1) {
		i++ ;
	}
	return 0 ;
}
