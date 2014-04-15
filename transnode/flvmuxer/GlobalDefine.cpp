#include "GlobalDefine.h"

char GlobalDefinebuf[10 * 1024 * 1024] = "";

#define BUFFERSIZE 10240

unsigned int Read8(FILE *pf)
{
	unsigned char temp;
	fread(&temp, sizeof(unsigned char), 1,pf);
	return temp;
}

unsigned int Read16(FILE *pf)
{
	unsigned char temp[3] = "";
	unsigned int num = 0;
	fread(temp, 2, 1, pf);
	for(int i = 0; i < 2; i++)
	{
		num |= temp[i] << ((1 - i) * 8);
	}
	return num;
}

unsigned int Read24(FILE *pf)
{
	unsigned char temp[4] = "";
	unsigned int num = 0;
	fread(temp, 3, 1, pf);
	for(int i = 0; i < 3; i++)
	{
		num |= temp[i]<<(2 - i) * 8;
	}
	return num;
}

unsigned int Read32(FILE *pf)
{
	unsigned char temp[5] = "";
	unsigned int num = 0;
	fread(temp, 4, 1, pf);
	for(int i = 0; i < 4; i++)
	{
		num |= temp[i] << ((3 - i) *8);
	}
	return num;
}

unsigned long long Read64(FILE *pf)
{
	unsigned char temp[9] = "";
	unsigned long long num = 0;
	fread(temp, 8, 1, pf);
	for(int i = 0; i < 8; i++)
	{
		num = num << 8;
		num |= temp[i];
	}
	return num;
}

unsigned int ReadL32(FILE *pf)
{
	unsigned int temp = Read32(pf);
	unsigned int num = 0;
	num |= (temp >> 24) & 0x000000ff;
	num |= (temp >> 8) & 0x0000ff00;
	num |= (temp << 8) & 0x00ff0000;
	num |= (temp << 24) & 0xff000000;
	return num;
}

string ReadLine(FILE *pf)
{
	string str = "";
	char buf;
	while((buf = fgetc(pf)) != '\n')
	{
		if(buf = -1)
		{
			break;
		}
		str += buf;
	}
	return str;
}

string ReadBytes(FILE *pf, const unsigned int length )
{
	int left = length;
	int current = 0;
	string str = "";
	while(left)
	{
		if (left > 1024 * 1024)
		{
			current = 1024 * 1024;
		}
		else
		{
			current = left;
		}
		memset(GlobalDefinebuf, 0, current);
		//	rewind(pf);
		fread(GlobalDefinebuf, current, 1, pf);
		for (int i = 0; i < current; i ++)
		{
			str += GlobalDefinebuf[i];
		}
		left -= current;
	}
	return str;
}

void Write8( FILE *pf, unsigned char temp)
{
	fwrite(&temp, sizeof(unsigned char), 1, pf);
}

void Write16( FILE *pf, unsigned int temp )
{
	char buf[2] = "";
	for (int i = 0; i < 2; i ++)
	{
		buf[i] = temp >> ((1 - i) * 8);
	}
	fwrite(buf, 2, 1, pf);
}

void Write24( FILE *pf,unsigned int temp )
{
	char buf[3] = "";
	for (int i = 0; i < 3; i ++)
	{
		buf[i] = temp >> ((2 - i) * 8);
	}
	fwrite(buf, 3, 1, pf);
}

void Write32( FILE *pf,unsigned int temp )
{
	char buf[4] = "";
	for (int i = 0; i < 4; i ++)
	{
		buf[i] = temp >> ((3 - i) * 8);
	}
	fwrite(buf, 4, 1, pf);
}

void Write64( FILE *pf,unsigned long long temp )
{
	char buf[8] = "";
	for (int i = 0; i < 8; i ++)
	{
		buf[i] = temp >> ((7 - i) * 8);
	}
	fwrite(buf, 8, 1, pf);
}

void WriteL32( FILE *pf,unsigned int temp )
{
	Write8(pf,temp);
	Write8(pf,temp >> 8);
	Write8(pf,temp >> 16);
	Write8(pf,temp >> 24);
}

