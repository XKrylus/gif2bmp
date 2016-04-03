/*
 * Soubor:  main.c
 * Datum:   2015/02/20
 * Autor:   Jan Kryl, xkrylj00@stud.fit.vutbr.cz
 * Projekt: Konverze obrazoveho formatu GIF na BMP
 * Popis:   Aplikace pro prevod obrazoveho
 *          formatu GIF na soubor grafickeho formatu BMP
 *
  */

#define __STDC_FORMAT_MACROS
#include <iostream>
#include <inttypes.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include "gif2bmp.h"

/**
* Napoveda
*/
const char *HELPMSG =
  "Program 'Konverze obrazoveho formatu GIF na BMP'\n"
  "Autor: Jan Kryl, xkrylj00@stud.fit.vutbr.cz\n"
  "Aplikace pro prevod obrazoveho formatu GIF na soubor grafickeho formatu BMP\n"
  "Pouziti:\n"
  "gif2bmp -help \\ -h / vytiskne napovedu \n"
  "gif2bmp -i FILE -o FILE -l FILE\n"
  "                     -i nazev vstupniho souboru, je-li nezadan, vstupem je obsah stdin\n"
  "                     -o nazev vystupniho souboru, je-li nezadan, vysutpen je obsah stdout\n"
  "                     -l nazev souboru vystupni zpravy, je-li nezadan, vypis zpravy ignorovan\n";

// ************ CHYBOVA HLASENI *************
/** Kódy chyb programu */
enum  tecodes{
        EOK = 0,        /**< Bez chyby */
        EINNOTFILE,     /**< Chybne zadany vstupni soubor */
        EOUTNOTFILE,    /**< Chybne zadany vystupni soubor */
        ELOGNOTFILE,    /**< Chybne zadany soubor pro vystupni zpravu */
        ECLWRONG,       /**< Chybne zadane parametry. */
        EUNKNOWN        /**< Neznama chyba */
};

/** Chybova hlaseni odpovidajici chybovym kodum. */
const char *ECODEMSG[] = {
        [EOK] = "OK",
        [EINNOTFILE] = "Chybne zadany vstupni soubor.\n",
        [EOUTNOTFILE] = "Chybne zadany vystupni soubor.\n",
        [ELOGNOTFILE] = "Chybne zadany soubor pro vystupni zpravu.\n",
        [ECLWRONG] = "Chybne zadane parametry.\n",
        [EUNKNOWN] = "Neznama chyba.\n"
};

// **************** PARAMETRY ***************

/** Stavove kody programu */
enum tstates
{
    CONV,       /**< konverze */
    HELP,       /**< vypis napovedy */
};

/**
 * Vytiskne hlaseni odpovidajici chybovemu kodu
 * @param ecode kod chyby programu
 */
void printECode(uint8_t ecode)
{
  if (ecode < EOK || ecode > EUNKNOWN)
  { ecode = EUNKNOWN; }

  fprintf(stderr, "%s", ECODEMSG[ecode]);
}

/**
 * Struktura obsahujici hodnoty parametru prikazove radky
 */
typedef struct params
{
  uint8_t N;
  uint8_t ecode;        /**< Chybovy kod programu, odpovida vyctu tecodes. */
  uint8_t state;        /**< Stavovy kod programu, odpovida vyctu tstates. */

  FILE* ifile;      /**< Nazev vstupniho souboru. */
  FILE* ofile;      /**< Nazev vystupniho souboru. */
  FILE* logfile;    /**< Nazev souboru vystupni zpravy. */

} TParams;

/**
 * Zpracuje argumenty prikazoveho radku a vrati je ve strukture TParams.
 * Pokud je format argumentu chybny, ukonci program s chybovym kodem.
 * @param argc Pocet argumentu.
 * @param argv Pole textovych reezcu s argumenty.
 * @return Vraci analyzovane argumenty prikazoveho radku.
 */
TParams getParams(uint16_t argc, char *argv[])
{
  TParams result =
  { // inicializace struktury
    .N = 0,
    .ecode = EOK,
    .state = CONV,
    .ifile = fopen("/dev/stdin", "r"),
    .ofile = fopen("/dev/stdout", "w"),
    .logfile = NULL,
  };


  if (argc == 2 && strcmp("-h", argv[1]) == 0) {
      result.state = HELP;
  }
  else if(argc >= 2) {

    uint32_t c;
    while((c = getopt(argc, argv, "hi:o:l:")) != -1) {
        switch(c) {
        case 'h':
            result.state = HELP;
            break;
        case 'i':
            fclose(result.ifile);
            result.ifile = fopen(optarg, "rb");
            if(result.ifile == NULL){printECode(EINNOTFILE);result.ecode = ECLWRONG;exit(1);}
            //setbuf(result.ifile, NULL);
            break;
        case 'o':
            fclose(result.ofile);
            result.ofile = fopen(optarg, "wb");
            if(result.ofile == NULL){printECode(EOUTNOTFILE);result.ecode = ECLWRONG;exit(1);}
            //setbuf(result.ofile, NULL);
            break;
        case 'l':
            result.logfile = fopen(optarg, "w");
            if(result.logfile == NULL){printECode(ELOGNOTFILE);result.ecode = ECLWRONG;exit(1);}
            //setbuf(result.logfile, NULL);
            break;
        case ':':
            fprintf(stderr,"Parametru '-%c' chybi argument!\n", optopt);exit(1);
            break;
        case '?':
            printECode(ECLWRONG);
            exit(1);
            break;
        }
    }
  }
  else {
    // prilis mnoho parametru
    if(argc != 1)result.ecode = ECLWRONG;
  }

  return result;
}

void generateLog(FILE *logfile, tGIF2BMP gif2bmpOUT) {
    fprintf(logfile, "login = xkrylj00\nuncodedSize = %" PRIu64 "\ncodedSize = %" PRIu64 "", gif2bmpOUT.gifSize, gif2bmpOUT.bmpSize);
}


int main(uint32_t argc, char *argv[] )
{
    tGIF2BMP gif2bmpOUT;
    //Ziska parametry
    TParams params = getParams(argc, argv);

    //Kontrola parametru
    if(params.ecode == ECLWRONG){printECode(ECLWRONG);return 1;}

    if(params.state == HELP){printf(HELPMSG); return 0;}

    //FILE* printX = fopen("testSampleSmall.txt", "wb");
    //getGIF(params.ifile, printX);

    //Konverze/*
    if(gif2bmp(&gif2bmpOUT, params.ifile, params.ofile) != 1) {
        //Vystupni zprava
        if(params.logfile != NULL) {
                generateLog(params.logfile, gif2bmpOUT);
    }
    }

    return 0;
}

