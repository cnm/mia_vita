/*******************************************************************************
 * Program: 
 *    Get DIO (dio.c)
 *    Technologic Systems TS-7500 with TS-752 Development Board
 * 
 * Summary:
 *   This program will accept any pin number between 5 and 40 and attempt to get
 * or set those pins in a c program rather than scripted.  You will need the 
 * TS-7500 and TS-752 development board. Although this program will enable the 
 * use of said pins, it will primarily enable the use of the 8 Inputs, 3 Outputs,
 * and Relays on the TS-752.  Keep in mind that if a GND or PWR pin is read (or
 * something else unlogical, we don't necessarily care about the output because 
 * it could be simply "junk". 
 *   Notice careful semaphore usage (sbuslock, sbusunlock) within main.
 *
 * Usage:
 *   ./dio <get|set> <pin#> <set_value (0|1|2)>
 *
 * 0 - GND
 * 1 - 3.3V
 * 2 - Z (High Impedance)
 *
 * Examples:
 *   To read an input pin (such as 1 through 8 on the TS-752):
 *      ts7500:~/sbus# ./dio get 40
 *      Result of getdiopin(38) is: 1 
 *
 *   To set an output pin (such as 1 through 3 or relays on the TS-752):
 *      ts7500:~/sbus# ./dio set 33 0
 *      Pin#33 has been set to 0   [You may verify with DVM]
 *
 * Compile with:
 *   gcc -mcpu=arm9 dio.c sbus.c -o dio
 *******************************************************************************/
#include "sbus.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#define DIO_Z 2

#define CLOCK   14
#define CS      13
#define SDO     12

#define PREPARE_TIME 500
#define INTERVAL_TIME 500

#define NANO_SECONDS 1000
#define MICRO_SECONDS 1000


void wait_miliseconds(int miliseconds){

    struct timespec interval, left;

    interval.tv_sec = 0;
    interval.tv_nsec = 500 * MICRO_SECONDS * NANO_SECONDS;

    if (nanosleep(&interval, &left) == -1) {
        if (errno == EINTR) {
            printf("nanosleep interrupted\n");
            printf("Remaining secs: %d\n", left.tv_sec);
            printf("Remaining nsecs: %d\n", left.tv_nsec);
        }
        else perror("nanosleep");
    }
}

void setpin(int pin, int value){
    sbuslock();
    setdiopin(pin, value);
    sbusunlock();
}

int getpin(int pin){
    int returned_value;
    sbuslock();
    returned_value = getdiopin(pin);
    sbusunlock();

    return returned_value;
}

int main(int argc, char **argv)
{
  int returned_value;
  int i;
  int x;

  setpin(CLOCK, 0);                         // Meter clock a 0
  setpin(CS, 1);                            // Desligar o CS
  wait_miliseconds(PREPARE_TIME);           // Esperar um segundo
  setpin(SDO, 2);                           // PREPARAR METER O SDO para enviar dados
  setpin(CS, 0);                            // Ligar o CS
  wait_miliseconds(PREPARE_TIME);           // Esperar um segundo

  for (i = 1; i++; i<=32){
      setpin(CLOCK, 1);                       // Flanco ascendente
      wait_miliseconds(INTERVAL_TIME);         // Esperar um segundo
      x = getpin(SDO);             // Buscar valores

      /* Por a rodar para o proximo bit */
      if (x == 0)
        returned_value = returned_value << 1; 
      else
        returned_value = (returned_value << 1) | 1;

      setpin(CLOCK, 0);                        // Flanco descendente
      wait_miliseconds(INTERVAL_TIME);         // Esperar um segundo
  }

  printf("\t\t%x", returned_value);
}
