/***************************************************
 * vg5000k72wav, 2025.04. Princz László
 * Convert k7 files to VG5000 wav file
 * VG5000 Wav format:
 * bit 1 : 5+5-5+5-
 * bit 0 : 10+10-
 * Byte : 0 b0 b1 b2 b3 b4 b5 b6 b7 1 1
 * Leading : (5+5-)+
 ****************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "getopt.h"
#include "math.h"

#define VM 0
#define VS 1
#define VB 'b'

#define WAV_SAMPLE_RATE 44100
#define HIGH 255
#define LOW 0
#define SILENCE 128

bool verbose = false;

void write_samples( FILE *fout, unsigned char value, int counter ) {
    for( int i=0; i<counter; i++ ) {
        fputc( value, fout );
    }
}

void write_silence( FILE *fout, int counter ) {
    for( int i=0; i<counter; i++ ) {
        fputc( SILENCE, fout );
    }
}

void write_bit( FILE* fout, int bit ) {
    if ( bit ) {
        write_samples( fout, HIGH, 5 );
        write_samples( fout, LOW, 5 );
        write_samples( fout, HIGH, 5 );
        write_samples( fout, LOW, 5 );
    } else {
        write_samples( fout, HIGH, 10 );
        write_samples( fout, LOW, 10 );
    }
}

void write_byte( FILE *fout, unsigned char b ) {
    write_bit( fout, 0 );
    for( int i=0; i<8; i++ ) {
        write_bit( fout, b & 1<<i );
    }
    write_bit( fout, 1 );
    write_bit( fout, 1 );
}

int copy_header( FILE *fin, FILE *fout ) {
    unsigned char header[32];
    for( int i=0; i<32; i++ ) {
        write_byte( fout, header[i] = fgetc( fin ) );
    }
    // printf( "Data length: 0x%02X 0x%02X\n", header[ 28 ], header[ 29 ] );
    return header[ 28 ] + 256 * header[ 29 ] + 20; // data + 10 bytes block type + 10 bytes 
}

void copy_data( FILE *fin, FILE *fout, int data_size ) { // 10 * 0xD6 + payload + 10 * 0x00
    for( int i=0; i<data_size; i++ ) {
        write_byte( fout, fgetc( fin ) );
    }
}

void write_leading( FILE *fout, int counter ) {
    for( int i=0; i<counter; i++ ) {
        write_bit( fout, 1 );
    }
}

void convert( FILE *fin, FILE *fout ) { // return sample counter
    fseek( fin, 0L, SEEK_END );
    unsigned long data_size_0 = ftell( fin ) - 32;
    fseek( fin, 0L, SEEK_SET );
    write_silence( fout, 1024 );
    write_leading( fout, 1024 );
    int data_size = copy_header( fin, fout );
    if ( data_size != data_size_0 ) {
        fprintf( stderr, "Fájlmérethiba!\n" );exit(1);
    }
    fprintf( stdout, "Data: %d , %d\n", data_size, data_size_0 );
    write_silence( fout, 1024 );
    write_leading( fout, 1024 );
    copy_data( fin, fout, data_size );
}

/**
 * Wav header size
 */
