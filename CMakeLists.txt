cmake_minimum_required(VERSION 3.28)
project(V3drawImageSampling)

set(CMAKE_CXX_STANDARD 20)

add_executable(V3drawImageSampling
        basic_4dimage.cpp
        basic_4dimage.h
        basic_memory.cpp
        basic_memory.h
        FL_coordDefinition.h
        FL_downSample3D.h
        FL_interpolateCoordCubic3D.h
        FL_interpolateCoordLinear3D.h
        FL_upSample3D.h
        img_definition.h
        main.cpp
        mg_image_lib.cpp
        mg_image_lib.h
        mg_utilities.cpp
        mg_utilities.h
        stackutil.cpp
        stackutil.h
        v3d_basicdatatype.h
)
