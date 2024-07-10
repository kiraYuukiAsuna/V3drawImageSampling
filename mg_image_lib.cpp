#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

#ifdef _MSC_VER
#include <windows.h>
void sleep(unsigned int seconds)
{
    Sleep(1000 * seconds);
}
#else
#include <unistd.h>
#endif

#include "mg_utilities.h"
#include "mg_image_lib.h"

static int Warnings = 1;

static int error(const char *msg, const char *arg)
{ fprintf(stderr,"\nError in TIFF library:\n   ");
  fprintf(stderr,msg,arg);
  fprintf(stderr,"\n");
  return 1;
}

/*********** STACK PLANE SELECTION ****************************/

Stack_Plane *Select_Plane(Stack *a_stack, int plane)  // Build an image for a plane of a stack
{ static Stack_Plane My_Image;

  if (plane < 0 || plane >= a_stack->depth)
    return (NULL);
  My_Image.kind   = a_stack->kind;
  My_Image.width  = a_stack->width;
  My_Image.height = a_stack->height;
  My_Image.array  = a_stack->array + plane*a_stack->width*a_stack->height*a_stack->kind;
  return (&My_Image);
}

/*********** SPACE MANAGEMENT ****************************/

// Raster working buffer

static uint32_t *get_raster(int npixels, const char *routine)
{ static uint32_t *Raster = NULL;
  static int     Raster_Size = 0;                           //  Manage read work buffer

  if (npixels < 0)
    { free(Raster);
      Raster_Size = 0;
      Raster      = NULL;
    }
  else if (npixels > Raster_Size)
    { Raster_Size = npixels;
      Raster = (uint32_t *) Guarded_Realloc(Raster,sizeof(uint32_t)*Raster_Size,routine);
    }
  return (Raster);
}

// Awk-generated (manager.awk) Image memory management

static inline int image_asize(Image *image)
{ return (image->height*image->width*image->kind); }

//  Image-routines: new_image, pack_image (Free|Kill)_Image, reset_image, Image_Usage

typedef struct __Image
  { struct __Image *next;
    int             asize;
    Image           image;
  } _Image;

static _Image *Free_Image_List = NULL;
static int    Image_Offset, Image_Inuse;

static inline Image *new_image(int asize, const char *routine)
{ _Image *object;

  if (Free_Image_List == NULL)
    { object = (_Image *) Guarded_Malloc(sizeof(_Image),routine);
      Image_Offset = ((char *) &(object->image)) - ((char *) object);
      object->asize = asize;
      object->image.array = (uint8_t *)Guarded_Malloc(asize,routine);
      Image_Inuse += 1;
    }
  else
    { object = Free_Image_List;
      Free_Image_List = object->next;
      if (object->asize < asize)
        { object->asize = asize;
          object->image.array = (uint8_t *)Guarded_Realloc(object->image.array,
                                                asize,routine);
        }
    }
  return (&(object->image));
}

inline void pack_image(Image *image)
{ _Image *object  = (_Image *) (((char *) image) - Image_Offset);
  if (object->asize != image_asize(image))
    { object->asize = image_asize(image);
      object->image.array = (uint8_t *)Guarded_Realloc(object->image.array,
                                            object->asize,"Pack_Image");
    }
}

void Free_Image(Image *image)
{ _Image *object  = (_Image *) (((char *) image) - Image_Offset);
  object->next = Free_Image_List;
  Free_Image_List = object;
  Image_Inuse -= 1;
}

void Kill_Image(Image *image)
{ free(image->array);
  free(((char *) image) - Image_Offset);
  Image_Inuse -= 1;
}

void reset_image()
{ _Image *object;
  while (Free_Image_List != NULL)
    { object = Free_Image_List;
      Free_Image_List = object->next;
      Kill_Image(&(object->image));
      Image_Inuse += 1;
    }
}

int Image_Usage()
{ return (Image_Inuse); }

Image *Copy_Image(Image *image)
{ Image  *copy = new_image(image_asize(image),"Copy_Image");

  copy->kind   = image->kind;
  copy->width  = image->width;
  copy->height = image->height;
  memcpy(copy->array,image->array,image_asize(image));
  return (copy);
}

void Pack_Image(Image *image)
{ pack_image(image); }

void Reset_Image()
{ reset_image();
  get_raster(-1,NULL);
}

// Awk-generated (manager.awk) Stack memory management

static inline int stack_vsize(Stack *stack)
{ return (stack->depth*stack->height*stack->width*stack->kind); }

//  Stack-routines: new_stack, pack_stack (Free|Kill)_Stack, reset_stack, Stack_Usage

typedef struct __Stack
  { struct __Stack *next;
    int             vsize;
    Stack           stack;
  } _Stack;

static _Stack *Free_Stack_List = NULL;
static int    Stack_Offset, Stack_Inuse;

