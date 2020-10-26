#include <stdint.h>
#include <malloc.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include "image.h"

static int frequency_hz = 1000;
static unsigned int desired_rate = 48000;
static unsigned int actual_rate;  



static int SetLevels(long master, long capture)
{
   const char *card = "default";
   const char *pb_name = "Master";
   const char *cap_name = "Capture";
   int err;

   snd_mixer_t *handle;
   snd_mixer_open(&handle, 0);
   snd_mixer_attach(handle, card);
   snd_mixer_selem_register(handle, NULL, NULL);
   snd_mixer_load(handle);

   if(master >= 0) {
      long vol, min=0, max=0;
      snd_mixer_selem_id_t *sid_pb;
      snd_mixer_elem_t* elem_pb;

      snd_mixer_selem_id_alloca(&sid_pb);
      snd_mixer_selem_id_set_index(sid_pb, 0);
      snd_mixer_selem_id_set_name(sid_pb, pb_name);
      elem_pb = snd_mixer_find_selem(handle, sid_pb);
      snd_mixer_selem_get_playback_volume_range(elem_pb, &min, &max);
      vol = master * (max-min) / 100 + min;
      snd_mixer_selem_set_playback_volume_all(elem_pb, vol);
      printf("Setting playback volume to %li in range %li %li\n", vol, max, min);
   }

   if(capture >= 0) {
      long vol, min=0, max=0;
      snd_mixer_selem_id_t *sid_cap;
      snd_mixer_elem_t* elem_cap;

      snd_mixer_selem_id_alloca(&sid_cap);
      snd_mixer_selem_id_set_index(sid_cap, 0);
      snd_mixer_selem_id_set_name(sid_cap, cap_name);
      elem_cap = snd_mixer_find_selem(handle, sid_cap);
    
      err = snd_mixer_selem_get_capture_volume_range(elem_cap, &min, &max);
      if(err != 0) {
         printf("Unable to get capture limits\n");
      }

      vol = capture * (max-min) / 100 + min;

      err = snd_mixer_selem_set_capture_volume_all(elem_cap, vol);
      if(err != 0) {
         printf("Unable to set capture volume\n");
      }
      printf("Setting capture level to %li in range %li %li\n", vol, max, min);
   }

   snd_mixer_close(handle);
   return 1;
}


