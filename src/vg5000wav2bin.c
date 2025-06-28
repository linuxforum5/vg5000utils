/***************************************************
 * vg5000wav2bin, 2025.04. Princz László
 * Based on audiodump from http://micken.se/esselte.html
 *
 * Convert Esselte 100 wav file into BASIC text source
 * Currently only 44100khz, 8 bit mono PCM
 ****************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "getopt.h"
#include "math.h"
#include "lib/WavReader.h"

#define VM 0
#define VS 1
#define VB 'b'

#define WAV_SAMPLE_RATE 44100
#define PI 3.1415926
#define D 3
#define SILD 30
#define SAMPLETYPE signed char

bool verbose = true;
bool binmode = true;


signed char get_sign( signed char sample ) { return ( sample > SILD ) ? 1 : ( sample < - SILD ) ? -1 : 0; }
// signed char get_sign( SAMPLETYPE sample ) { return ( sample > 127 + SILD ) ? 1 : ( sample < 127 - SILD ) ? -1 : 0; }

/*
bool sample_sign_changed( unsigned char sample, unsigned char last_sample ) {
    if ( abs( sample - last_sample ) > 5 ) {
        if ( sample <= 127 && last_sample > 127 ) {
            return true;
        } else if ( sample > 127 && last_sample <= 127 ) {
            return true;
        }
    }
    return false;
}
*/
unsigned char read_bit( WavReader* fin ) {
    SAMPLETYPE sample;
    int skipCnt = 0;
    for( sample = fin->readNextSample8( fin, 0 ); get_sign( sample ) == 2; sample = fin->readNextSample8( fin, 0 ) ) skipCnt++;
//fprintf( stdout, "Sample=%d\n", sample );
    if ( skipCnt ) {
        fprintf( stdout, "SKip %d samples\n", skipCnt );
    }
    int cnt1 = 0;
    unsigned char sampleSign1 = get_sign( sample ); // 0/1
    for( sample = fin->readNextSample8( fin, 0 ); get_sign( sample ) == sampleSign1; sample = fin->readNextSample8( fin, 0 ) ) cnt1++;
//fprintf( stdout, "Sample=%d\n", sample );
    unsigned char sampleSign2 = get_sign( sample ); // 0/1
    if ( sampleSign1 + sampleSign2 != 1 ) {
        fprintf( stderr, "Szinkronhiba 1!\n" );
        exit(1);
    }
    int cnt2 = 0;
    for( sample = fin->readNextSample8( fin, 0 ); get_sign( sample ) == sampleSign2; sample = fin->readNextSample8( fin, 0 ) ) cnt2++;
//fprintf( stdout, "Sample=%d\n", sample );
    if ( cnt1 != cnt2 ) {
        fprintf( stderr, "A két hullámhossz eltér: %d(%d) != %d(%d) !\n", cnt1, sampleSign1, cnt2, sampleSign2 );
        exit(1);
    }
    // if ( verbose ) fprintf( stdout, "Found bit %d in %d sample\n", bit, sample_counter );
    fseek( fin->file_descriptor, -2, SEEK_CUR );
    printf( "+\n" );
    switch ( cnt1 ) {
        case 4 : return 0; break;
        case 5 : return 0; break;
        case 10 : return 1; break;
        default: fprintf( stderr, "Invalid half wav length: %d\n", cnt1 ); exit(1);
    }
}

/**
 * Format of a byte: 
 * - leading bit 0
 * - 8 data bit
 * - ending bit 1
 */
unsigned char read_byte( WavReader* fin ) {
    unsigned char byte = 0;
    unsigned char bit;
    // while( read_bit( fin, 04 ) ); // search leading zero bit
    for ( int j = 0; j < 8; j++ ) {
        byte += read_bit( fin ) * (1<<j);
    }
    //if ( !read_bit( fin, 9 ) ) {
    //    fprintf( stderr, "Byte (0x%02X) stop bit error at pos: %d\n", byte, fin->ftell( fin ) );
    //    exit(1);
    //}
    //if ( verbose ) fprintf( stdout, "Found byte %d (%c)\n", byte, byte );
    return byte;
}

typedef struct SampleBlock { // "SampleBlock" osztály
    // Private area
    signed char sgn; // -1,0,1
    unsigned long len; // counter
} SampleBlock;