void wavheaderout( FILE *f, int numsamp ) {
    fprintf( f, "%s", "RIFF" );         //     Chunk ID - konstans, 4 byte hosszú, értéke 0x52494646, ASCII kódban "RIFF"
    int totlen = 44 + numsamp;
    fputc( (totlen & 0xff), f );  //     Chunk Size - 4 byte hosszú, a fájlméretet tartalmazza bájtokban a fejléccel együtt, értéke 0x01D61A72 (decimálisan 30808690, vag
    fputc( (totlen >> 8) & 0xff, f );
    fputc( (totlen >> 16) & 0xff, f );
    fputc( (totlen >> 24) & 0xff, f );
    int srate = WAV_SAMPLE_RATE;
    fprintf( f, "%s", "WAVE" );         //     Format - konstans, 4 byte hosszú,értéke 0x57415645, ASCII kódban "WAVE"
    fprintf( f, "%s", "fmt " );         //     SubChunk1 ID - konstans, 4 byte hosszú, értéke 0x666D7420, ASCII kódban "fmt "
    fprintf( f, "%c%c%c%c", 0x10, 0, 0, 0 );  //     SubChunk1 Size - 4 byte hosszú, a fejléc méretét tartalmazza, esetünkben 0x00000010
    fprintf( f, "%c%c", 0x1, 0x0 );     //     Audio Format - 2 byte hosszú, PCM esetében 0x0001
    fprintf( f, "%c%c", 0x1, 0x0 );     //     Num Channels - 2 byte hosszú, csatornák számát tartalmazza, esetünkben 0x0002
    fprintf( f, "%c%c%c%c",srate&255, srate>>8, 0, 0 ); //     Sample Rate - 4 byte hosszú, mintavételezési frekvenciát tartalmazza, esetünkben 0x00007D00 (decimálisan 32000)
    fprintf( f, "%c%c%c%c",srate&255, srate>>8, 0, 0 ); //     Byte Rate - 4 byte hosszú, értéke 0x0000FA00 (decmálisan 64000)
    fprintf( f, "%c%c", 0x01, 0 ); //	Bytes Per Sample: 1=8 bit Mono, 2=8 bit Stereo or 16 bit Mono, 4=16 bit Stereo
    fprintf( f, "%c%c", 0x08, 0 ); //	//     Bits Per Sample - 2 byte hosszú, felbontást tartalmazza bitekben, értéke 0x0008
    fprintf( f, "%s", "data");     //     Sub Chunk2 ID - konstans, 4 byte hosszú, értéke 0x64617461, ASCII kódban "data"
    fprintf( f, "%c%c%c%c", numsamp &0xff, (numsamp >>8)&0xff, (numsamp >>16)&0xff, (numsamp >>24)&0xff ); //	Length Of Data To Follow : Sub Chunk2 Size - 4 byte hosszú, az adatblokk méretét tartalmazza bájtokban, értéke 0x01D61A1E
}

void print_usage() {
    printf( "vg5000k72wav v%d.%d%c (build: %s)\n", VM, VS, VB, __DATE__ );
    printf( "K7 to VG5000 wav file converter.\n");
    printf( "Copyright 2025 by László Princz\n");
    printf( "Usage:\n");
    printf( "vg5000k72wav <input_filename> [<output_filename_without_wav_extension>]\n" );
    printf( "Convert k7 file to %d sampled 8 bit mono wav format.\n", WAV_SAMPLE_RATE );
    printf( "Command line option:\n");
    printf( "-v          : set verbose mode\n" );
    printf( "-h          : prints this text\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    bool finished = false;
    int argd = argc;

    while ( !finished ) {
        switch ( getopt ( argc, argv, "?hvc:" ) ) {
            case -1:
            case ':':
                finished = true;
                break;
            case '?':
            case 'h':
                print_usage();
                break;
/*            case 'c' :
                if ( !sscanf( optarg, "%i", &afterLineZeroCounter ) ) {
                    fprintf( stderr, "Error parsing argument for '-c'.\n");
                    exit(2);
                }
                break;*/
            case 'v':
                verbose = true;
                break;
            default:
                break;
        }
    }

    if ( argc - optind > 0 ) {
        char inname[ 80 ]; // Input filename
        char outname[ 80 ]; // Output filename
        FILE *fin,*fout;
        strcpy( inname, argv[ optind ] );
        strcpy( outname, inname ); // The default output name is input filename + .wav
        if ( argc - optind > 1 ) strcpy( outname, argv[ optind + 1 ] );
        if ( argc - optind > 2 ) print_usage();
        strcat( outname, ".wav" );
        if ( fin = fopen( inname, "rb" ) ) {
            if ( fout = fopen( outname, "wb" ) ) {
                fprintf( stdout, "Start conversion from '%s' to '%s'\n", inname, outname );
                wavheaderout( fout, 0 );
                convert( fin, fout );
                unsigned long sample_counter = ftell( fout ) - 44;
                fseek( fout, 0, SEEK_SET );
                wavheaderout( fout, sample_counter );
                fclose( fout );
                fclose( fin );
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