static inline Stack *new_stack(int vsize, const char *routine)
{ _Stack *object;

  if (Free_Stack_List == NULL)
    { object = (_Stack *) Guarded_Malloc(sizeof(_Stack),routine);
      Stack_Offset = ((char *) &(object->stack)) - ((char *) object);
      object->vsize = vsize;
      object->stack.array = (uint8_t *)Guarded_Malloc(vsize,routine);
      Stack_Inuse += 1;
    }
  else
    { object = Free_Stack_List;
      Free_Stack_List = object->next;
      if (object->vsize < vsize)
        { object->vsize = vsize;
          object->stack.array = (uint8_t *)Guarded_Realloc(object->stack.array,
                                                vsize,routine);
        }
    }
  return (&(object->stack));
}

inline void pack_stack(Stack *stack)
{ _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);
  if (object->vsize != stack_vsize(stack))
    { object->vsize = stack_vsize(stack);
      object->stack.array = (uint8_t *)Guarded_Realloc(object->stack.array,
                                            object->vsize,"Pack_Stack");
    }
}

void Free_Stack(Stack *stack)
{ _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);
  object->next = Free_Stack_List;
  Free_Stack_List = object;
  Stack_Inuse -= 1;
}

void Kill_Stack(Stack *stack)
{ free(stack->array);
  free(((char *) stack) - Stack_Offset);
  Stack_Inuse -= 1;
}

void reset_stack()
{ _Stack *object;
  while (Free_Stack_List != NULL)
    { object = Free_Stack_List;
      Free_Stack_List = object->next;
      Kill_Stack(&(object->stack));
      Stack_Inuse += 1;
    }
}

int Stack_Usage()
{ return (Stack_Inuse); }

Stack *Copy_Stack(Stack *stack)
{ Stack *copy = new_stack(stack_vsize(stack),"Copy_Stack");

  copy->kind   = stack->kind;
  copy->width  = stack->width;
  copy->height = stack->height;
  copy->depth  = stack->depth;
  memcpy(copy->array,stack->array,stack_vsize(stack));
  return (copy);
}

void Pack_Stack(Stack *stack)
{ pack_stack(stack); }

void Reset_Stack()
{ reset_stack();
  get_raster(-1,NULL);
}

/*********** READ + WRITE INTERFACE ****************************/

void Parse_Stack_Name(char *file_name, char **prefix, int *num_width, int *first_num)
{ static char *Prefix = NULL;
  static int   Prefix_Max = 0;

  char *s, *t, c;

  s = file_name + strlen(file_name) - 4;
  if (strcmp(s,".tif") != 0)
    error("1st file, %s, in stack does not have .tif extension",file_name);
  t = s;
  while (t > file_name && isdigit(t[-1]))
    t -= 1;
  if (s-t <= 0)
    error("No number sequence in stack file names %s",file_name);

  if (t-file_name > Prefix_Max)
    { Prefix_Max = (int)((t-file_name)*1.2 + 20);
      Prefix     = (char *) Guarded_Realloc(Prefix,Prefix_Max+1,"Parse_Stack_Name");
    }

  c = *t;
  *t = '\0';
  strcpy(Prefix,file_name);
  *t = c;

  *prefix    = Prefix;
  *num_width = s-t;
  *first_num = atoi(t);
}


/*********** COMPUTE RANGES AND SCALE IMAGES AND STACKS *********************/

//  Compute min and max values in 'array' of type 'kind' with 'length' elements

static Pixel_Range *compute_minmax(uint8_t *array, int kind, int length, int channel)
{ static Pixel_Range My_Range;
  uint16_t *array16;
  int     i, x;
  int     min, max;

  min = 0xFFFF;
  max = 0;
  if (kind == GREY16)
    { array16 = (uint16_t *) array;
      for (i = 0; i < length; i++)
        { x = array16[i];
          if (x < min)
            min = x;
          if (x > max)
            max = x;
        }
    }
  else
    { if (kind == COLOR)
        { length *= 3;
          array  += channel;
        }
      for (i = 0; i < length; i += kind)
        { x = array[i];
          if (x < min)
            min = x;
          if (x > max)
            max = x;
        }
    }

  My_Range.maxval = max;
  My_Range.minval = min;
  return (&My_Range);
}

Pixel_Range *Image_Range(Image *image, int channel)
{ return (compute_minmax(image->array,image->kind,image->width*image->height,channel)); }

Pixel_Range *Stack_Range(Stack *stack, int channel)
{ return (compute_minmax(stack->array,stack->kind,
                         stack->width*stack->height*stack->depth,channel));
}

//  Compute min and max values in 'array' of type 'kind' with 'length' elements