// signed char last_sample;

SampleBlock readBlock( WavReader *fin ) {
    SampleBlock bb = { 0, 0 };
    unsigned long pos = ftell( fin->file_descriptor );
    signed char sample = fin->readNextSample8( fin, 0 );
    bb.sgn = get_sign( sample );
    while( !fin->eof( fin ) && ( get_sign( sample ) == bb.sgn ) ) {
        bb.len++;
        sample = fin->readNextSample8( fin, 0 );
    }
    if ( !get_sign( sample ) ) { // Ha 0-ával volt vége
        if ( bb.len < 5 ) {
            bb.len++;
            sample = fin->readNextSample8( fin, 0 );
            while( !fin->eof( fin ) && ( get_sign( sample ) == bb.sgn ) ) {
                bb.len++;
                sample = fin->readNextSample8( fin, 0 );
            }
        }
    } else if ( !bb.sgn ) { // Ez egy 0 értékű blokk
        if ( bb.len == 1 ) {
            bb = readBlock( fin );
            bb.len++;
        }
    }
    if ( !fin->eof( fin ) ) fseek( fin->file_descriptor, pos + bb.len*2, SEEK_SET );
    return bb;
}

unsigned char readBit( WavReader *fin, int *i ) {
    SampleBlock bb = readBlock( fin );
    if ( bb.sgn ) {
        SampleBlock bb2 = readBlock( fin );
        if ( bb.len != bb2.len ) {
            fprintf( stderr, "Hullámhossz hiba: %d (sgn=%d) + %d (sgn=%d)\n", bb.len, bb.sgn, bb2.len, bb2.sgn );
            exit(1);
        }
        if ( bb.sgn + bb2.sgn ) {
            fprintf( stderr, "Félhullám előjel hiba: %d (sgn=%d)\n", bb.len, bb.sgn );
            exit(1);
        }
        switch ( bb.len ) {
            case 5 : return 0; break; // 0-ás bit
            case 10: return 1; break;
            default:
                fprintf( stderr, "Bithossz hiba: %d (sgn=%d)\n", bb.len, bb.sgn );
                exit(1);
        }
    } else { // Silence
        if ( i ) {
            fprintf( stderr, "Szünethiba: %d. bit. Mintahossz: %d.\n", i, bb.len );exit(1);
        } else {
printf( "****\n" );
            i--;
        }
    }
    return 0;
}

unsigned char readBit2( WavReader *fin, int *i ) {
    unsigned char b = readBit( fin, i );
    if ( !b ) {
        b = readBit( fin, i );
        if ( b ) {
            fprintf( stderr, "Zérő bit error\n" );exit(1);
        }
    }
    return 1-b;
}

unsigned char readByte2( WavReader *fin ) {
    unsigned char b = 0;
    int dummi = 0;
    if ( readBit2( fin, &dummi ) ) {
        fprintf( stderr, "Prefix bit not zero!\n" );exit(1);
    }
    for( int i=0; i<8; i++ ) {
        // b += readBit2( fin, &i ) * (128>>i);
        b += readBit2( fin, &i ) * (1<<i);
    }
    if ( !readBit2( fin, &dummi ) ) {
        fprintf( stderr, "First postfix bit not 1!\n" );exit(1);
    }
    if ( !readBit2( fin, &dummi ) ) {
        fprintf( stderr, "Second postfix bit not 1!\n" );exit(1);
    }
    return b;
}

void skipLeader( WavReader *fin ) {
    unsigned long pos = ftell( fin->file_descriptor );
    unsigned char bit = readBit( fin, 0 );
    int skipCnt = 0;
    while( !bit ) {
        pos = ftell( fin->file_descriptor );
        bit = readBit( fin, 0 );
        skipCnt++;
        // printf( "Bit: %d\n", bit );
    }
    printf( "Skip %d 0\n", skipCnt );
    fseek( fin->file_descriptor, pos, SEEK_SET );
}