static int init_pb(snd_pcm_t **snddev_pb, const char *name)
{
  int err;
  snd_pcm_hw_params_t *hw_params;

  if( name == NULL ) {
      err = snd_pcm_open(snddev_pb, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0 );
  } else {
      err = snd_pcm_open(snddev_pb, name, SND_PCM_STREAM_PLAYBACK, 0);
  }

  if( err < 0 ) {
      printf("Init: cannot open audio playback device %s (%s)\n", name, snd_strerror(err));
      return 0;
  }
  printf("Audio playback device opened successfully.\n");

  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
      printf("Init: cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
      return 0;
  }
 
  if ((err = snd_pcm_hw_params_any (*snddev_pb, hw_params)) < 0) {
      printf("Init: cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
      return 0;
  }

  unsigned int resample = 1;
  err = snd_pcm_hw_params_set_rate_resample(*snddev_pb, hw_params, resample);
  if (err < 0) {
      printf("Init: Resampling setup failed for playback: %s\n", snd_strerror(err));
      return 0;
  }

  // Set access to RW interleaved.
  if ((err = snd_pcm_hw_params_set_access (*snddev_pb, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
      printf("Init: cannot set access type (%s)\n", snd_strerror (err));
      return 0;
  }

  if ((err = snd_pcm_hw_params_set_format (*snddev_pb, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
      printf("Init: cannot set sample format (%s)\n", snd_strerror (err));
      return 0;
  }

  // Set channels to stereo (2).
  if ((err = snd_pcm_hw_params_set_channels (*snddev_pb, hw_params, 2)) < 0) {
      printf("Init: cannot set channel count (%s)\n", snd_strerror (err));
      return 0;
  }

  // Set sample rate.
  actual_rate = desired_rate;
  if ((err = snd_pcm_hw_params_set_rate_near (*snddev_pb, hw_params, &actual_rate, 0)) < 0) {
      printf("Init: cannot set sample rate to %i. (%s)\n",desired_rate, snd_strerror(err));
      return 0;
  }
  if( actual_rate < desired_rate ) {
      printf("Init: sample rate does not match requested rate. (%i)\n", actual_rate);
  }

  if(snd_pcm_nonblock(*snddev_pb, 1) < 0) {
      printf("Init: cannot set non-blocking (%s)\n", snd_strerror (err));
  }

  if ((err = snd_pcm_hw_params (*snddev_pb, hw_params)) < 0) {
      printf("Init: cannot set parameters (%s)\n", snd_strerror (err));
      return 0;
  } else {
     printf("Audio device parameters have been set successfully.\n");
  }

  snd_pcm_uframes_t bufferSize;
  snd_pcm_hw_params_get_buffer_size( hw_params, &bufferSize );
  printf("Init: Buffer size = %lu frames.\n", bufferSize);
  printf("Init: Significant bits for linear samples = %i\n", snd_pcm_hw_params_get_sbits(hw_params));
  snd_pcm_hw_params_free (hw_params);

  if ((err = snd_pcm_prepare(*snddev_pb)) < 0) {
      printf("Init: cannot prepare audio interface for use (%s)\n", snd_strerror(err));
      return 0;
  } else {
      printf("Audio device has been prepared for use.\n");
  }

  return 1;
}

static int init_cap(snd_pcm_t **snddev_cap, const char *name)
{
  int err;
  snd_pcm_hw_params_t *hw_params;

  if( name == NULL ) {
      err = snd_pcm_open(snddev_cap, "plughw:0,0", SND_PCM_STREAM_CAPTURE, 0 );
  } else {
      err = snd_pcm_open(snddev_cap, name, SND_PCM_STREAM_CAPTURE, 0);
  }

  if( err < 0 ) {
      printf("Init: cannot open audio playback device %s (%s)\n", name, snd_strerror(err));
      return 0;
  } else {
      printf("Audio playback device opened successfully.\n");
  }

  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
      printf("Init: cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
      return 0;
  }
 
  if ((err = snd_pcm_hw_params_any (*snddev_cap, hw_params)) < 0) {
      printf("Init: cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
      return 0;
  }

  unsigned int resample = 1;
  err = snd_pcm_hw_params_set_rate_resample(*snddev_cap, hw_params, resample);
  if (err < 0) {
      printf("Init: Resampling setup failed for playback: %s\n", snd_strerror(err));
      return err;
  }

  // Set access to RW interleaved.
  if ((err = snd_pcm_hw_params_set_access (*snddev_cap, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
      printf("Init: cannot set access type (%s)\n", snd_strerror (err));
      return 0;
  }

  if ((err = snd_pcm_hw_params_set_format (*snddev_cap, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
      printf("Init: cannot set sample format (%s)\n", snd_strerror (err));
      return 0;
  }

  // Set channels to stereo (2).
  if ((err = snd_pcm_hw_params_set_channels (*snddev_cap, hw_params, 2)) < 0) {
      printf("Init: cannot set channel count (%s)\n", snd_strerror (err));
      return 0;
  }

  // Set sample rate.
  unsigned int actualRate = 48000;
  if ((err = snd_pcm_hw_params_set_rate_near (*snddev_cap, hw_params, &actualRate, 0)) < 0) {
      printf("Init: cannot set sample rate to 48000. (%s)\n", snd_strerror(err));
      return 0;
  }
  if( actualRate < desired_rate ) {
      printf("Init: sample rate does not match requested rate. (%i)\n", actualRate);
  }

  if(snd_pcm_nonblock(*snddev_cap, 1) < 0) {
      printf("Init: cannot set non-blocking (%s)\n", snd_strerror (err));
  }

  if ((err = snd_pcm_hw_params (*snddev_cap, hw_params)) < 0) {
      printf("Init: cannot set parameters (%s)\n", snd_strerror (err));
      return 0;
  } else {
     printf("Audio capture device parameters have been set successfully.\n");
  }

  snd_pcm_uframes_t bufferSize;
  snd_pcm_hw_params_get_buffer_size( hw_params, &bufferSize );
  printf("Init: Buffer size = %lu frames.\n", bufferSize);
  printf("Init: Significant bits for linear samples = %i\n", snd_pcm_hw_params_get_sbits(hw_params));
  snd_pcm_hw_params_free (hw_params);

  if ((err = snd_pcm_prepare(*snddev_cap)) < 0) {
      printf("Init: cannot prepare audio capture interface for use (%s)\n", snd_strerror(err));
      return 0;
  } else {
      printf("Audio capture device has been prepared for use.\n");
  }

  return 1;
}


static int UnInit(snd_pcm_t *snddev_pb, snd_pcm_t *snddev_cap)
{
  if(snddev_pb)
    snd_pcm_close (snddev_pb);
  if(snddev_cap)
    snd_pcm_close (snddev_cap);
  printf("Audio devices has been uninitialized.\n");
  return 1;
}


struct frame_i16_stereo {
   int16_t l;
   int16_t r;
};

static int capture_data(double *points, int point_count, int volume_pb, int volume_cap) {
   assert(points != NULL);
   snd_pcm_t *snddev_pb;
   snd_pcm_t *snddev_cap;
   int rtn = 0;
   int wp = 0;
   struct frame_i16_stereo buffer_out[1024];
   struct frame_i16_stereo buffer_in[1024];
   int16_t *pb_samples;

   pb_samples  = malloc(sizeof(int16_t)*desired_rate);
   if(pb_samples == NULL) {
      fprintf(stderr,"Out of memory\n");
      return 0;
   }
   for(int i = 0; i < desired_rate; i++) {
      int s = 3*8192*sin((i*2+1)/(desired_rate*2.0)*2.0*M_PI);
      if(s < 0) {
         s -= 0.5;
      } else {
         s += 0.5;
      }
      pb_samples[i] = s;
   }

   if(init_pb(&snddev_pb, NULL) && init_cap(&snddev_cap, NULL)) {
      int to_write = 0;
      int buffer_written = 0;
      int samples_read = 0;
      int skip = actual_rate/10;
      SetLevels(volume_pb, volume_cap);

      while(samples_read < skip+point_count) {
         if(to_write == 0) {
            for(int j = 0; j < sizeof(buffer_out)/sizeof(struct frame_i16_stereo); j++) {
               buffer_out[j].l = pb_samples[wp];
               buffer_out[j].r = pb_samples[wp];
               wp += frequency_hz;
               if(wp >= desired_rate) 
                 wp -= desired_rate;
            }
            to_write = sizeof(buffer_out)/sizeof(struct frame_i16_stereo); 
            buffer_written = 0;
         }
   

         int frames_written = snd_pcm_writei(snddev_pb, buffer_out+buffer_written, to_write);
         if(frames_written < 0) {
            if(frames_written != -EAGAIN) {
               printf("Playback error %i\n", frames_written);
            }
         } else {
            to_write       -= frames_written;
            buffer_written += frames_written;
         }

         int frames_read = snd_pcm_readi(snddev_cap, buffer_in, sizeof(buffer_in)/sizeof(struct frame_i16_stereo));
         if(frames_read > 0) {
             for(int i = 0; i < frames_read; i++) {
                if(samples_read >= skip && samples_read < skip+point_count)
                  points[samples_read - skip] = buffer_in[i].l;
                samples_read++;
             } 
         }
         usleep(5000);
      }
      rtn = 1;
   }
   UnInit(snddev_pb, snddev_cap);
   free(pb_samples);
   return rtn;
}

//=========================================================================================
#define WIDTH   1920
#define HEIGHT  1080
#define LEFT_MARGIN   200
#define RIGHT_MARGIN   50
#define TOP_MARGIN    100
#define BOTTOM_MARGIN 100


void plot(double *data, int count, char *bottom_text) {
   struct image *img;
   struct image *font;
   double min,max;
   int last;

   if(count < 2)
      return;
   min = data[0];
   max = data[1];
   if(max == min)
     max += 0.1;

   for(int i = 0; i < count; i++) {
     if(min > data[i]) 
        min = data[i];
     if(max < data[i]) 
        max = data[i];
   }
   max = 0;
   min = -140;
   img = image_new(WIDTH, HEIGHT);
   if(img == NULL) {
     printf("Out of RAM\n");
     return;
   }
   font = image_from_ppm("font_ML.ppm");
   if(font == NULL) {
     printf("Out of RAM\n");
     image_free(img);
     return;
   }
   image_set_font(img, font);
   image_set_colour(img, 0, 0, 0);
   image_set_text_align(img, 0, 0);
   image_text(img, WIDTH/2, TOP_MARGIN/2, "CODEC Loopback Frequency Spectrum");
   image_text(img, WIDTH/2, HEIGHT-BOTTOM_MARGIN/2, bottom_text);
   image_set_text_align(img, -1, 0);
   int h = HEIGHT-TOP_MARGIN-BOTTOM_MARGIN;
   image_text(img, LEFT_MARGIN, TOP_MARGIN+0*h/7, "0dB ");
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+1*h/14, 0,0,0);
   image_text(img, LEFT_MARGIN, TOP_MARGIN+1*h/7, "-20dB ");
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+2*h/14, 0,0,0);
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+3*h/14, 0,0,0);
   image_text(img, LEFT_MARGIN, TOP_MARGIN+2*h/7, "-40dB ");
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+4*h/14, 0,0,0);
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+5*h/14, 0,0,0);
   image_text(img, LEFT_MARGIN, TOP_MARGIN+3*h/7, "-60dB ");
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+6*h/14, 0,0,0);
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+7*h/14, 0,0,0);
   image_text(img, LEFT_MARGIN, TOP_MARGIN+4*h/7, "-80dB ");
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+8*h/14, 0,0,0);
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+9*h/14, 0,0,0);
   image_text(img, LEFT_MARGIN, TOP_MARGIN+5*h/7, "-100dB ");
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+10*h/14, 0,0,0);
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+11*h/14, 0,0,0);
   image_text(img, LEFT_MARGIN, TOP_MARGIN+6*h/7, "-120dB ");
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+12*h/14, 0,0,0);
   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) image_set_pixel(img, x, TOP_MARGIN+13*h/14, 0,0,0);
   image_text(img, LEFT_MARGIN, TOP_MARGIN+7*h/7, "-140dB ");

   for(int i = 0; i < count; i++) {
     double d = data[i];
     if(d < min) d = min;
     if(d > max) d = max;
     int x = (WIDTH-LEFT_MARGIN-RIGHT_MARGIN-1)*i/count+LEFT_MARGIN;
     int y = (HEIGHT-BOTTOM_MARGIN-1)-(HEIGHT-TOP_MARGIN-BOTTOM_MARGIN-1)*(d-min)/(max-min);

     if(i == 0) {
        last = y;
     } else {
        if(y>last) {
           while(last != y) {
              image_set_pixel(img, x,last, 255,0,0);
              last++;
           }
        } else {
           while(last != y) {
              image_set_pixel(img, x,last, 255,0,0);
              last--;
           } 
        }
     }
   }

   for(int x = LEFT_MARGIN; x < WIDTH-RIGHT_MARGIN; x++) {
      image_set_pixel(img, x, TOP_MARGIN, 0,0,0);
      image_set_pixel(img, x, TOP_MARGIN-1, 0,0,0);
      image_set_pixel(img, x, HEIGHT-BOTTOM_MARGIN, 0,0,0);
      image_set_pixel(img, x, HEIGHT-BOTTOM_MARGIN+1, 0,0,0);
   }
  
   for(int y = TOP_MARGIN; y < HEIGHT-BOTTOM_MARGIN; y++) {
      image_set_pixel(img, LEFT_MARGIN, y, 0,0,0);
      image_set_pixel(img, LEFT_MARGIN-1, y, 0,0,0);
      image_set_pixel(img, WIDTH-RIGHT_MARGIN, y, 0,0,0);
      image_set_pixel(img, WIDTH-RIGHT_MARGIN+1, y, 0,0,0);
   }
 
   image_write(img,"graph.ppm");
   image_free(img);
}