static void scale_values(uint8_t *array, int kind, int length, int channel,
                         double factor, double offset)
{ /* static Pixel_Range My_Range; */
  uint16_t *array16;
  int     i, x;

  if (kind == GREY16)
    { array16 = (uint16_t *) array;
      for (i = 0; i < length; i++)
        { x = factor*array16[i] + offset;
          array16[i] = x;
        }
    }
  else
    { if (kind == COLOR)
        { length *= 3;
          array += channel;
        }
      for (i = 0; i < length; i += kind)
        { x = factor*array[i] + offset;
          array[i] = x;
        }
    }
}

void Scale_Image(Image *image, int channel, double factor, double offset)
{ return (scale_values(image->array,image->kind,image->width*image->height,
                       channel,factor,offset));
}

void Scale_Stack(Stack *stack, int channel, double factor, double offset)
{ return (scale_values(stack->array,stack->kind,stack->width*stack->height*stack->depth,
                       channel,factor,offset));
}


/*********** CONVERT IMAGES AND STACKS  *********************/

static void translate(int skind, uint8_t *in8, int tkind, uint8_t *out8, int length)
{ uint16_t *in16, *out16;
  int     i, x, maxval; 
  double  c, scale;

  if (skind == GREY16)
    { maxval = compute_minmax(in8,skind,length,0)->maxval;
      if (maxval > 255)
        scale  = 255. / maxval;
      else
        scale  = 1.;
    }

  if (tkind > skind)
    { in8  += length*skind;
      out8 += length*tkind;
    }
  in16  = (uint16_t *) in8;
  out16 = (uint16_t *) out8;
    
     
  if (tkind == COLOR)
    if (skind == GREY)
      for (i = length; i > 0; i--)   // G->C
        { x = *--in8;
          *--out8 = x;
          *--out8 = x;
          *--out8 = x;
        }
    else
      for (i = length; i > 0; i--)   // G16->C
        { x = (*--in16) * scale;
          *--out8 = x;
          *--out8 = x;
          *--out8 = x;
        }
  else if (tkind == GREY16)
    if (skind == COLOR)
      for (i = length; i > 0; i--)   // C->G16
        { c  = .3 * (*in8++);
          c += .59 * (*in8++);
          c += .11 * (*in8++);
          *out16++ = (uint16_t) c;
        }
    else
      for (i = length; i > 0; i--)   // G->G16
        { *--out16 = *--in8; }
  else  // tkind == GREY
    if (skind == COLOR)
      for (i = length; i > 0; i--)   // C->G
        { c  = .3 * (*in8++);
          c += .59 * (*in8++);
          c += .11 * (*in8++);
          *out8++ = (uint8_t) c;
        }
    else
      for (i = length; i > 0; i--)   // G16->G
        { *out8++ = (*in16++) * scale; }
}

Image *Translate_Image(Image *image, int kind, int in_place)
{ int width, height;

  width  = image->width;
  height = image->height;

  if (in_place)
    { if (image->kind == kind)
        return (image);

      if (kind > image->kind)
        { _Image *object  = (_Image *) (((char *) image) - Image_Offset);
          if (object->asize < width * height * kind)
            { object->asize = width * height * kind;
              image->array  = (uint8_t *)Guarded_Realloc(image->array,object->asize,"Translate_Image");
            }
        }

      translate(image->kind,image->array,kind,image->array,width*height);

      image->kind = kind;

      return (image);
    }
  else
    { Image  *xlate;

      if (image->kind == kind)
        return (Copy_Image(image));

      xlate  = new_image(kind*width*height,"Translate_Image");
      xlate->width  = width;
      xlate->height = height;
      xlate->kind   = kind;

      translate(image->kind,image->array,kind,xlate->array,width*height);

      return (xlate);
    }
}

Stack *Translate_Stack(Stack *stack, int kind, int in_place)
{ int width, height, depth;

  width  = stack->width;
  height = stack->height;
  depth  = stack->depth;

  if (in_place)
    { if (stack->kind == kind)
        return (stack);

      if (kind > stack->kind)
        { _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);

          if (object->vsize < width * height * depth * kind)
            { object->vsize = width * height * depth * kind;
              stack->array  = (uint8_t *)Guarded_Realloc(stack->array,object->vsize,"Translate_Stack");
            }
        }

      translate(stack->kind,stack->array,kind,stack->array,width*height*depth);

      stack->kind = kind;

      return (stack);
    }
  else
    { Stack *xlate;

      if (stack->kind == kind)
        return (Copy_Stack(stack));

      xlate  = new_stack(kind*width*height*depth,"Translate_Stack");
      xlate->depth  = depth;
      xlate->width  = width;
      xlate->height = height;
      xlate->kind   = kind;

      translate(stack->kind,stack->array,kind,xlate->array,width*height*depth);

      return (xlate);
    }
}
