/*
 * Soubor:  gif2bmp.h
 * Datum:   2015/02/20
 * Autor:   Jan Kryl, xkrylj00@stud.fit.vutbr.cz
 * Projekt: Konverze obrazoveho formatu GIF na BMP
 * Popis:   Aplikace pro prevod obrazoveho
 *          formatu GIF na soubor grafickeho formatu BMP
 *
  */

#define __STDC_FORMAT_MACROS
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sstream>
#include <iostream>
#include <inttypes.h>
#include <math.h>
#include <bitset>

using namespace std;

//Main structure (outputs size of images)
typedef struct {
	int64_t bmpSize;
	int64_t gifSize;
} tGIF2BMP;

//Color Map structure (global AND local)
typedef struct {
    uint8_t rgb[3];
} tCOLORMAP;

enum color{RED, GREEN, BLUE};

//Gif structure
typedef struct {
    //Header
	uint8_t signature[3];
	uint8_t version[3];
	//Width, Hegith
	uint16_t sWidth;
	uint16_t sHeight;
	//Pixel Bits(Logical Screen Descriptor)
	uint8_t pixels;
	//Pixel Bits(Image Descriptor)
	uint8_t localPixels;
	uint16_t sizeLZW;
	//Color map (global OR local)
	vector<tCOLORMAP> colorMap;
	//Actuall image data
	vector<uint16_t> data;
	int64_t size;
} tGIF;

//Main function
int32_t gif2bmp(tGIF2BMP *gif2bmp, FILE *inputFile, FILE *outputFile);

//Functions for conversion
unsigned short getHexConvert(uint8_t num1, uint8_t num2);
unsigned getHex(uint8_t i);
string getDecConvert(uint16_t num);
uint32_t getBinConvert(string str);
void process(uint32_t cislo,uint8_t limit, FILE *bmp);