void WriteBytes( FILE *pf,string temp,const unsigned int length )
{
	memset(GlobalDefinebuf, 0, length + 1);
	memcpy(GlobalDefinebuf,temp.c_str(),temp.length());
	fwrite(GlobalDefinebuf, length, 1, pf);
}

unsigned int ParseInt( string str )
{
	unsigned int temp = 0;
	for (int i = 0; i < str.length(); i ++)
	{
		temp *= 10;
		temp += str[i] - '0';
	}
	return temp;
}

unsigned int ParseClockInt( string clock )
{
	int pos = clock.find_first_of(':');
	string h = clock.substr(0,pos);
	clock = clock.substr(pos + 1, -1);
	pos = clock.find_first_of(':');
	string m = clock.substr(0,pos);
	clock = clock.substr(pos + 1, -1);
	pos = clock.find_first_of('.');
	string s = clock.substr(0,pos);
	clock = clock.substr(pos + 1, -1);
	string ms = clock;
	unsigned int temp = ParseInt(h) * 60 * 60 * 1000 + ParseInt(m) * 60 * 1000 + ParseInt(s) * 1000 + ParseInt(ms);
	return temp;
}

void WriteFLVHead(FILE *pf,bool hasaudio,bool hasvideo)
{
	WriteBytes(pf,"FLV",3);
	Write8(pf,1);
	unsigned char hasaudiovideo = 0;
	if (hasaudio)
	{
		hasaudiovideo += 4;
	}
	if (hasvideo)
	{
		hasaudiovideo += 1;
	}
	Write8(pf,hasaudiovideo);
	Write32(pf,9);
	Write32(pf,0);
}

void WriteMetaData(FILE *pf)
{
	WriteFLVTag(pf,18,0,false,0);
}

void WriteFLVTag( FILE *pf,unsigned char type,unsigned int datasize,bool keyframe,unsigned int timestamp,unsigned int dts,string data /*= ""*/,int length /*= 0*/ )
{
	if (type == 9)
	{
		WriteTagHead(pf,type,datasize + 5,timestamp);
		if (keyframe)
		{
			Write8(pf,0x17);
		}
		else
		{
			Write8(pf,0x27);
		}
		Write8(pf,1);
		Write24(pf,dts);
		WriteTag(pf,data,length);
		Write32(pf,datasize + 16);
	}
	else if(type == 8)
	{
		WriteTagHead(pf,type,datasize + 2,timestamp);
		Write8(pf,0xaf);
		Write8(pf,1);
		WriteTag(pf,data,length);
		Write32(pf,datasize + 13);
	}
	else
	{
		WriteTagHead(pf,type,datasize,timestamp);
		WriteTag(pf,data,length);
		Write32(pf,datasize + 11);
	}
}

void WriteTagHead( FILE *outfile,unsigned char type,unsigned int datasize,unsigned int timestamp )
{
	Write8(outfile,type);
	Write24(outfile,datasize);
	Write24(outfile,timestamp);
	Write8(outfile,timestamp >> 24);
	Write24(outfile,0);
}

void WriteTag( FILE *pf,string data,int length )
{
	if (length == 0)
	{
		return;
	}
	WriteBytes(pf,data,length);
}

bool IsBigEndian()
{
	int x = 1;
	int* p = &x;
	return !((char*)p)[0];
}

std::string CharToString( const char* str,int len )
{
	string temp = "";
	for (int i = 0; i < len; i ++)
	{
		temp += str[i];
	}
	return temp;
}

std::string IntToString( unsigned int value, int maxnum )
{
	string temp = "";
	for (int i = 0; i < maxnum; i ++)
	{
		temp += (value >> (maxnum - i - 1)) & 0xff;
	}
	return temp;
}

bool IsWordorNumber( char value )
{
	if (value >= '0' && value <= '9')
	{
		return true;
	}
	if (value >= 'a' && value <= 'z')
	{
		return true;
	}
	if (value >= 'A' && value <= 'Z')
	{
		return true;
	}
	return false;
}
