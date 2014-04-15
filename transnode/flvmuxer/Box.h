#ifndef __BOX_H
#define __BOX_H

#include <fstream>
#include <vector>
#include <string>
using namespace std;

class Box
{
public:
	string type;
	unsigned long long size;
	unsigned long long pos;
	vector<Box> children;

public:
	Box(void);
	Box(string ptype,unsigned long long psize,unsigned long long ppos);
	~Box(void);

public:
	void FindAllChild(FILE *pf);
	Box FindBox(FILE *pf, string boxtype,int num = 1);
	Box GetChild(string boxtype);
	unsigned int BRead32(FILE *pf,long long offset);
	unsigned int BRead16(FILE *pf,long long offset);
	unsigned int BRead8(FILE *pf,long long offset);
	string BReadBytes(FILE *pf,long long offset,long long size);
	void operator = (const Box& rh);
};

#endif