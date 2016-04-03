/*
 * Soubor:  gif2bmp.c
 * Datum:   2015/02/20
 * Autor:   Jan Kryl, xkrylj00@stud.fit.vutbr.cz
 * Projekt: Konverze obrazoveho formatu GIF na BMP
 * Popis:   Aplikace pro prevod obrazoveho
 *          formatu GIF na soubor grafickeho formatu BMP
 *
  */

#include "gif2bmp.h"

//Main function
int32_t gif2bmp(tGIF2BMP *gif2bmp, FILE *inputFile, FILE *outputFile) {

    tGIF gif;
    vector<uint16_t> bmp;

    bool globalColorMap = false;
    bool localColorMap = false;
    bool endBlock = false;
    bool readData = false;

    //numerical constants
    uint16_t extBlock = 0x21;
    uint16_t commentBlock = 0xf9;
    uint16_t dataBlock = 0x2c;

    uint16_t bmpHead1 = 0x42;
    uint16_t bmpHead2 = 0x4d;

    uint16_t bmpValue1 = 0x18;
    uint16_t bmpValue2 = 0x28;
    uint16_t bmpValue3 = 0xb13;

    uint32_t colorPos;
    uint32_t i = 1;

    string dataString;

    //GIF size
    fseek(inputFile, 0L, SEEK_END);
    gif.size = ftell(inputFile);
    fseek(inputFile, 0L, SEEK_SET);

    //printf("SIZE OF GIF: %d\n", gifSize);

    vector<uint32_t> gifContent;
    uint32_t c = 0;
    while((c = fgetc(inputFile)) != EOF) {
        gifContent.push_back(c);
    }

    // LOAD GIF INTO STRUCTURE

    //Load header (standard values: GIF89a or GIF87a)
    gif.signature[0] = gifContent[0];
	gif.signature[1] = gifContent[1];
	gif.signature[2] = gifContent[2];
	gif.version[0] = gifContent[3];
	gif.version[1] = gifContent[4];
	gif.version[2] = gifContent[5];

    //shift -> Content - (Header)
    gifContent.erase(gifContent.begin(), gifContent.begin() + sizeof(gif.signature) + sizeof(gif.version));

    gif.sWidth = getHexConvert(gifContent[0], gifContent[1]);
	gif.sHeight = getHexConvert(gifContent[2], gifContent[3]);
	if(((gifContent[4] & 128) >> 7) == 1) globalColorMap = true;
	gif.pixels = (gifContent[4] & 7) + 1;

	uint16_t widthUpped = gif.sWidth * 3;

    //shift -> Content - (Header, Logical Screen Descriptor)
    //printf("LOG DESCRIPTION SIZE: %d\n", sizeof(gif.logDescription));
	gifContent.erase(gifContent.begin(), gifContent.begin() + 6 + 1);

    //Global Color Table Flag set -> Load Global Color Table
    if(globalColorMap) {
        uint16_t countIndexTable = 1 << gif.pixels;
        tCOLORMAP colorMap;
        colorPos = 0;
        for(uint16_t i = 0; i < countIndexTable; i++) {
            colorMap.rgb[RED] = gifContent[colorPos++];
            colorMap.rgb[GREEN] = gifContent[colorPos++];
            colorMap.rgb[BLUE] = gifContent[colorPos++];
            gif.colorMap.push_back(colorMap);
        }

        //shift -> Content - (Header, Logical Screen Descriptor, Global Color Map)
        gifContent.erase(gifContent.begin(), gifContent.begin() + ((1 << gif.pixels))*3);
	}

	uint32_t position = 0;
	//Subblocks, size defined in a value ahead of these blocks
	vector<uint16_t> subBlock;

	while(position < gif.size) {
        //0x21 -> start of additional information, like comments etc. not to be worked with in this convertor
        if(gifContent[position] == extBlock) {
            position++;
            if(gifContent[position] == commentBlock) {
                gifContent.erase(gifContent.begin(), gifContent.begin() + 8);
                position = 0;
            }
        }
        //0x2c -> start of data segment - consists of subblocks, with size set in value
        // at the start of those subblocks. subblock with size zero means data segment ends
        else if(gifContent[position] == dataBlock && !readData) {

            //Handling only single image (there may be more defined)
            readData = true;

            if(((gifContent[9] & 128) >> 7) == 1) localColorMap = true;
            gif.localPixels = (gifContent[9] & 7) + 1;
			gifContent.erase(gifContent.begin(), gifContent.begin() + 10);
			if(localColorMap) {

                uint16_t countIndexTable = 1 << gif.localPixels;
                //Global color is always used as color map - so if global and local color maps
                //  are set, we are interested in local color map value
                gif.colorMap.clear();
                tCOLORMAP colorMap;
                colorPos = 0;
                for(uint16_t i = 0; i < countIndexTable; i++) {
                    colorMap.rgb[RED] = gifContent[colorPos++];
                    colorMap.rgb[GREEN] = gifContent[colorPos++];
                    colorMap.rgb[BLUE] = gifContent[colorPos++];
                    gif.colorMap.push_back(colorMap);
                }

				gif.pixels = gif.localPixels;
				gifContent.erase(gifContent.begin(), gifContent.begin() + ((1 << gif.localPixels))*3);
			}

            //DATA PART
			gif.sizeLZW = gifContent[position++];

			uint32_t sizeBlock = gifContent[position++];

            //Writing blocks, until end is reached
			while(endBlock != true) {
				if(i <= sizeBlock) {
					subBlock.push_back(gifContent[position++]);
					i++;
				}
				else if(gifContent[position] == 0) {
					endBlock = true;
				}
				else {
					sizeBlock = sizeBlock + gifContent[position++];
				}
			}
        }
        if(readData) {
            position++;
        }
	}

	for(uint32_t i = 0; i < subBlock.size(); i++) {
		dataString.insert(0, getDecConvert(subBlock[i]));
	}
	getBinConvert(dataString.substr(dataString.size()-9,dataString.size()));

	vector<vector<int16_t> > dict;
	uint16_t dictSize = (1 << gif.pixels)+2;

	for(uint16_t i = 0; i < dictSize; i++) {
        dict.push_back(vector<int16_t>());
		dict[i].push_back(i);
	}
	uint8_t minBitSize = gif.sizeLZW + 1;
	uint16_t decode = 0;

	//RESERVED PHRASES - CC: clear code
	//                   EOI: end of input
	//always last two members of dictionary
	uint16_t codeCC = dict.size() - 2;
	uint16_t codeEOI = dict.size() - 1;

    //First phrase
	decode = getBinConvert(dataString.substr(dataString.size() - minBitSize, dataString.size()));
	dataString.erase(dataString.size()-minBitSize, dataString.size());

    //If First phrase is Clear Corde -> get next phrase
	if(decode == dict[codeCC][0]) {
		decode = getBinConvert(dataString.substr(dataString.size() - minBitSize, dataString.size()));
		dataString.erase(dataString.size() - minBitSize, dataString.size());
	}


	//Process first phrase
	vector<uint16_t> resultLZW;
	vector<uint16_t> prev;
	uint8_t last;
	prev.push_back(dict[decode][0]);
	resultLZW.push_back(dict[decode][0]);

    //Remainder
    colorPos = dictSize;
	uint16_t sizeMax = pow(2, minBitSize);

	while(dataString.size() > 0) {
		if(minBitSize > 12) minBitSize = 12;

		decode = getBinConvert(dataString.substr(dataString.size()-minBitSize, dataString.size()));
		dataString.erase(dataString.size()-minBitSize, dataString.size());
		if(decode == codeEOI) {
            //End of Input - end loading bytes
			break;
		}
		else if(decode == dict[codeCC][0]) {
            //Clear code - reinitiate
			minBitSize = gif.sizeLZW + 1;
			sizeMax = pow(2, minBitSize);
 			colorPos = dictSize;
			dict.clear();

            dictSize = (1 << gif.pixels)+2;

			for(uint32_t i = 0; i < dictSize; i++) {
                dict.push_back(vector<int16_t>());
				dict[i].push_back(i);
			}
            //Load 'First' phrase
			decode = getBinConvert( dataString.substr(dataString.size()-minBitSize, dataString.size()));
			dataString.erase(dataString.size()-minBitSize, dataString.size());

            //Process First phrase
			prev.clear();
			prev.push_back(dict[decode][0]);
			resultLZW.push_back(dict[decode][0]);
		}
		else {
		if(decode < (dict.size())) {
            //Print values from result
			for(uint16_t k = 0; k < dict[decode].size(); k++) {
				resultLZW.push_back(dict[decode][k]);
			}
			//Move first dict value
			last = dict[decode][0];
			dict.push_back(vector<int16_t>());
			for(uint16_t k = 0; k < prev.size(); k++) {
				dict[colorPos].push_back(prev[k]);
			}
			dict[colorPos].push_back(last);

            //Clear previous output holder, and store new old
			prev.clear();


			for(uint16_t k = 0; k < dict[decode].size(); k++) {
				prev.push_back(dict[decode][k]);
			}
		}
		else if(decode >= (dict.size())) {
			last = prev[0];
			dict.push_back(vector<int16_t>());
			for(uint16_t k = 0; k < prev.size(); k++) {
                dict[colorPos].push_back(prev[k]);
				resultLZW.push_back(prev[k]);
			}
			dict[colorPos].push_back(last);

            prev.push_back(last);
			resultLZW.push_back(last);

		}
		if(colorPos == sizeMax - 1) {
			minBitSize++;
			sizeMax = pow(2, minBitSize);

		}
		colorPos++;
		}
	}

	gif.data = resultLZW;

    //WRITE BMP
    vector<uint16_t> data;
    for(uint32_t i = 0; i < gif.data.size(); i++) {
		data.push_back(gif.colorMap[gif.data[i]].rgb[BLUE]);
		data.push_back(gif.colorMap[gif.data[i]].rgb[GREEN]);
		data.push_back(gif.colorMap[gif.data[i]].rgb[RED]);
	}

	int64_t x = data.size() - widthUpped;
	int64_t y = data.size();
	uint8_t align = (4 - (widthUpped % 4)) % 4;

	while(!(x < 0)) {
		for(int write = x; write < y; write++) {
			bmp.push_back(data[write]);
		}
		//Align output
		for(uint8_t aligning = 0; aligning < align; aligning++) {
			bmp.push_back(0);
		}

		y = y - widthUpped;
		x = y - widthUpped;
	}

	uint32_t buffer[1];

	fputc(bmpHead1, outputFile);
	fputc(bmpHead2, outputFile);

	buffer[0] = 14 + 40 + bmp.size();
    fwrite(buffer, sizeof(uint32_t), 1, outputFile);

    for(uint8_t i = 0; i < 4; i++) {
        fputc(0, outputFile);
    }
    fputc(bmpValue2, outputFile);
    for(uint8_t i = 0; i < 3; i++) {
        fputc(0, outputFile);
    }

    fputc(bmpValue2, outputFile);
    for(uint8_t i = 0; i < 3; i++) {
        fputc(0, outputFile);
    }

    process(gif.sWidth, 4, outputFile);
    process(gif.sHeight, 4, outputFile);

    fputc(1, outputFile);
    for(uint8_t i = 0; i < 1; i++) {
        fputc(0, outputFile);
    }

    fputc(bmpValue1, outputFile);
    for(uint8_t i = 0; i < 1; i++) {
        fputc(0, outputFile);
    }

    fputc(0, outputFile);
    for(uint8_t i = 0; i < 3; i++) {
        fputc(0, outputFile);
    }

    buffer[0] = bmp.size();
    fwrite(buffer, sizeof(uint32_t), 1, outputFile);

    fputc(bmpValue3, outputFile);
    for(uint8_t i = 0; i < 3; i++) {
        fputc(0, outputFile);
    }

    fputc(bmpValue3, outputFile);
    for(uint8_t i = 0; i < 3; i++) {
        fputc(0, outputFile);
    }

    fputc(0, outputFile);
    for(uint8_t i = 0; i < 3; i++) {
        fputc(0, outputFile);
    }

    fputc(0, outputFile);
    for(uint8_t i = 0; i < 3; i++) {
        fputc(0, outputFile);
    }

    gif2bmp->bmpSize = bmp.size();
    gif2bmp->gifSize = gif.size;

	for(uint32_t i = 0; i< bmp.size();i++) {
        process(bmp[i], 1, outputFile);
	}

    return 0;
}


