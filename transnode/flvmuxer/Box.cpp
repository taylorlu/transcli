#include "Box.h"
#include "GlobalDefine.h"

Box::Box(void)
{

}

Box::Box( string ptype,unsigned long long psize,unsigned long long ppos )
{
	type = ptype;
	size = psize;
	pos = ppos;

}

Box::~Box(void)
{

}

void Box::FindAllChild( FILE *pf )
{
	fseek(pf, pos, SEEK_SET);
	while ((long long)ftell(pf) - pos + 8 <= size)
	{
		unsigned int tsize = Read32(pf);
		string type = ReadBytes(pf,4);
		bool isbox = true;
		if (!tsize)
		{
			break;
		}
		for (int i = 0; i < type.length(); i ++)
		{
			if (!IsWordorNumber(type[i]))
			{
				isbox = false;
				break;
			}
		}
		if (!isbox)
		{
			break;
		}
		Box child(type,tsize,ftell(pf));
		children.push_back(child);
		fseek(pf, tsize - 8, SEEK_CUR);
	}
	for (int i = 0; i < children.size(); i ++)
	{
		children[i].FindAllChild(pf);
	}
}

Box Box::FindBox( FILE *pf, string boxtype ,int num)
{
	fseek(pf, pos, SEEK_SET);
	int count = 0;
	while ((long long)ftell(pf) - pos < size)
	{
		long long offset = ftell(pf);
		unsigned long long tsize = Read32(pf);
		string ttype = ReadBytes(pf,4);
		if (!tsize)
		{
			break;
		}
		if (ttype == boxtype)
		{
			count ++;
			if (count == num)
			{
				Box temp(ttype,tsize,ftell(pf));
				return temp;
			}
		}
		fseek(pf, tsize - 8, SEEK_CUR);
	}
	Box temp("",0,0);
	return temp;
}

Box Box::GetChild( string boxtype )
{
	for (int i = 0; i < children.size(); i ++)
	{
		if (children[i].type == boxtype)
		{
			Box temp = children[i];
			return temp;
		}
		Box childbox = children[i].GetChild(boxtype);
		if (childbox.type == boxtype)
		{
			return childbox;
		}
	}
	return Box("",0,0);
}

void Box::operator=( const Box& rh )
{
	type = rh.type;
	size = rh.size;
	pos = rh.pos;

}

unsigned int Box::BRead32( FILE *pf,long long offset )
{
	fseek(pf, pos + offset, SEEK_SET);
	return Read32(pf);
}

unsigned int Box::BRead16( FILE *pf,long long offset )
{
	fseek(pf, pos + offset, SEEK_SET);
	return Read16(pf);
}

unsigned int Box::BRead8(FILE *pf,long long offset )
{
	fseek(pf, pos + offset, SEEK_SET);
	return Read8(pf);
}

std::string Box::BReadBytes( FILE *pf,long long offset,long long size )
{
	fseek(pf, pos + offset, SEEK_SET);
	return ReadBytes(pf, size);
}