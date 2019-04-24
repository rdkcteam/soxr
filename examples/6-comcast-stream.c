/* SoX Resampler Library      Copyright (c) 2007-13 robs@users.sourceforge.net
 * Licence for this file: LGPL v2.1                  See LICENCE for details. */

/* Example 2: resample a raw, single-channel, floating-point data stream from
 * stdin to stdout.
 *
 * The application uses the single function `soxr_process' for both input and
 * output to/from the resampler; compared to the `input function' approach
 * (illustrated in example 3) this requires that the application implements
 * more logic, but one less function.
 *
 * Arguments are: INPUT-RATE OUTPUT-RATE
 */

#include <soxr.h>
#include "examples-common.h"

#include <sys/time.h>
struct timeval PerfTimer_tvstart;
unsigned int PerfTimer_microseconds = 0;
void PerfTimer_start(void);
void PerfTimer_stop(void);
void PerfTimer_printResult(void);
void PerfTimer_start(void)
{
    struct timezone tz;
    gettimeofday(&PerfTimer_tvstart, &tz);
}
void PerfTimer_stop(void)
{
    struct timeval tvstop;
    struct timezone tzstop;
    gettimeofday(&tvstop, &tzstop);
    PerfTimer_microseconds += (unsigned int)((tvstop.tv_sec - PerfTimer_tvstart.tv_sec)*1000000 + (tvstop.tv_usec - PerfTimer_tvstart.tv_usec));
}
void PerfTimer_printResult(void)
{
    printf("PerfTimer_microseconds=%u\n", PerfTimer_microseconds);
}


int main(int argc, char const * arg[])
{
  double const irate = 16000.0;
  double const orate = 8000.0;
  #define buf_total_len 1024  /* In samples. */
  size_t const olen = (size_t)(orate * buf_total_len / (irate + orate) + .5);
  size_t const ilen = buf_total_len - olen;
  size_t const osize = sizeof(short);
  size_t const isize = osize;
  void * obuf = malloc(osize * olen);
  void * ibuf = malloc(isize * ilen);
  size_t odone, written, need_input = 1;
  soxr_error_t error;
  soxr_t soxr;
  long num_frames;
  soxr_io_spec_t io_spec = soxr_io_spec(SOXR_INT16_I, SOXR_INT16_I);
  /*
    SOXR_QQ                 0 'Quick' cubic interpolation.
    SOXR_LQ                 1 'Low' 16-bit with larger rolloff.
    SOXR_MQ                 2 'Medium' 16-bit with medium rolloff.
    SOXR_HQ                 SOXR_20_BITQ 'High quality'.
    SOXR_VHQ                SOXR_28_BITQ 'Very high quality'.
    SOXR_16_BITQ            3
    SOXR_20_BITQ            4
    SOXR_24_BITQ            5
    SOXR_28_BITQ            6
    SOXR_32_BITQ            7
    SOXR_LSR0Q              8 'Best sinc'. 
    SOXR_LSR1Q              9 'Medium sinc'. 
    SOXR_LSR2Q              10 'Fast sinc'. 
  */
  unsigned long quality = argc > 1? (unsigned long)atoi(arg[1]) : SOXR_HQ;
  soxr_quality_spec_t qt_spec = soxr_quality_spec(quality, 0);
  
  soxr_runtime_spec_t rt_spec = soxr_runtime_spec(1);
  const char* in_file = argc > 2 ? arg[2] : "/home/mark/Downloads/large16m.pcm";
  const char* out_file = argc > 3 ? arg[3] : "/home/mark/Downloads/out16m.pcm";
  FILE* fdin = fopen(in_file, "rb");
  FILE* fdout = fopen(out_file, "wb"); 
  if(!fdin) { printf("bad input file\n"); exit(1); }
  if(!fdout) { printf("bad output file\n"); exit(1); }  
  
  fseek(fdin, 0, SEEK_END);
  num_frames = (int)ftell(fdin)/2;
  fseek(fdin, 0, SEEK_SET);
  printf("quality=%lu input=%s output=%s num_frames=%ld\n", quality, in_file, out_file, num_frames);

         
  PerfTimer_start();
  
  /* Create a stream resampler: */
  soxr = soxr_create(
      irate, orate, 1,             /* Input rate, output rate, # of channels. */
      &error,                         /* To report any error during creation. */
      &io_spec, 
      &qt_spec, 
      &rt_spec );

  if (!error) {                         /* If all is well, run the resampler: */
                                                       /* Resample in blocks: */
    do {
      size_t ilen1 = 0;

      if (need_input) {

        /* Read one block into the buffer, ready to be resampled: */
        ilen1 = fread(ibuf, isize, ilen, fdin);

        if (!ilen1) {     /* If the is no (more) input data available, */
          free(ibuf);     /* set ibuf to NULL, to indicate end-of-input */
          ibuf = NULL;    /* to the resampler. */
        }
      }

      /* Copy data from the input buffer into the resampler, and resample
       * to produce as much output as is possible to the given output buffer: */
      error = soxr_process(soxr, ibuf, ilen1, NULL, obuf, olen, &odone);

      written = fwrite(obuf, osize, odone, fdout); /* Consume output.*/

      /* If the actual amount of data output is less than that requested, and
       * we have not already reached the end of the input data, then supply some
       * more input next time round the loop: */
      need_input = odone < olen && ibuf;

    } while (!error && (need_input || written));
  }
                                                                  /* Tidy up: */
  soxr_delete(soxr);
  free(obuf), free(ibuf);

  PerfTimer_stop();
  PerfTimer_printResult();  
                                                              /* Diagnostics: */
  fprintf(stderr, "%-26s %s; I/O: %s\n", arg[0], soxr_strerror(error),
      ferror(stdin) || ferror(stdout)? strerror(errno) : "no error");
  return !!error;
}
