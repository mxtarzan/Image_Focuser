#!/bin/bash
g++ -lglut -lGL -lm -lpthread -g imageIO_TGA.cpp rasterImage.cpp gl_frontEnd.cpp main.cpp -o Prog
echo "To run: ./Prog /paths/to/all/images/*.tga /path/and/name/to/output/file.tga" 