//=========================================================================================
void find_s_c(double *points, int point_count, double bin, double *st, double *ct, double *tt) {
   static double *s_table = NULL;
   static double *c_table = NULL;
   static int pc = 0;

   if(pc != point_count) {
      if(s_table) free(s_table);
      if(c_table) free(c_table);
      s_table = malloc(sizeof(double)*point_count);
      c_table = malloc(sizeof(double)*point_count);
      if(s_table == NULL || c_table == NULL) {
         fprintf(stderr,"Out of memory\n");
         exit(1);
      }
      for(int i = 0; i < point_count; i++) {
         double phase = i/((float)point_count)*2*M_PI;
         s_table[i] = sin(phase);
         c_table[i] = cos(phase);
      }
      pc = point_count;
   }

   double s = 0.0;
   double c = 0.0;
   double t = 0.0;
   int index = 0;
   for(int i = 0; i < point_count; i++) {
      s += points[i] * s_table[index];
      c += points[i] * c_table[index];
      t += points[i];
      index += bin;
      if(index >= point_count)
        index -= point_count;
   }
   *st = s/(point_count/2.0);
   *ct = c/(point_count/2.0);
   *tt = t/point_count;
}

void remove_tone(double *points, int point_count, double bin, double st, double ct, double tt, double *rms) {
   int i;
   *rms = 0.0;
   for(i = 0; i < point_count; i++) {
      double phase = i*bin/((double)point_count)*2*M_PI;
      double f = st*sin(phase) + ct*cos(phase) +tt;
      if(points[i] > f)
        *rms += points[i] -f;
      else
        *rms += f -points[i]; 
   }
   *rms /= point_count;
}

