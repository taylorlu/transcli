#include "MetaUnit.h"
#include "GlobalDefine.h"

MetaUnit::MetaUnit(void)
{
	title = "";
	type = 0;
	data = "";
}

MetaUnit::~MetaUnit(void)
{
}

void MetaUnit::GetDoubleData( const char* strtitle,double doubledata )
{
	type = 0;
	title = string(strtitle);
	data = DoubletoString(doubledata);
}

void MetaUnit::GetStringData( const char* strtitle,const char* stringdada )
{
	type = 2;
	title = string(strtitle);
	data = string(stringdada);
	if (data.length() > (unsigned short)(0xffff))
	{
		GetLongStringData(strtitle,stringdada);
	}
	else
	{
		data = LentoString(data.length()) + data;
	}
}

void MetaUnit::GetLongStringData( const char* strtitle,const char* longstringdata )
{
	type = 12;
	title = string(strtitle);
	data = string(longstringdata);
	data = LentoString(data.length(),4) + data;
}

void MetaUnit::GetArrayData( const char* strtitle,vector<char*> subtitles,vector<vector<double> > arraydata )
{
	type = 3;
	title = string(strtitle);
	data = "";
	for(int i = 0; i < subtitles.size(); i ++)
	{
		string subtitle = string(subtitles[i]);
		data += LentoString(subtitle.length());
		data += subtitle;
		data += char(10);
		if (!arraydata.size())
		{
			continue;
		}
		data += LentoString(arraydata[i].size(),4);
		for (int j = 0; j < arraydata[i].size(); j ++)
		{
			data += char(0);
			data += DoubletoString(arraydata[i].at(j));
		}
	}
	data += char(0);
	data += char(0);
	data += char(9);
}

//void MetaUnit::WriteMetaUnit( ofstream &outfile )
//{
//	Write16(outfile,title.length());
//	WriteBytes(outfile,title,title.length());
//	Write8(outfile,type);
//	WriteBytes(outfile,data,data.length());
//}
void MetaUnit::WriteMetaUnit( FILE *pf )
{
	Write16(pf,title.length());
	WriteBytes(pf,title,title.length());
	Write8(pf,type);
	WriteBytes(pf,data,data.length());
}

std::string MetaUnit::DoubletoString( double temp )
{
	string tempdata;
	if (IsBigEndian())
	{
		for(int i = 0; i < 8; i ++)
		{
			tempdata += ((char*)(&temp))[i];
		}
	}
	else
	{
		for(int i = 0; i < 8; i ++)
		{
			tempdata += ((char*)(&temp))[7 - i];
		}
	}
	return tempdata;
}

std::string MetaUnit::LentoString( unsigned int len,unsigned int maxnum )
{
	string templen = "";
	while(len)
	{
		templen = (char)(len & 0xff) + templen;
		len >>= 8;
	}
	while(templen.length() < maxnum)
	{
		templen = char(0) + templen;
	}
	return templen;
}
