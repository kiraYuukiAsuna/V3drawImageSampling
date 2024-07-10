#include "stackutil.h"
#include "basic_4dimage.h"

#include <iostream>

typedef unsigned short int USHORTINT16;



void Image4DSimple::loadImage(const char* filename)
{
	return this->loadImage(filename, false); //default don't use MYLib
}

void Image4DSimple::loadImage(const char* filename, bool b_useMyLib)
{
	cleanExistData(); // note that this variable must be initialized as NULL.

	strcpy(imgSrcFile, filename);

	V3DLONG * tmp_sz = 0; // note that this variable must be initialized as NULL.
	int tmp_datatype = 0;
	int pixelnbits=1; //100817

    const char * curFileSuffix = getSuffix(imgSrcFile);
    //printf("The current input file has the suffix [%s]\n", curFileSuffix);

    if (curFileSuffix && (strcasecmp(curFileSuffix, "tif")==0 || strcasecmp(curFileSuffix, "tiff")==0 ||
        strcasecmp(curFileSuffix, "lsm")==0) ) //read tiff/lsm stacks
	{
            //printf("Image4DSimple::loadImage loading filename=[%s]\n", filename);

#if defined _WIN32


#else
		if (b_useMyLib)
		{
			std::cerr<<"Now try to use MYLIB to read the TIFF/LSM again...\n",0);
			if (loadTif2StackMylib(imgSrcFile, data1d, tmp_sz, tmp_datatype, pixelnbits))
			{
				std::cerr<<"Error happens in TIF/LSM file reading (using MYLIB). Stop. \n", false);
				b_error=1;
				return;
			}
			else
				b_error=0; //when succeed then reset b_error
		}
		else
		{
            //std::cerr<<"Now try to use LIBTIFF (slightly revised by PHC) to read the TIFF/LSM...\n",0);
            if (strcasecmp(curFileSuffix, "tif")==0 || strcasecmp(curFileSuffix, "tiff")==0)
			{
				if (loadTif2Stack(imgSrcFile, data1d, tmp_sz, tmp_datatype))
				{
					std::cerr<<"Error happens in TIF file reading (using libtiff). \n", false);
					b_error=1;
				}
			}
            else //if ( strcasecmp(curFileSuffix, "lsm")==0 ) //read lsm stacks
			{
				if (loadLsm2Stack(imgSrcFile, data1d, tmp_sz, tmp_datatype))
				{
					std::cerr<<"Error happens in LSM file reading (using libtiff, slightly revised by PHC). \n", false);
					b_error=1;
				}
			}
		}
               // printf("Image4DSimple::loadImage finished\n");

#endif

	}    



    else if ( curFileSuffix && strcasecmp(curFileSuffix, "mrc")==0 ) //read mrc stacks
	{
		if (loadMRC2Stack(imgSrcFile, data1d, tmp_sz, tmp_datatype))
		{
			std::cerr<<"Error happens in MRC file reading. Stop. \n";
			b_error=1;
			return;
		}
	}
#ifdef _ALLOW_WORKMODE_MENU_
    //else if ( curFileSuffix && ImageLoader::hasPbdExtension(QString(filename)) ) // read v3dpbd - pack-bit-difference encoding for sparse stacks
    else if ( curFileSuffix && strcasecmp(curFileSuffix, "v3dpbd")==0 ) // read v3dpbd - pack-bit-difference encoding for sparse stacks
    {
          std::cerr<<"start to try v3dpbd", 0);
        ImageLoaderBasic imageLoader;
        if (! imageLoader.loadRaw2StackPBD(imgSrcFile, this, false) == 0) {
            std::cerr<<"Error happens in v3dpbd file reading. Stop. \n", false);
            b_error=1;
            return;
        }
        // The following few lines are to avoid disturbing the existing code below
        tmp_datatype=this->getDatatype();
        tmp_sz=new V3DLONG[4];
        tmp_sz[0]=this->getXDim();
        tmp_sz[1]=this->getYDim();
        tmp_sz[2]=this->getZDim();
        tmp_sz[3]=this->getCDim();

        this->setFileName(filename); // PHC added 20121213 to fix a bug in the PDB reading.
    }
