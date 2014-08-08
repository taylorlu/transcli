#ifndef __GLOBAL_DEFINE_H
#define __GLOBAL_DEFINE_H

#include <fstream>
#include <stdio.h>
using namespace std;

#ifdef WIN32
#define fseek _fseeki64
#define ftell _ftelli64
#else
#define _FILE_OFFSET_BITS 64
#define __USE_FILE_OFFSET64
#define __USE_LARGEFILE64
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#define fseek fseeko64
#define fell ftello64
#endif

//unsigned int ReadBit(FILE *pf);
unsigned int Read8(FILE *pf);

unsigned int Read16(FILE *pf);

unsigned int Read24(FILE *pf);

unsigned int Read32(FILE *pf);

unsigned long long Read64(FILE *pf);

unsigned int ReadL32(FILE *pf);

string ReadLine(FILE *pf);

string ReadBytes(FILE *pf, const unsigned int length);

void Skip(FILE *pf, unsigned long long bytesOffset);

void Write8(FILE *pf,unsigned char temp);

void Write16(FILE *pf,unsigned int temp);

void Write24(FILE *pf,unsigned int temp);

void Write32(FILE *pf,unsigned int temp);

void Write64(FILE *pf,unsigned long long temp);

void WriteL32(FILE *pf,unsigned int temp);

void WriteBytes(FILE *pf,string temp,const unsigned int length);

unsigned int ParseInt(string str);
unsigned int ParseClockInt(string clock);

void WriteFLVHead(FILE *pf,bool hasaudio = true,bool hasvideo = true);

void WriteMetaData(FILE *pf);

void WriteFLVTag(FILE  *pf,unsigned char type,unsigned int datasize,bool keyframe,unsigned int timestamp,unsigned int dts = 0,string data = "",int length = 0);

void WriteTagHead(FILE *pf,unsigned char type,unsigned int datasize,unsigned int timestamp);

void WriteTag(FILE *pf,string data,int length);

bool IsBigEndian();
bool IsWordorNumber(char value);
string CharToString(const char* str,int len);
string IntToString(unsigned int value,int maxnum = 4);

#endif