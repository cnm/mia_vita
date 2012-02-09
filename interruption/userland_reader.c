#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

/*
 *The idea is to timestamp samples right after the first channel is read.
 */
typedef struct{

#ifdef __GPS__
    int64_t gps_us;
#endif

    int64_t timestamp;
    unsigned int data[3]; //each sample will hold 4 channels
}sample;


/*
 * Function Prototypes
 */
void correct_sample(sample*);

/*
 *
 * CODE
 *
 */
int main(void)
{
  int i, n;
  int getc= 0;
  int read_oct = 0; // Number of read octets
  int len_uc = sizeof(unsigned char);
  int length_sample = sizeof(sample);
  int offset = 0;

  unsigned char* temp;

  sample* samp = (sample*) malloc(sizeof(sample));

  FILE *ifp;

  ifp = fopen("/proc/geophone", "r");
      
  n = 0;
  temp = (unsigned char*) samp;
  while (read_oct < length_sample){ // Read enough octets to fill a sample
    i = getc(ifp);
    printf("%2X \n", i);
    read_oct += len_uc;
    *temp = (unsigned char) i; 
    temp += len_uc;
  }

  printf("Before\n");
  printf("timestamp: %x\n", samp->timestamp);
  printf("data[0]: %x\n", samp->data[0]);
  printf("data[1]: %x\n", samp->data[1]);
  printf("data[2]: %x\n", samp->data[2]);

  correct_sample(samp);

  printf("After\n");
  printf("timestamp: %x\n", samp->timestamp);
  printf("data[0]: %x\n", samp->data[0]);
  printf("data[1]: %x\n", samp->data[1]);
  printf("data[2]: %x\n", samp->data[2]);

  printf("OuT\n");
  
  return 0;
}


/*DATA memory layout:
 *
 *For 3 integers 0xAABBCCDD, DATA is:
 *
 *byte:       0  2  1  0    1  0  2  1    2  1  0  2
 *DATA:       DD CC BB AA | DD CC BB AA | DD CC BB AA
 *Sample:     2-|---1-----|--3---|--2---|----4----|-3
 *be_samples:
 */
void correct_sample(sample* samp){
  unsigned int* corr_data = (unsigned int*) malloc(sizeof(unsigned int) * 3);
  uint8_t* int1 = (uint8_t*) samp->data;
  uint8_t* int2 = (uint8_t*) (samp->data + 1);
  uint8_t* int3 = (uint8_t*) (samp->data + 2);

  ((uint8_t*) corr_data)[0] = int1[3];
  ((uint8_t*) corr_data)[1] = int1[2];
  ((uint8_t*) corr_data)[2] = int1[1];

  ((uint8_t*) corr_data)[3] = int1[0];
  ((uint8_t*) corr_data)[4] = int2[3];
  ((uint8_t*) corr_data)[5] = int2[2];

  ((uint8_t*) corr_data)[6] = int2[1];
  ((uint8_t*) corr_data)[7] = int2[0];
  ((uint8_t*) corr_data)[8] = int3[3];

  ((uint8_t*) corr_data)[9] = int3[2];
  ((uint8_t*) corr_data)[10] = int3[1];
  ((uint8_t*) corr_data)[11] = int3[0];

  samp->data[0] = corr_data[0];
  samp->data[1] = corr_data[1];
  samp->data[2] = corr_data[2];
  
}
