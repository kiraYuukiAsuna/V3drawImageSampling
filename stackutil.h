#pragma once

#include "v3d_basicdatatype.h"

typedef char BIT8_UNIT;
typedef short int BIT16_UNIT;
typedef int BIT32_UNIT;
//typedef V3DLONG BIT64_UNIT;

int loadMRC2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype); //simple MRC reading
int saveStack2MRC(const char* filename, const unsigned char* img, const V3DLONG* sz, int datatype); //simple MRC writing

int loadRaw5d2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype); //4-byte raw reading
int loadRaw5d2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype, int stack_id_to_load);

//overload for convenience to read only 1 stack in a time series
int saveStack2Raw5d(const char* filename, const unsigned char* img, const V3DLONG* sz, int datatype);

//4-byte raw writing


int loadRaw2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype); //4-byte raw reading
int loadRaw2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype, int chan_id_to_load);

//overload for convenience to read only 1 channel
int saveStack2Raw(const char* filename, const unsigned char* img, const V3DLONG* sz, int datatype); //4-byte raw writing

int loadRaw2Stack_2byte(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype);

int loadRaw2Stack_2byte(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype, int chan_id_to_load);

//overload for convenience to read only 1 channel
int saveStack2Raw_2byte(const char* filename, const unsigned char* img, const V3DLONG* sz, int datatype);

void swap2bytes(void* targetp);

void swap4bytes(void* targetp);

char checkMachineEndian();


//use libtiff to read tiff files. MUST < 2G
int loadTif2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype);

int loadTif2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype, int chan_id_to_load);

//overload for convenience to read only 1 channel
int saveStack2Tif(const char* filename, const unsigned char* img, const V3DLONG* sz, int datatype);

//the following two functions are the major routines to load LSM file using libtiff, the file should have a size < 2G
int loadLsm2Stack_obsolete(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype); //070806
int loadLsm2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype); //070806
int loadLsm2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype, int chan_id_to_load);

//overload for convenience to read only 1 channel

//read LSM thumbnail. 070819
int loadLsmThumbnail2Stack(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype);

int loadLsmThumbnail2Stack_middle(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype);

//a more comprehensive way to read LSM file. 070823
int loadLsmSlice(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype, V3DLONG sliceno, bool b_thumbnail);

int loadRawSlice(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype, V3DLONG sliceno, bool b_thumbnail);

int loadRawSlice_2byte(char* filename, unsigned char* &img, V3DLONG* &sz, int&datatype, V3DLONG sliceno,
                       bool b_thumbnail);

//

const char* getSuffix(const char* filename);

//note that no need to delete the returned pointer as it is actually a location to the "filename"

bool ensure_file_exists_and_size_not_too_big(char* filename, V3DLONG sz_thres);

bool loadImage(char imgSrcFile[], unsigned char*&data1d, V3DLONG* &sz, int&datatype); //070215
bool loadImage(char imgSrcFile[], unsigned char*&data1d, V3DLONG* &sz, int&datatype, int chan_id_to_load); //081204
bool saveImage(const char filename[], const unsigned char* data1d, const V3DLONG* sz, const int datatype); //070214
