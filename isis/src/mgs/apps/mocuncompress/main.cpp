/*

==================================================
2003-APR-17 Glenn Bennett - US Geological Survey

Due to a conflict with XV - by John Bradley
- XV program also uses a version of this 'readmoc'
program ISIS system has renamed this file

  from: readmoc
  to:   readmocisis

an accompanying program has also been renamed:

  from: readmocdrv
  to:   readmocdrvisis

This will prevent problems encountered where two
packages have the same named underlying programs.
==================================================
==================================================
2003-OCT-15 Stuart Sides - US Geological Survey

Converted calls to "gets" to "fgets" to satisfy
of compiler warning about "gets" being unsafe. These
calls were used to get the input and output filenames.
Changing from "gets" to "fgets" also required a check
to make sure there was no '\n' at the end of the input
string.
==================================================
==================================================
2018-OCT-19 Kaitlyn Lee - US Geological Survey

Removed the register keyword because it is deprecated in C++17.
==================================================


NOTICE

The software accompanying this notice (the "Software") is provided to you
free of charge to facilitate your use of the data collected by the Mars
Orbiter Camera (the "MOC Data").  Malin Space Science Systems ("MSSS")
grants to you (either as an individual or entity) a personal,
non-transferable, and non-exclusive right (i) to use and reproduce the
Software solely for the purpose of accessing the MOC Data; (ii) to modify
the source code of the Software as necessary to maintain or adapt the
Software to run on alternate computer platforms; and (iii) to compile, use
and reproduce the modified versions of the Software solely for the purpose
of accessing the MOC Data.  In addition, you may distribute the Software,
including any modifications thereof, solely for use with the MOC Data,
provided that (i) you must include this notice with all copies of the
Software to be distributed; (ii) you may not remove or alter any
proprietary notices contained in the Software; (iii) you may not charge any
third party for the Software; and (iv) you will not export the Software
without the appropriate United States and foreign government licenses.

You acknowledge that no title to the intellectual property in the Software
is transferred to you.  You further acknowledge that title and full
ownership rights to the Software will remain the exclusive property of MSSS
or its suppliers, and you will not acquire any rights to the Software
except as expressly set forth above.  The Software is provided to you AS
IS.  MSSS MAKES NO WARRANTY, EXPRESS OR IMPLIED, WITH RESPECT TO THE
SOFTWARE, AND SPECIFICALLY DISCLAIMS THE IMPLIED WARRANTIES OF
NON-INFRINGEMENT OF THIRD PARTY RIGHTS, MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.  SOME JURISDICTIONS DO NOT ALLOW THE EXCLUSION OR
LIMITATION OF INCIDENTAL OR CONSEQUENTIAL DAMAGES, SO SUCH LIMITATIONS OR
EXCLUSIONS MAY NOT APPLY TO YOU.

Your use or reproduction of the Software constitutes your agreement to the
terms of this Notice.  If you do not agree with the terms of this notice,
promptly return or destroy all copies of the Software in your possession.

Copyright (C) 1999 Malin Space Science Systems.  All Rights Reserved.
*/
//static char *sccsid = "@(#)readmocisis.c  1.2 04/10/00";

/*
    SDP interpretation program
    Mike Caplinger, MOC GDS Design Scientist
    SCCS @(#)readmocisis.c  1.2 04/10/00

    Reads and decompresses MOC SDP files to create PDS images.
    Derived from the GDS readmsdp program.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "image_io.h"
#include "array.h"
#include "msdp.h"
#include "fs.h"
#include "predCompCommon.h"
#include "bitsOut.h"

#include "predictiveDecompressor.h"

#define DEFINE_CODE_TABLES

#include "predcode.h"


typedef struct ht_node {
  int value;
  struct ht_node *zero, *one;
} Huffman_node;

/* in TJL terminology, left is 0 and right is 1. */
extern uint8 code[], left[], right[];


extern FILE *write_header(int width, int height,
                          FILE *infile, char *outfname);
static FILE *out;
static FILE *infile;
int errors;
static int test_pred;
static int rawencode = 0;

static int verbose = 0;

static int frag_offset[128];

static char infname[256], outfname[256];
static int mbr = 0;

byte *decode(struct msdp_header h, byte *data, int datlen, int *len, int mbr);
void init_output(struct msdp_header h);
extern unsigned int CS8EACC2(unsigned char *dat, unsigned int len);

