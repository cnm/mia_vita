#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#define BUFFER_SIZE 250

#define NUM_OF_CHANNELS 4

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
void read_sample(sample*, FILE*);
void correct_sample(sample*);
void separate_channels(sample*, unsigned int*);
void two_complement(unsigned int*);


/*
 *
 * CODE
 *
 */
int main(void)
{

  FILE *ifp;

  int i, j, read_samples;
  int ** channel_data = (int **) malloc(sizeof(int *) * BUFFER_SIZE);

  sample** samples = (sample**) malloc(sizeof(sample*) * BUFFER_SIZE);

  for(i=0; i<BUFFER_SIZE; i++)
    {
      channel_data[i] = (unsigned int *) malloc(sizeof(unsigned int) * NUM_OF_CHANNELS);
      samples[i] = (sample*) malloc(sizeof(sample));
    }

  ifp = fopen("/proc/geophone", "r");

  if(ifp == NULL)
    {
      fprintf(stderr, "No /proc/geophone to read from. Are you sure you installed the module.\n");
      exit(1);
    }

  for(i=0, read_samples=0; (i<BUFFER_SIZE) && (!feof(ifp)); ++i, ++read_samples)
    {
      read_sample(samples[i], ifp);

      /*
         printf("timestamp: %x\n", samp->timestamp);
         printf("data[0]: %x\n", samp->data[0]);
         printf("data[1]: %x\n", samp->data[1]);
         printf("data[2]: %x\n", samp->data[2]);
       */

      correct_sample(samples[i]);

      separate_channels(samples[i], channel_data[i]);

      two_complement(channel_data[i]);
    }

  for(i=0; i<read_samples; i++)
    {
      printf("Sample %04d: ", i);
      for(j=0; j < NUM_OF_CHANNELS; j++)
        {
          printf("%05d ", channel_data[i][j]);
        }
      printf("\n");
    }

  return 0;
}


void read_sample(sample* samp, FILE* ifp)
{
  unsigned char* temp;

  int i;
  unsigned int len_uc = sizeof(unsigned char);
  unsigned int length_sample = sizeof(sample);
  unsigned int read_oct = 0; // Number of read octets

  temp = (unsigned char*) samp;
  while (read_oct < length_sample)
    { // Read enough octets to fill a sample
      i = getc(ifp);
      *temp = (unsigned char) i;

      //    printf("%2X \n", i);

      read_oct += len_uc; 
      temp += len_uc;
    }
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
void correct_sample(sample* samp)
{
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

void separate_channels(sample* samp, unsigned int* channels)
{
  int i;
  uint8_t* sample_data;

  sample_data = (uint8_t*) samp->data;

  for(i=0; i < NUM_OF_CHANNELS*3; i +=3 )
    { // Extract the 24 bits from each channel
      // The bits come from the kernel in the following order: MSB -> LSB
      channels[i/3] = (unsigned int) ((sample_data[i]<<16) + (sample_data[i+1]<<8) + sample_data[i+2]);
    }

}

void two_complement(unsigned int* channels)
{

  int i;
  for(i=0; i < NUM_OF_CHANNELS; i++)
    {
      if(channels[i] > 0x800000)
        {
          channels[i] = (~(channels[i])+1 & 0x00FFFFFF)*(-1);
        }
    }
}
