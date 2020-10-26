#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>

#include "image.h"

struct image {
   int width;
   int height;
   int8_t h_align;
   int8_t v_align;
   struct image *font;
   int x,y;
   uint8_t r,g,b;
   uint8_t **data;
};

struct image *image_new(int w, int h) {
   struct image *img;
   int i;

   img = malloc(sizeof(struct image));
   if(img == NULL)
      return NULL;

   img->width  = w;
   img->height = h;

   img->x = 0;
   img->y = 0;
   img->h_align = 0;
   img->v_align = 0;

   img->r = 0;
   img->g = 0;
   img->b = 0;

   img->font = NULL;
   img->data = malloc(sizeof(char *)*h);
   if(img->data == NULL) {
      free(img);
      return NULL;
   }
 
   memset(img->data,0,sizeof(char *)*h);

   for(i = 0; i < h; i++) {
      img->data[i] = malloc(w*3);
      if(img->data[i] == NULL)
         break;
      memset(img->data[i],255,w*3);
   }
   if(i != h) {
      for(i = 0; i < h; i++) {
         if(img->data[i] != NULL) {
            free(img->data[i]);
         }
      }
      free(img->data);
      free(img);
      return NULL;
   }
   return img;
}

void image_set_font(struct image *img, struct image *font) {
   img->font = font;
}

void image_set_pos(struct image *img, int x, int y) {
   img->x = x;
   img->y = y;
}

void image_set_colour(struct image *img, uint8_t r, uint8_t g, uint8_t b) {
   img->r = r;
   img->g = g;
   img->b = b;
}
void image_set_pixel(struct image *img, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
   if(y <= 0) return;
   if(x <= 0) return;
   if(x >= img->width) return; 
   if(y >= img->height) return; 
   img->data[y][(x)*3+0] = r;
   img->data[y][(x)*3+1] = g;
   img->data[y][(x)*3+2] = b;

}
void image_rectangle(struct image *img, int x, int y, int w, int h) {
   int i,j;

   if(y+h <= 0) return;
   if(x+w <= 0) return;
   if(x > img->width) return; 
   if(y > img->height) return; 

   if(x < 0) {
      w += x;
      x = 0;
   }
   if(x+w > img->width) {
     w = img->width -x;
   }
   if(y+h > img->height) {
     h = img->height -y;
   }

   if(y < 0) {
      h += y;
      y = 0;
   }

   for(j = 0; j < h; j++) {
      for(i = 0; i < w; i++) {
         img->data[j+y][(x+i)*3+0] = img->r;
         img->data[j+y][(x+i)*3+1] = img->g;
         img->data[j+y][(x+i)*3+2] = img->b;
      }
   }
}

int image_write(struct image *img, char *fname) {
  int i;
  FILE *f;

  if(img == NULL) {
     return 0;
  }

  if(img->data == NULL) {
     return 0;
  }

  for(i = 0;i < img->height; i++) {
     if(img->data[i] == NULL) {
        return 0;
     }
  }

  f = fopen(fname,"w");
  if(f == NULL) {
     return 0;
  }

  fprintf(f,"P6\n%i %i\n255\n", img->width, img->height);
  for(i = 0;i < img->height; i++) {
     if(fwrite(img->data[i],3,img->width,f) != img->width) {
        fclose(f);
        return 0;
     }
  }

  fclose(f);
  return 1;
}
void image_free(struct image *img) {
   int i;
   if(img->data != NULL) {
      for(i = 0; i < img->height; i++) {
         if(img->data[i] != NULL) {
            free(img->data[i]);
         }
      }
      free(img->data);
   }
}

static int whitespace(char c) {
  if(c == ' ') return 1;
  if(c == '\t') return 1;
  if(c == '\r') return 1;
  if(c == '\n') return 1;
  return 0;
}

static int digit(char c) {
   return (c >= '0' && c <= '9');
}

struct image *image_from_ppm(char *file_name) {
   FILE *file;
   struct image *img;
   int c, last_c = 0;
   int width = 0, height = 0, maxval = 0;

   file = fopen(file_name, "r");
   if(file == NULL) {
     fprintf(stderr,"File %s not able to be opened\n", file_name);
     return 0;
   }

   c = getc(file);
   if(c != 'P') {
     goto format_error;
   }

   c = getc(file);
   if(c != '6') {
     goto format_error;
   }

   c = getc(file);
   while(whitespace(c)) {
     last_c = c;
     c = getc(file);
     if(c == '#' && last_c == '\n') {
       c = getc(file);
       while(c != '\n' && c != EOF) {
         c = getc(file);
       }
     }
   }

   if(!digit(c)) {
     goto format_error;
   }