#endif
	else //then assume it is Hanchuan's Vaa3D RAW format
    {
		std::cerr<<"The data does not have supported image file suffix, -- now this program assumes it is Vaa3D's RAW format and tries to load it... \n";
		if (loadRaw2Stack(imgSrcFile, data1d, tmp_sz, tmp_datatype))
		{
			printf("The data doesn't look like a correct 4-byte-size Vaa3D's RAW file. Try 2-byte-raw. \n");
			if (loadRaw2Stack_2byte(imgSrcFile, data1d, tmp_sz, tmp_datatype))
			{
				std::cerr<<"Error happens in reading 4-byte-size and 2-byte-size Vaa3D's RAW file. Stop. \n";
				b_error=1;
				return;
			}
		}
	}

	//080302: now convert any 16 bit or float data to the range of 0-255 (i.e. 8bit)
	switch (tmp_datatype)
	{
		case 1:
			datatype = V3D_UINT8;
			break;

		case 2: //080824
			//convert_data_to_8bit((void *&)data1d, tmp_sz, tmp_datatype);
			//datatype = UINT8; //UINT16;
			datatype = V3D_UINT16;
			break;

		case 4:
			//convert_data_to_8bit((void *&)data1d, tmp_sz, tmp_datatype);
			datatype = V3D_FLOAT32; //FLOAT32;
			break;

		default:
			std::cerr<<"The data type is not UINT8, UINT16 or FLOAT32. Something wrong with the program, -- should NOT display this message at all. Check your program. \n";
			if (tmp_sz) {delete []tmp_sz; tmp_sz=0;}
			return;
	}

	sz0 = tmp_sz[0];
	sz1 = tmp_sz[1];
	sz2 = tmp_sz[2];
	sz3 = tmp_sz[3]; //no longer merge the 3rd and 4th dimensions

	/* clean all workspace variables */

	if (tmp_sz) {delete []tmp_sz; tmp_sz=0;}

	return;
}

void Image4DSimple::loadImage_slice(char filename[], bool b_useMyLib, V3DLONG zsliceno)
{
    cleanExistData(); // note that this variable must be initialized as NULL.

    strcpy(imgSrcFile, filename);

    V3DLONG * tmp_sz = 0; // note that this variable must be initialized as NULL.
    int tmp_datatype = 0;
    int pixelnbits=1; //100817

    const char * curFileSuffix = getSuffix(imgSrcFile);
    printf("The current input file has the suffix [%s]\n", curFileSuffix);

    if (curFileSuffix && (strcasecmp(curFileSuffix, "tif")==0 || strcasecmp(curFileSuffix, "tiff")==0 ||
        strcasecmp(curFileSuffix, "lsm")==0) ) //read tiff/lsm stacks
    {
           // printf("Image4DSimple::loadImage loading filename=[%s] slice =[%ld]\n", filename, zsliceno);

#if defined _WIN32


#else
        if (b_useMyLib)
        {
            if (loadTif2StackMylib_slice(imgSrcFile, data1d, tmp_sz, tmp_datatype, pixelnbits, zsliceno))
            {
                std::cerr<<"Error happens in TIF/LSM file reading (using MYLIB). Stop. \n", false);
                b_error=1;
                return;
            }
            else
                b_error=0; //when succeed then reset b_error
        }
        else
        {
            //std::cerr<<"Now try to use LIBTIFF (slightly revised by PHC) to read the TIFF/LSM...\n",0);
            if (strcasecmp(curFileSuffix, "tif")==0 || strcasecmp(curFileSuffix, "tiff")==0)
            {
                if (loadTifSlice(imgSrcFile, data1d, tmp_sz, tmp_datatype, zsliceno, false))
                {
                    std::cerr<<"Error happens in TIF file reading (using libtiff). \n", false);
                    b_error=1;
                }
            }
            else //if ( strcasecmp(curFileSuffix, "lsm")==0 ) //read lsm stacks
            {
                if (loadLsmSlice(imgSrcFile, data1d, tmp_sz, tmp_datatype, zsliceno, false))
                {
                    std::cerr<<"Error happens in LSM file reading (using libtiff, slightly revised by PHC). \n", false);
                    b_error=1;
                }
            }
        }
               // printf("Image4DSimple::loadImage finished\n");