double *signal;

int analyze(double *points, int point_count) {
   int i = 0;
   double st,ct,tt;
   double rms = 0.0;

   printf("\nAnalysing captured data...\n");
   signal = malloc(sizeof(double)*point_count/2);

   if(signal == NULL) {
      fprintf(stderr,"Out of memory\n");
      return 0;
   }

   for(i = 0;i < point_count/2; i++) {
      find_s_c(points, point_count, i, &st, &ct, &tt);
      signal[i] = log(sqrt(st*st+ct*ct)/32768)/log(10)*20;
   }

   int max_bin = 0;
   for(i = 0;i < point_count/2; i++) { 
      if(signal[i] > signal[max_bin]) {
         max_bin = i;
      }
   }


   find_s_c(points, point_count, max_bin, &st, &ct, &tt);
   remove_tone(points, point_count, max_bin, st, ct, tt, &rms);
   printf("\n");
   double s =sqrt(st*st+ct*ct);
   printf("signal = %10.2f\n",s);
   printf("dc     = %10.2f\n",tt);
   printf("thd+n  = %10.2f  (%7.3f%%)\n",rms, rms/s*100);
   printf("s:n    = %10.2f dB\n",log(s/rms*s/rms)/log(10)*10);

   char text[100]; 
   sprintf(text,"thd+n %7.4f%%, peak %4.2f Hz", rms/s*100, (double)max_bin * actual_rate/point_count );
   plot(signal, point_count/2, text);
   free(signal);
   return 0;
}

int main( int argc, char *argv[] )
{
   int volume_pb  = 100;
   int volume_cap = 30;
   int points_to_cap = 12000;

   double *points;
   points = malloc(sizeof(double)*points_to_cap);
   if(points == NULL) {
      fprintf(stderr,"Out of memory\n");
      return 3;
   }

   if(argc >= 2) {
      volume_pb = atoi(argv[1]);
   }
   if(argc >= 3) {
      volume_cap = atoi(argv[2]);
   }
   if(!capture_data(points, points_to_cap, volume_pb, volume_cap))
      return 3;

   analyze(points, points_to_cap);
   free(points);
   return 0;
}