#define FRAGSIZE (256*1024)

static char decode_file[128];

static unsigned int moc_sync;

static char label[1024];

static int status;
#define STAT_SHORT 2
#define STAT_BADSEQ 4
#define STAT_BADCS 8

enum MocCompressEnum {
  RAW = 0, PRED, XFORM
} MocCompress = RAW;



extern void exit();

#define BUFFERSIZE  4096

/* Huffman tree (in table form) */
uint8 code[256];
uint8 left[256];
uint8 right[256];

int pred_past_eof;



void decodeLoad(char *decodefile)
{
  unsigned int decodeSize;
  FILE *fd;

  if((fd = fopen(decodefile, "r")) == NULL) {
    (void)fprintf(stderr, "Unable to open '%s' for reading\n",
                  decodefile);
    exit(1);
  }

  if(fread((char *)(&decodeSize), sizeof(decodeSize), 1, fd) != 1) {
    fprintf(stderr, "Unable to read decode file, '%s'\n", decodefile);
    exit(1);
  }

  if(fread(code, sizeof(code[0]), decodeSize, fd) != decodeSize) {
    fprintf(stderr, "Unable to read decode file, '%s'\n", decodefile);
    exit(1);
  }

  if(fread(left, sizeof(left[0]), decodeSize, fd) != decodeSize) {
    fprintf(stderr, "Unable to read decode file, '%s'\n", decodefile);
    exit(1);
  }

  if(fread(right, sizeof(right[0]), decodeSize, fd) != decodeSize) {
    fprintf(stderr, "Unable to read decode file, '%s'\n", decodefile);
    exit(1);
  }

  (void)fclose(fd);
}

Huffman_node *ht_insert(Huffman_node *root, int value, int code, int len)
{
  int bit;
  Huffman_node **branch;

  if(!root) {
    root = (Huffman_node *) malloc(sizeof(Huffman_node));
    root->value = 0;
    root->zero = root->one = NULL;
  }

  if(len == 0) {
    root->value = value;
  }
  else {
    bit = code & 0x1;
    if(bit == 0) branch = &root->zero;
    else branch = &root->one;

    if(*branch == 0) {
      *branch = (Huffman_node *) malloc(sizeof(Huffman_node));
      (*branch)->value = 0;
      (*branch)->zero = 0;
      (*branch)->one = 0;
    }
    ht_insert(*branch, value, code >> 1, len - 1);
  }
  return root;
}

int ht_lookup(Huffman_node *root, int code, int len)
{
  int bit;

  if(root->zero == 0 && root->one == 0) return root->value;
  bit = code & 1;
  if(bit == 0) return ht_lookup(root->zero, code >> 1, len - 1);
  else return ht_lookup(root->one, code >> 1, len - 1);
}

void ht_dump(Huffman_node *root, int code, int len)
{
  if(root->zero == 0 && root->one == 0) {
    printf("%d %x(%d)\n", root->value, code, len);
  }
  else {
    if(root->zero) {
      ht_dump(root->zero, code, len + 1);
    }
    if(root->one) {
      ht_dump(root->one, code | (1 << len), len + 1);
    }
  }
}

#define ZERO (1<<0)
#define ONE (1<<1)

/*
    Convert a Huffman tree to TJL table form. Call initially
    with index = 0.
*/
int ht_tablefy(Huffman_node *root, unsigned char *flags, unsigned char *zero, unsigned char *one, unsigned char *index)
{
  int local_index = (int)(long)(index);

  if(root->zero) {
    if(root->zero->zero == 0 && root->zero->one == 0) {
      flags[(int)(long)index] &= ~ZERO;
      zero[(int)(long)index] = root->zero->value;
    }
    else {
      flags[local_index] |= ZERO;
      index += 1;
      zero[local_index] = (char)(long)index;
      index = (unsigned char*)(long)ht_tablefy(root->zero, flags, zero, one, index);
    }
  }
  if(root->one) {
    if(root->one->zero == 0 && root->one->one == 0) {
      flags[local_index] &= ~ONE;
      one[local_index] = root->one->value;
    }
    else {
      flags[local_index] |= ONE;
      index += 1;
      one[local_index] = (char)(long)index;
      index = (unsigned char*)(long)ht_tablefy(root->one, flags, zero, one, index);
    }
  }
  return (int)(long)index;
}

