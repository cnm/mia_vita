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
  char option;
  int dio_n;
  int value;

  if ( argc != 2 ) /* argc should be 2 for correct execution */
    {
      /* We print argv[0] assuming it is the program name */
      printf( "usage: %s [-s|-g] n v", argv[0] );
    }
/*  else */
/*    {*/
      option = argv[1][1];
      dio_n = atoi(argv[2]);

      if(option == 's'){
          printf("Setting the DIO %d", dio_n);
          value = atoi(argv[3]);
          setpin(dio_n, value);                         // Meter clock a 0
      }

      else if(option == 'g'){
          printf("Getting the DIO %d", dio_n);
          value = getpin(dio_n);                         // Meter clock a 0
          value = printf("VALUE: %d\n", value);
      }

      else
        {
          printf("Error");
        }
/*    }*/
}