// Kansas City standard
// 4 x 1200 hz = 0
// 8 x 2400 hz = 1
void audioread( WavReader *fin, FILE *fout ) {
    skipLeader( fin );
    unsigned char bit;

    for( int i=0; i<32; i++ ) {
        unsigned char b = readByte2( fin );
        printf( "%02X ", b );
    }
    printf( "\n" );

/*
    for( int i=0; i<352; i++ ) {
        bit = readBit2( fin, 0 );
        fprintf( stdout, "%d ", bit );
        if ( i%11 == 10 ) fprintf( stdout, "\n" );
    }
*/
    skipLeader( fin );


    for( int i=0; i<32; i++ ) {
        unsigned char b = readByte2( fin );
        printf( "%02X ", b );
    }
    printf( "\n" );
/*
    for( int i=0; i<352; i++ ) {
        bit = readBit2( fin, 0 );
        fprintf( stdout, "%d ", bit );
        if ( i%11 == 10 ) fprintf( stdout, "\n" );
    }
*/
    exit(1);
    // last_sample = fin->readNextSample8( fin, 0 );
    for( int i=0; i<100; i++ ) {
        unsigned char b = readByte2( fin );
        fprintf( stdout, "%d. byte: 0x%0X\n", i, b );
    }
    exit(1);
/*
    SampleBlock bb0,bb2;
    last_sample = fin->readNextSample8( fin, 0 );
    for ( SampleBlock bb = readBlock( fin ); !fin->eof( fin ); bb = readBlock( fin ) ) {
        if ( bb.sgn ) {
            bb2 = readBlock( fin );
            if ( bb.len != bb2.len ) {
                fprintf( stderr, "Wave Error 1\n" );
                exit(1);
            }
        }
*/
    for ( unsigned char b = readByte2( fin ); !fin->eof( fin ); b = readByte2( fin ) ) {
        fputc( b, fout );
//        printf( "%d: %d\n", bb.sgn, bb.len );
//        bb0 = bb;
/*        unsigned char byte = read_byte( fin );
        if ( binmode ) {
            fputc( byte, fout );
        } else if ( byte == 13 ) {
            fputc( 10, fout );
        } else if ( byte >= 32 ) {
            fputc( byte, fout );
        }*/
    }
}

void print_usage() {
    printf( "vg5000wav2bin v%d.%d%c (build: %s)\n", VM, VS, VB, __DATE__ );
    printf( "Create bin file from Philips VG5000 loadable wav file.\n");
    printf( "Copyright 2025 by László Princz\n");
    printf( "Usage:\n");
    printf( "vg5000wav2bin <input_filename> [<output_filename_without_wav_extension>]\n" );
    printf( "Convert input file to %d sampled 8 bit mono wav format.\n", WAV_SAMPLE_RATE );
    printf( "Command line option:\n");
    printf( "-v          : set verbose mode\n" );
    // printf( "-b          : save binary output instead of txt file\n" );
    printf( "-h          : prints this text\n");
    exit(1);
}

WavReader* fopen_wav_rd( const char* inname ) {
    WavReader *f = WavReader_open( inname );
    f->checkSampleRate( f, 44100 );
    f->showInfo( f, stdout );
    return f;
}

int main(int argc, char *argv[]) {
    int finished = 0;
    int argd = argc;

    while ( !finished ) {
        switch ( getopt ( argc, argv, "?hvtbn:e:" ) ) {
            case -1:
            case ':':
                finished = 1;
                break;
            case '?':
            case 'h':
                print_usage();
                break;
            case 'v':
                verbose = true;
                break;
            case 'b':
                binmode = true;
                break;
            default:
                break;
        }
    }

    if ( argc - optind > 0 ) {
        char inname[ 80 ]; // Input filename
        char outname[ 80 ]; // Output filename
        WavReader *fin;
        FILE *fout;
        strcpy( inname, argv[ optind ] );
        strcpy( outname, inname ); // The default output name is input filename + .txt
        if ( argc - optind > 1 ) strcpy( outname, argv[ optind + 1 ] );
        if ( argc - optind > 2 ) print_usage();
        strcat( outname, binmode ? ".bin" : ".bas" );
        if ( fin = fopen_wav_rd( inname ) ) {
            if ( fout = fopen( outname, "wb" ) ) {
                fprintf( stdout, "Start conversion from '%s' to '%s'\n", inname, outname );
                audioread( fin, fout );
                fclose( fout );
                WavReader_close( fin );
            } else {
                fprintf( stderr, "Error creating %s.\n", outname );
                exit(4);
            }
        } else {
            fprintf( stderr, "Error opening %s.\n", inname );
            exit(4);
        }
    } else {
        print_usage();
    }
}