#endif

    }

   /*
    else if ( curFileSuffix && strcasecmp(curFileSuffix, "mrc")==0 ) //read mrc stacks
    {
        if (loadMRC2Stack_slice(imgSrcFile, data1d, tmp_sz, tmp_datatype, layer))
        {
            std::cerr<<"Error happens in MRC file reading. Stop. \n", false);
            b_error=1;
            return;
        }
    }
#ifdef _ALLOW_WORKMODE_MENU_
    //else if ( curFileSuffix && ImageLoader::hasPbdExtension(QString(filename)) ) // read v3dpbd - pack-bit-difference encoding for sparse stacks
    else if ( curFileSuffix && strcasecmp(curFileSuffix, "v3dpbd")==0 ) // read v3dpbd - pack-bit-difference encoding for sparse stacks
    {
          std::cerr<<"start to try v3dpbd", 0);
        ImageLoaderBasic imageLoader;
        if (! imageLoader.loadRaw2StackPBD(imgSrcFile, this, false) == 0) {
            std::cerr<<"Error happens in v3dpbd file reading. Stop. \n", false);
            b_error=1;
            return;
        }
        // The following few lines are to avoid disturbing the existing code below
        tmp_datatype=this->getDatatype();
        tmp_sz=new V3DLONG[4];
        tmp_sz[0]=this->getXDim();
        tmp_sz[1]=this->getYDim();
        tmp_sz[2]=this->getZDim();
        tmp_sz[3]=this->getCDim();

        this->setFileName(filename); // PHC added 20121213 to fix a bug in the PDB reading.
    }
#endif
    else //then assume it is Hanchuan's Vaa3D RAW format
    {
        std::cerr<<"The data does not have supported image file suffix, -- now this program assumes it is Vaa3D's RAW format and tries to load it... \n", false);
        if (loadRaw2Stack_slice(imgSrcFile, data1d, tmp_sz, tmp_datatype, layer))
        {
            printf("The data doesn't look like a correct 4-byte-size Vaa3D's RAW file. Try 2-byte-raw. \n");
            if (loadRaw2Stack_2byte_slice(imgSrcFile, data1d, tmp_sz, tmp_datatype, layer))
            {
                std::cerr<<"Error happens in reading 4-byte-size and 2-byte-size Vaa3D's RAW file. Stop. \n", false);
                b_error=1;
                return;
            }
        }
    }

    */

    //Temporarily do nothing to read other single slice from other formats
    else
    {
        std::cerr<<"The single slice reading function is NOT available for other format at this moment.\n";
        b_error=1;
        return;
    }

    //080302: now convert any 16 bit or float data to the range of 0-255 (i.e. 8bit)
    switch (tmp_datatype)
    {
        case 1:
            datatype = V3D_UINT8;
            break;

        case 2: //080824
            //convert_data_to_8bit((void *&)data1d, tmp_sz, tmp_datatype);
            //datatype = UINT8; //UINT16;
            datatype = V3D_UINT16;
            break;

        case 4:
            //convert_data_to_8bit((void *&)data1d, tmp_sz, tmp_datatype);
            datatype = V3D_FLOAT32; //FLOAT32;
            break;

        default:
            std::cerr<<"The data type is not UINT8, UINT16 or FLOAT32. Something wrong with the program, -- should NOT display this message at all. Check your program. \n";
            if (tmp_sz) {delete []tmp_sz; tmp_sz=0;}
            return;
    }

    sz0 = tmp_sz[0];
    sz1 = tmp_sz[1];
    sz2 = tmp_sz[2];
    sz3 = tmp_sz[3]; //no longer merge the 3rd and 4th dimensions

    /* clean all workspace variables */

    if (tmp_sz) {delete []tmp_sz; tmp_sz=0;}

    return;
}


bool Image4DSimple::saveImage(const char filename[])
{
    if (!data1d || !filename)
	{
		std::cerr<<"This image data is empty or the file name is invalid. Nothing done.\n";
		return false;
	}
    
    V3DLONG mysz[4];
	mysz[0] = sz0;
	mysz[1] = sz1;
	mysz[2] = sz2;
	mysz[3] = sz3;

	int dt;
	switch (datatype)
	{
		case V3D_UINT8:  dt=1; break;
		case V3D_UINT16:  dt=2; break;
		case V3D_FLOAT32:  dt=4; break;
		default:
			std::cerr<<"The data type is unsupported. Nothing done.\n";
			return false;
			break;
	}

    return ::saveImage(filename, data1d, mysz, dt);
}


