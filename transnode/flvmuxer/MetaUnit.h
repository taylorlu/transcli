#ifndef _META_UNIT_H_
#define _META_UNIT_H_

#include <string>
#include <vector>
#include <fstream>
using namespace std;

class MetaUnit
{
public:
	MetaUnit(void);
	~MetaUnit(void);

private:
	string	title;
	char	type;
	string	data;

public:
	void GetDoubleData(const char* strtitle,double doubledata);
	void GetStringData(const char* strtitle,const char* stringdada);
	void GetLongStringData(const char* strtitle,const char* longstringdata);
	void GetArrayData(const char* strtitle,vector<char*> subtitles,vector<vector<double> > arraydata);

	/*void WriteMetaUnit(ofstream &outfile);*/
	void WriteMetaUnit(FILE *pf);
private:
	string DoubletoString(double temp);
	string LentoString(unsigned int len,unsigned int maxnum = 2);
};

#endif