   while(digit(c)) {
     width = (width*10)+(c-'0');
     c = getc(file);
   }

   while(whitespace(c)) {
     last_c = c;
     c = getc(file);
     if(c == '#' && last_c == '\n') {
       c = getc(file);
       while(c != '\n' && c != EOF) {
         c = getc(file);
       }
     }
   }

   if(!digit(c)) {
     goto format_error;
   }

   while(digit(c)) {
     height = (height*10)+(c-'0');
     c = getc(file);
   }

   while(whitespace(c)) {
     last_c = c;
     c = getc(file);
     if(c == '#' && last_c == '\n') {
       c = getc(file);
       while(c != '\n' && c != EOF) {
         c = getc(file);
       }
     }
   }

   if(!digit(c)) {
     goto format_error;
   }

   while(digit(c)) {
     maxval = (maxval*10)+(c-'0');
     c = getc(file);
   }

   if(!whitespace(c)) {
     goto format_error;
   }
   
   if(maxval != 255) {
     goto format_error;
   }
   img = image_new(width,height);

   if(img == NULL) {
     goto img_error;
   }
   for(int i = 0; i < height; i++) {
      if(fread(img->data[i],3, width,file) != width) {
         goto read_error;
      }
   }
   fclose(file);
   return img;

read_error:
   image_free(img);
   fprintf(stderr,"Error reading data in %s\n", file_name);
   fclose(file);
   return 0;

img_error:
   fprintf(stderr,"Unable to create image %s\n", file_name);
   fclose(file);
   return NULL;

format_error:
   fprintf(stderr,"File format error\n"); 
   fclose(file);
   return 0;
}

int char_write(struct image *img, char c) {
   int char_width, char_height;
   int fx, fy, dx, dy;

   if(img->font == NULL) {
      return 1;
   }
   char_width  = img->font->width/16;
   char_height = img->font->height/6;
   // Test to see if off the page //
   if(img->y >= img->height || img->x >= img->width || img->y <= -char_height || img->x <= -char_width) {
      return 1;
   }
   if(c < 32) c = ' ';
   if(c >= 127) c = ' ';

   c -=32;
   fx = (c%16)*char_width;
   fy = (c/16)*char_height;

   for(dy = 0; dy < img->font->height/6 && img->y+dy < img->height; dy++) {
      if(img->y+dy >= 0) {
         int copy_w = char_width;
         int offset = 0;
     
         if(img->x+copy_w > img->width) {
            copy_w = img->width - img->x;
         }
         if(img->x < 0) { 
            offset = -img->x;
            copy_w -= offset;
         }
         for(dx = 0; dx < copy_w; dx++) {
           uint8_t mix_r = img->font->data[fy+dy][3*(fx+offset+dx)+0];
           uint8_t mix_g = img->font->data[fy+dy][3*(fx+offset+dx)+1];
           uint8_t mix_b = img->font->data[fy+dy][3*(fx+offset+dx)+2];
           int tx = img->x+offset+dx;
           img->data[img->y+dy][3*tx+0] = (img->data[img->y+dy][3*tx+0] * mix_r + img->r * (255-mix_r))/255;
           img->data[img->y+dy][3*tx+1] = (img->data[img->y+dy][3*tx+1] * mix_g + img->g * (255-mix_g))/255;
           img->data[img->y+dy][3*tx+2] = (img->data[img->y+dy][3*tx+2] * mix_b + img->b * (255-mix_b))/255;
         }
      }
   }
   return 1;
}

void image_set_text_align(struct image *img, int h_align, int v_align) {
  img->h_align = h_align;
  img->v_align = v_align;
}

int image_text(struct image *img, int x, int y, char *text) {
   int cur_x = x;
   int this_width = 0;
   int width, height;
   if(img->font == NULL) {
      return 0;
   }

   // Text alignment
   this_width = 0;
   height   = 1; 
   width    = 0;
   for(int i = 0; text[i] != '\0'; i++) {
      if(text[i] == '\n') {
        height++;
        this_width = 0;
      } else {
        this_width++;
      }
      if(this_width > width)
         width = this_width;
   }
   width  *= img->font->width/16;
   height *= img->font->height/6;

   switch( img->h_align) {
      case -1:
         cur_x -= width;
         break;
      case  0:
         cur_x -= width/2;
         break;
      default:
         break;
   }

   switch( img->v_align) {
      case -1:
         y -= height;
         break;
      case  0:
         y -= height/2;
         break;
      default:
         break;
   }

   while(*text) {
      if(*text == '\n') {
         cur_x = x;
         y += img->font->height/6;
      } else {
        image_set_pos(img, cur_x, y);
        char_write(img, *text);
        cur_x += img->font->width/16;
      }
      text++;
   }
   return 1;
}