Huffman_node *ht_tree_gen(int i)
{
  Huffman_node *tree = 0;
  uint16 *code;
  uint8 *len;
  uint8 *requant;

  code = CodeBitsVec[i];
  len = CodeLenVec[i];
  requant = CodeRequantVec[i];

  tree = ht_insert(tree, requant[0], code[0], len[0]);

  for(i = 1; i < 128; i++) {
    if(requant[i] != requant[i-1])
      tree = ht_insert(tree, requant[i], code[i], len[i]);
  }

  tree = ht_insert(tree, requant[255], code[255], len[255]);

  for(i = 254; i >= 128; i--) {
    if(requant[i] != requant[i+1])
      tree = ht_insert(tree, requant[i], code[i], len[i]);
  }
  return tree;
}

void ht_free(Huffman_node *root)
{
  if(root->zero) ht_free(root->zero);
  if(root->one) ht_free(root->one);
  free(root);
}

void decodeInit(int n) {
  Huffman_node *tree = 0;
//  uint8 flags[256], zero[256], one[256];

  tree = ht_tree_gen(n);
  /* i is the # of slots actually used... */
  ht_tablefy(tree, code, left, right, 0);
  ht_free(tree);
}


/*
    NOTE: the globals code, left, and right should be set up prior
    to calling this routine.
*/
uint8 *predictive_decomp_main(uint8 *data, int len, int height, int width,
                              uint8 doSync, uint16 sync, int xpred, int ypred, int *got_height)
{
  uint8 *curLine;     /* Current line */
  uint8 *prevLine;    /* Previous line */
  BITSTRUCT bitStuff;   /* Output bit stream structure */
  uint8 compType = 0;   /* Type of compression */
  //uint32 decodeSize;    /* Size of huffman tables */
  uint32 y;     /* Looping variable */
  //uint32 index;     /* Indexing variable */
  uint8 *result;
  uint8 *lastsync;
  uint16 gotsync;
  extern int errors;
  extern uint8 *findsync(uint8 * p, int len, uint16 sync);


  pred_past_eof = 0;

  /* Allocate space for decompression */
  if((prevLine = (uint8 *)malloc(width * sizeof(*prevLine))) == NULL) {
    (void)fprintf(stderr, "Unable to get enough memory for line buffers\n");
    exit(1);
  };

  for(y = 0; y < (unsigned)width; y++) {
    prevLine[y] = 0;
  };

  if((curLine = (uint8 *)malloc(width * sizeof(*curLine))) == NULL) {
    (void)fprintf(stderr, "Unable to get enough memory for line buffers\n");
    exit(1);
  };

  if((result = (uint8 *) malloc(height * width)) == NULL) {
    fprintf(stderr, "can't get memory for output image\n");
    exit(1);
  }

  if(xpred) compType |= XPRED;
  if(ypred) compType |= YPRED;

  bitStuff.output     = data;
  bitStuff.bitQueue = *(bitStuff.output);
  bitStuff.bitCount = 0;

  lastsync = data;

  for(y = 0; y < (unsigned)height; y++) {
    if((y % 128 == 0) && (doSync == 1)) {
      if(bitStuff.bitCount != 0) {
        bitStuff.bitCount = 0;
        bitStuff.output++;
      };
      if(((bitStuff.output - data) & 0x1) == 0x1) {
        bitStuff.output++;
      };
      bitStuff.bitQueue = *(bitStuff.output);
      /* check sync pattern:
         NOTE this is kind of a funny place to do this,
         but it's simplest to do it here due to the
         structure of the code. mc, 11/11/98 */
      gotsync = *(bitStuff.output) | (*(bitStuff.output + 1) << 8);
      if(gotsync != sync) {
        fprintf(stderr, "lost sync, line %d -- ", y);
        errors += 1;
#define AUTOSYNC
#ifdef AUTOSYNC
        if(!(lastsync = (uint8 *)findsync(lastsync,
                                          len - (lastsync - data), sync))) {
#else
        if(1) {
#endif
          *got_height = y;
          if(bitStuff.output - data > len) {
            // we tried to read beyond the end of the data.
            pred_past_eof = 1;
          }
          fprintf(stderr, "aborting\n");
          return result;
        }
        else {
          bitStuff.output = lastsync;
        }
      }
      else {
        lastsync = bitStuff.output;
      }

      predictiveDecompressor(curLine, prevLine, width, compType | SYNC, code, left, right, sync, &bitStuff);
    }
    else {
      predictiveDecompressor(curLine, prevLine, width, compType, code, left, right, sync, &bitStuff);
    };

    memmove(result + y * width, curLine, width);
  };

  // Free the temporary storage
  free((char *)prevLine);
  free((char *)curLine);

  *got_height = height;
  return result;
}

int main(int argc, char **argv)
{
  int height = 0, width = 0;
  pixel *frag;
  int actual_height;
  int total_image = 0;
  int total = 0;
  int cs_check = 1;
  int pad_cs = 0;
  int multi = 0;
  int sequence = -1, processor = 0, n_processors = 1;
  int last_frag = -1;

  moc_sync = 0xf0ca;

  if(argc < 3) {
    printf("\nEnter name of file to be decompressed: ");
    if(!fgets(infname, 256, stdin)) {
      fprintf(stderr, "Encountered error while tring to read name of "
              "file to be decompressed\n");
    }
    int inLen = strlen(infname) - 1;
    if(inLen == '\n') infname[inLen] = '\0';
    printf("\nEnter name of uncompressed output file: ");
    int outLen = strlen(outfname) - 1;
    if(outLen == '\n') outfname[outLen] = '\0';
    if(!fgets(outfname, 256, stdin)) {
      fprintf(stderr, "Encountered error while tring to read name of "
              "uncompressed output file\n");
    }
  }
  else {
    strcpy(infname, argv[1]);
    strcpy(outfname, argv[2]);
  }

  infile = fopen(infname, "r");
  if(infile == 0) {
    fprintf(stderr, "Can't open %s\n", argv[1]);
  }

  while(1) {
    int count;
    int len;
    static int first = 1;
    struct msdp_header h, lasth;
    int datlen;
    byte *indat = 0, *chunk = 0;

    lasth = h;
    fseek(infile, total + 2048, 0);
    count = fread(&h, sizeof(h), 1, infile);
    MAKELONG(h.len);
    if(count && MAKELONG(h.len) == 0) {
      // simulate the EOF even though there's padding
      count = 0;
      h = lasth;
    }
    if(count == 0 && MocCompress == PRED && (h.status & 2) == 0) {
      // image was short -- last flag missing
      h.status = 2;
      frag = decode(h, indat, 0, &len, mbr);
      fwrite(frag, 1, len, out);
      total_image += len;
    }
    if(count == 0) break;
    if(MAKELONG(h.len) == 0) break;
    sequence += 1;

    if(first && !multi) {
      width = h.edit_length * 16;
      init_output(h);
      first = 0;
      height = MAKESHORT(h.down_total) * 16;
    }
    h.edit_length = width / 16;
    if(mbr) width = 512;

    datlen = MAKELONG(h.len);

    if(sequence % n_processors != processor) {
      total += sizeof(struct msdp_header) + datlen + 1;
      continue;
    }

    if(!multi && MAKESHORT(h.fragment) != last_frag + 1) {
      int n_pad = MAKESHORT(h.fragment) - last_frag - 1;
      char *frag = (char *) malloc(240 * 1024);
      // don't pad predictively-compressed data
      if(!(h.compression[0] & 3) && n_pad > 0) {
        errors += 1;
        status |= STAT_BADSEQ;
        bzero(frag, 240 * 1024);
        total_image += n_pad * 240 * 1024;
        if(verbose) fprintf(stderr, "padding %d frags\n", n_pad);
        while(n_pad--) fwrite(frag, 1, 240 * 1024, out);
      }
      free(frag);
    }
    last_frag = MAKESHORT(h.fragment);

    if(verbose) fprintf(stderr, "id %d/%d, len %d\n", MAKESHORT(h.id),
                          MAKESHORT(h.fragment), MAKELONG(h.len));
    chunk = (byte *) malloc(datlen + sizeof(struct msdp_header) + 1);
    indat = chunk + sizeof(struct msdp_header);
    count = fread(indat, 1, datlen, infile);
    if(count != datlen) {
      if(verbose) fprintf(stderr,
                            "Error: short read (%d) of data part of fragment\n",
                            count);
      errors += 1;
      break;
    }

    // check MSDP checksum
    if(cs_check) {
      memmove(chunk, &h, sizeof(h));
      if(fread(chunk + datlen + sizeof(h), 1, 1, infile) != 1
         && ferror(infile) && verbose) {
        fprintf(stderr, "Encountered error while tring to read MSDP "
                "checksum\n");
      }

      if(!CS8EACC2(chunk, datlen + sizeof(h) + 1)) {
        if(verbose) fprintf(stderr, "Error: bad MSDP checksum\n");
        status |= STAT_BADCS;
        errors += 1;
        if(pad_cs) {
          char *frag = (char *) malloc(240 * 1024);
          bzero(frag, 240 * 1024);
          total_image += 240 * 1024;
          total += sizeof(struct msdp_header) + datlen + 1;
          if(verbose) fprintf(stderr, "trashing bad frag\n");
          fwrite(frag, 1, 240 * 1024, out);
          free(frag);
          continue;
        }
      }
    }

    frag = decode(h, indat, datlen, &len, mbr);
    total_image += len;
    if(verbose) fprintf(stderr, "fragment len %d => %d\n", datlen, len);
    total += sizeof(struct msdp_header) + datlen + 1;
    fwrite(frag, 1, len, out);
    if(0) free(frag);
    free(chunk);
    if(h.status & 2) break;
  }
  fclose(out);

  actual_height = total_image / width;
  if(!multi && actual_height != height) {
    if(verbose) fprintf(stderr,
                          "Error: total MSDP height (%d) != actual height (%d)\n",
                          height, actual_height);
    (void) write_header(width, actual_height, infile, outfname);
    errors += 1;
    status |= STAT_SHORT;
  }

  if(status && verbose) fprintf(stderr, "error status %c%c%c%c\n",
                                  MocCompress == RAW ? 'r' : (MocCompress == PRED ? 'p' : 't'),
                                  status & STAT_BADCS ? 'c' : '-',
                                  status & STAT_BADSEQ ? 'n' : '-',
                                  status & STAT_SHORT ? 's' : '-'
                                 );
  if(errors) exit((MocCompress << 4) | status | (errors ? 1 : 0));
  else exit(0);
}

byte *decode(struct msdp_header h, byte *data, int datlen, int *len, int mbr)
{
  int height, width;
  unsigned int xcomp, pcomp, spacing = 0, levels = 0;
  uint8 *transform_decomp_main(uint8 *data, int len, int height, int width, uint32 transform, uint32 spacing, uint32 numLevels);

  byte *image = 0;
  static Array *tbuf;
  static int init_decode;
  int huffman_table = 0;

  if(mbr) {
    width = 512;
    height = 480;
    xcomp = 0;
    pcomp = 0;
  }
  else {
    width = h.edit_length * 16;
    height = MAKESHORT(h.down_length) * 16;
    xcomp = (h.compression[0] >> 2) & 3;
    pcomp = (h.compression[0] & 3);
    spacing = h.compression[4] | (h.compression[5] << 8);
    levels = (h.compression[1] >> 5) + 1;
    huffman_table = h.compression[1] & 0xf;
  }

  *len = width * height;

  if(pcomp && xcomp) {
    fprintf(stderr, "error: both pcomp and xcomp set\n");
    exit(1);
  }
  if(pcomp) MocCompress = PRED;
  if(xcomp) MocCompress = XFORM;

  if(!rawencode && pcomp == 0 && xcomp == 0) {
    // raw image
    image = data;
    if(datlen > *len) {
      if(verbose) fprintf(stderr, "Warning: MSDP line count (%d) < implied (%d), using latter\n", height, datlen / width);
      *len = datlen;
      height = datlen / width;
    }
    if(verbose) fprintf(stderr, "%d wide by %d high ", width, height);
    if(verbose) fprintf(stderr, "raw fragment%s\n", mbr ? " (MBR)" : "");
  }
  else if(verbose) fprintf(stderr, "%d wide by %d high ", width, height);

  if(xcomp > 0) {
    // transform compressed; 2 = DCT, 1 = WHT
    if(verbose) fprintf(stderr, "%s transformed fragment (%d groups, %.2f requant)\n",
                          xcomp == 2 ? "dct" : "wht", levels, spacing / 16.0);
    image = transform_decomp_main(data, datlen, height, width,
                                  xcomp - 1, spacing, levels);
  }

  if(rawencode || pcomp > 0) {
    // predictively compressed
    if(rawencode) {
      if(verbose) fprintf(stderr, "raw encoded fragment\n");
      else if(verbose) fprintf(stderr, "%s%s predictive fragment, table %d\n",
                                 pcomp & 1 ? "x" : "", (pcomp & 2) >> 1 ? "y" : "",
                                 huffman_table);
    }

    // set up decode arrays
    extern void decodeLoad(char *);
    extern void decodeInit(int);
    if(!init_decode) {
      if(*decode_file) decodeLoad(decode_file);
      else decodeInit(huffman_table);
    }

    if(test_pred) {
      int dummy;

      image = predictive_decomp_main(data, datlen, height, width,
                                     (moc_sync != 0), moc_sync,
                                     pcomp & 1, (pcomp & 2) >> 1,
                                     &dummy);
    }
    else {
      // squirrel data away
      if(!tbuf) tbuf = array_new(datlen * 8);
      if(datlen && !array_append(tbuf, (char*)data, datlen)) {
        // FIXED 2008/10/29, "datalen" was part of the print statement and no arguments
        // were provided for the %d. - Steven Lambright, pointed out by "novas0x2a" (Support Forum Member)
        fprintf(stderr, "can't allocate temp space (%d bytes)\n", datlen);
        exit(1);
      }
      image = 0;
      *len = 0;
      if(h.status & 2) {
        int got_height;
        extern int pred_past_eof;
        int want_h = MAKESHORT(h.down_total) * 16;;
        if(verbose) fprintf(stderr, "decompressing %d wide by %d high image\n",
                              width, want_h);
        image =
          predictive_decomp_main((uint8*)array_data(tbuf),
                                 array_len(tbuf),
                                 want_h, width,
                                 (moc_sync != 0), moc_sync,
                                 (pcomp & 1), ((pcomp & 2) >> 1),
                                 &got_height);
        /* This is tricky.  We can get bad moc_sync even without
           checksum errors if anomaly 8 occurs.  We want to
           distinguish between this case and the case where
           we just ran out of fragments during the NA image.
           So if we run into a sync error and haven't run off
           the end of the image, we force BADCS on. */
        if(got_height != want_h && !pred_past_eof)
          status |= STAT_BADCS;
        *len = got_height * width;
      }
    }
  }

  return image;
}

int worklist_init() {
  struct msdp_header h;
  int count;
  int frag = 0;
  int height, width, xcomp;
  int datlen;

  while(1) {
    count = fread(&h, sizeof(h), 1, infile);
    if(count == 0) break;
    xcomp = (h.compression[0] >> 2) & 3;
    if(!xcomp) return 0;
    height = MAKESHORT(h.down_length) * 16;
    width = h.edit_length * 16;
    datlen = MAKELONG(h.len);
    init_output(h);
    frag_offset[frag+1] = frag_offset[frag] + height * width;
    frag += 1;
    fseek(infile, datlen + 1, 1);
  }
  return 1;
}

void init_output(struct msdp_header h)
{
  int height, width;
  char s[8];
  int i;
  char buf[1024];

  height = MAKESHORT(h.down_total) * 16;
  width = h.edit_length * 16;
  sprintf(label, "decompressed-from %s\nid %d time %u:%d\ngain 0x%x \
offset %d\nstart %d cross %d down %d\ncmd ", infname,
          MAKESHORT(h.id),
          MAKELONG(h.time + 1), h.time[0],
          h.gain, h.offset,
          h.edit_start * 16,
          h.edit_length * 16,
          MAKESHORT(h.down_total) * 16);
  switch(h.cmd[0]) {
    case 1:
    case 2:
      strcat(label, "na ");
      break;
    case 3:
    case 4:
      strcat(label, "wa ");
      break;
    case 5:
    case 6:
      strcat(label, "global-map ");
      break;
    case 0x15:
    case 0x16:
      strcat(label, "mbr ");
      mbr = 1;
      width = 512;
      height = 512;
      break;
    case 0xd:
      strcat(label, "read-memory ");
      break;
    default:
      strcat(label, "unknown ");
      break;
  }
  for(i = 0; i < 17; i++) {
    sprintf(s, "%02x", h.cmd[i]);
    strcat(label, s);
  }

  sprintf(buf, "\nsensor %d clocking %d system-id 0x%x",
          MAKESHORT(h.sensors),
          MAKESHORT(h.other + 1),
          h.other[3]);
  strcat(label, buf);

  out = (FILE *) write_header(width, height, infile, outfname);
}