unsigned getHex(uint8_t i) {
    uint32_t result = 0;
    uint16_t j = 1;
    while(i) {
        result += i % 10 * (pow(256, j));
        j++;
        i /= 10;
    }
    return result;
}

//Recalculates hexadecimal value to decimal from 2 bytes
unsigned short getHexConvert(uint8_t input, uint8_t inputx) {
	uint32_t i = 0;
	uint32_t value = 0;

	while(input) {
        if(input & 1) {
            value += pow(2,i);
        }
        input >>= 1;
        i++;
	}
	value += getHex(inputx);
	return value;
}

//Recalculates decimal value to bites
string getDecConvert(uint16_t input) {
    return std::bitset<8>(input).to_string();
}

//Recalculates bites value to decimal
uint32_t getBinConvert(string str) {

	char * ptr;
	char *y = new char[str.length() + 1];
	strcpy(y, str.c_str());
	return strtoull(y, &ptr, 2);
}

//Process given input in decimal to bites, in given size
void process(uint32_t input,uint8_t size, FILE *outputFile) {
	uint32_t value = 0;
	uint32_t i = 0;
	uint32_t c = 0;
	uint32_t sizeStored = 0;
	while(input) {
        if (input & 1) {
            value += pow (2, i);
        }
	    input >>= 1;
	     i++;
	    if(i >= 8) {
            c = value;
            fputc(c, outputFile);
            sizeStored++;
            i = 0;
            value = 0;
	    }
	}
	if(value != 0) {
        c = value;
        fputc(c, outputFile);
        sizeStored++;
	}
	c = 0;
	while(sizeStored < size) {
        fputc(c, outputFile);
        sizeStored++;
	}
}


