#include "StrHelper.h"
#include "bit_osdep.h"

_MC_STRPRO_BEGIN
std::string StrHelper::buffer = "";

StrHelper::StrHelper(void)
{
}

StrHelper::~StrHelper(void)
{
}

std::string StrHelper::strToLower( const std::string& inStr )
{
	std::string outStr = "";
	for ( size_t i = 0; i < inStr.length(); i++ )
	{
		outStr += tolower(inStr[i]);
	}
	return outStr;
}

std::string StrHelper::strToUpper( const std::string& inStr )
{
	std::string outStr = "";
	for ( size_t i = 0; i < inStr.length(); i++ )
	{
		outStr += toupper( inStr[i] );
	}
	return outStr;
}

std::string StrHelper::int32ToString( int inInt32, int radix /* = 10 */ )
{
	std::string outStr = "";
	char num[65] = {0};
	_itoa(inInt32, num, radix);
	outStr = num;
	return outStr;
}

std::string StrHelper::replaceAll(const std::string& inStr, const char* relStr /* = 0 */, const char* newStr /* = 0 */)
{
	std::string outStr = inStr;
	if (!relStr || !newStr) {
		return outStr;
	}
	int pos = outStr.find(relStr);
	while(pos != std::string::npos)
	{
		outStr.replace(pos, strlen(relStr), newStr);
		pos = outStr.find(relStr);		
	}
	return outStr;
}

std::string& StrHelper::replaceAllDistinct(std::string& inStr, const char* relStr, const char* newStr)
{
	if(!relStr || !newStr) return inStr;
	size_t newStrLen = strlen(newStr);
	for(std::string::size_type pos = 0; pos!=std::string::npos; pos+=newStrLen)   {   
		if((pos=inStr.find(relStr,pos)) != std::string::npos)   
			inStr.replace(pos, strlen(relStr), newStr);   
		else   break;   
	}   
	return  inStr; 
}

std::string& StrHelper::prepareForXML(std::string& inStr)
{
	const char *entity[] = {"&lt;","&gt;","&amp;","&quot;","&apos;"};
	const char *refStr = "<>&\"\'", *fchar = NULL;

	for(std::string::size_type pos = 0; pos<inStr.length(); pos++)   {   
		if(fchar = strchr(refStr, inStr[pos])) {
			const char* newStr = entity[fchar-refStr];
			inStr.replace(pos, 1, newStr);
			pos += (strlen(newStr) - 1);
		}   
	}   
	return inStr;
}

std::string& StrHelper::TrimLeft(std::string& inStr)
{
	const char* whiteChar = " \r\n\t\f\v";
	size_t foundPos = inStr.find_first_not_of(whiteChar);
	if(foundPos != std::string::npos) {
		inStr.erase(0, foundPos);
	} else {	// All char is white char
		inStr.clear();
	}
	return inStr;
}

std::string& StrHelper::TrimRight(std::string& inStr)
{
	const char* whiteChar = " \r\n\t\f\v";
	size_t foundPos = inStr.find_last_not_of(whiteChar);
	if(foundPos != std::string::npos) {
		inStr.erase(foundPos+1);
	} else {	// All char is white char
		inStr.clear();
	}
	return inStr;
}

bool StrHelper::getFileTitle(const char* fileName, std::string& outTitle)
{
	std::string title(fileName);
	size_t slashIdx = title.find_last_of('\\');
	if(slashIdx == title.npos) {
		slashIdx = title.find_last_of('/');
	}
	if(slashIdx != title.npos) {
		title = title.substr(slashIdx+1);
	}

	size_t dotIdx = title.find_last_of('.');
	if(dotIdx != title.npos) {
		outTitle = title.substr(0, dotIdx);
	} else {
		outTitle = title;
	}
	
	return true;
}

bool StrHelper::getFileFolder(const char* fileName, std::string& outDir)
{
	std::string dir(fileName);
	size_t slashIdx = dir.find_last_of('\\');
	if(slashIdx == dir.npos) {
		slashIdx = dir.find_last_of('/');
	}
	if(slashIdx == dir.npos) {
		return false;
	}
	outDir = dir.substr(0, slashIdx+1);
	return true;
}

bool StrHelper::getFileLeafName(const char* fileName, std::string& leafName)
{
	if(!fileName || !*fileName) return false;
	leafName = fileName;
	size_t slashIdx = leafName.find_last_of('\\');
	if(slashIdx == leafName.npos) {
		slashIdx = leafName.find_last_of('/');
	}
	if(slashIdx != leafName.npos) {
		leafName = leafName.substr(slashIdx+1, leafName.size()-slashIdx-1);
	}
	return true;
}

bool StrHelper::getFileExt(const char* fileName, std::string& ext)
{
	std::string extStr(fileName);
	size_t dotIdx = extStr.find_last_of('.');
	if(dotIdx == extStr.npos) {
		return false;
	}
	ext = extStr.substr(dotIdx);
	return true;
}

bool StrHelper::splitFileName(const char* fileName, std::string& dir, std::string& title, std::string& ext)
{
	std::string originFile(fileName);
	size_t slashIdx = originFile.find_last_of('\\');
	if(slashIdx == originFile.npos) {
		slashIdx = originFile.find_last_of('/');
	}
	
	size_t dotIdx = originFile.find_last_of('.');
	if(dotIdx == originFile.npos) {
		return false;
	}

	if(slashIdx == originFile.npos) {
		dir.clear();	// No dir
		title = originFile.substr(0, dotIdx);
	} else {
		dir = originFile.substr(0, slashIdx+1);
		title = originFile.substr(slashIdx+1, dotIdx-slashIdx-1);
	}
	
	ext = originFile.substr(dotIdx);
	return true;
}

bool StrHelper::isThisStringNumber(const char* inStr)
{
	if (inStr && (*inStr)) {
		while (*inStr) {
			if(*inStr < 0) return false;
			if (!isdigit(*inStr) && (*inStr != '.')) return false;
			inStr++;
		}
		return true;
	}
	return false;
}

bool StrHelper::splitString(std::vector<std::string>& result, const char* str, const char* delimiter)
{
	if(!str) return false;
	std::string src(str);
	size_t delimiterIdx = src.npos;

	while((delimiterIdx=src.find(delimiter, 0)) != src.npos) {
		result.push_back(src.substr(0, delimiterIdx));
		src = src.substr(delimiterIdx+1);
	}
	result.push_back(src);
	return true;
}

_MC_STRPRO_END
