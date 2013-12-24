#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/times.h>
#endif
#include <stdio.h>
#include <fcntl.h>	
#include <sys/stat.h>
#include <list>

#include "MEvaluater.h"
#include "logger.h"
#include "bitconfig.h"
#include "util.h"

using namespace StrPro;
using namespace std;

//============================================================
enum TYPE_NAME
{
	tn_empty = 0,
	tn_int = 1,
	tn_string,
	tn_enum,
	tn_bool,
	tn_float,
	tn_file,
	tn_dir,
	tn_node,
	tn_root,
	tn_null,
};
static const char* typeNames[] = {"", "int", "string", "enum", "bool", "float", "file", "dir", "node", "root", 0};
enum VAR_LIST
{
	VL_DestFile = 0,
	VL_SourceFile,
	VL_SubtitleFile,
	VL_TempDir,
	VL_TrackPath,
	VL_FrameRate,
	VL_index,
	VL_VideoBitrateX1K,
	VL_Title,
	VL_Artist,
	VL_Comment,
	VL_CpuCores,
	VL_Max,
};

//static const char* varNames[] = {"DestFile", "SourceFile", "SubtitleFile", "TempDir", "TrackPath", "FrameRate", "index", "VideoBitrate*1K","Title", "Artist", "Comment", "CpuCores", 0};


MEvaluater::MEvaluater(void)
{

}

MEvaluater::~MEvaluater(void)
{

}

/*
string MEvaluater::parseKeyFromMcCore(CXML* coreXml, const char *key, bool isEnumIndex, bool fileWithQuots, bool isDefault)
{
	if (key == NULL) return "";
	char* tmpKeyStr = strdup(key);

	//search in mccore
	coreXml->goRoot();
	char* tmpStr = tmpKeyStr;
	char* curKey = NULL;
	while (curKey = Strsep(&tmpStr, ".")) {
		if (coreXml->goChild() != NULL) {
			if(coreXml->findNode("node", "key", curKey) == NULL) break;
		} else {
			break;
		}
	}
	free(tmpKeyStr);

	const char* ctype = coreXml->getAttribute("type");
	if (ctype == NULL) {
		return "";
	}
	
	return getMcCoreNodeValue(coreXml, ctype, isEnumIndex, isDefault);
}
*/

const char* MEvaluater::getMcCoreNodeValue(StrPro::CXML2* pCoreXml, const char* nodeType, bool isEnumIndex, bool isDefault)
{
	if (strcmp(nodeType, typeNames[tn_enum]) == 0) {
		int count = 0; 
		bool isSelDef = false; //the default is with the sel symbol 
		const char* firstEnumStr = NULL;
		const char* selEnumStr = NULL;
		pCoreXml->goChild();
		while (pCoreXml->findNode(typeNames[tn_enum]) != NULL) {
			const char* sel = pCoreXml->getAttribute("sel");
			const char* cval = pCoreXml->getNodeValue();
			if(!firstEnumStr) firstEnumStr = cval;
			if (sel != NULL) {
				if (!isEnumIndex) {
					return cval;
				}
				isSelDef = true;
				selEnumStr = cval;
				break;
			}
			
			count++;
			if(!pCoreXml->goNext()) break; 			
		}

		if (isSelDef) {
			if (!isEnumIndex) {
				return selEnumStr;
			} else {
				switch(count) {	// Count won't exceed 30
				case 0:  return "0";  case 1:  return "1";  case 2:  return "2";  case 3:  return "3";  case 4:  return "4";
				case 5:  return "5";  case 6:  return "6";  case 7:  return "7";  case 8:  return "8";  case 9:  return "9";
				case 10: return "10"; case 11: return "11"; case 12: return "12"; case 13: return "13"; case 14: return "14";
				case 15: return "15"; case 16: return "16"; case 17: return "17"; case 18: return "18"; case 19: return "19";
				case 20: return "20"; case 21: return "21"; case 22: return "22"; case 23: return "23"; case 24: return "24";
				case 25: return "25"; case 26: return "26"; case 27: return "27"; case 28: return "28"; case 29: return "29";
				case 30: return "30"; case 31: return "31"; case 32: return "32"; case 33: return "33"; case 34: return "34";
				default: return "0";
				}
			}
		} else {
			if (!isEnumIndex) {
				return firstEnumStr;
			} else {
				return "0";
			}
		}
	}
	else if (!strcmp(nodeType, typeNames[tn_int]) || !strcmp(nodeType, typeNames[tn_float]))
	{
		const char* cmax = pCoreXml->getAttribute("max");
		const char* cmin = pCoreXml->getAttribute("min");
		const char* cval = NULL;
		
		if(pCoreXml->findChildNode("value") != NULL) {
			cval = pCoreXml->getNodeValue();
		}
		
		if (cval == NULL) {
			cval = "0";
		}

		if (cmax != NULL) {
			if(atof(cval) > atof(cmax)) {
				return cmax;
			}
		}

		if (cmin != NULL) {
			if (atof(cval) < atof(cmin)) {
				return cmin;
			}
		}
		return cval;
	}
	else if (strcmp(nodeType, typeNames[tn_bool]) == 0)
	{
		if(pCoreXml->findChildNode("value") != NULL) {
			return  pCoreXml->getNodeValue();
		} else {
			return "false";
		}
	}
	else if (strcmp(nodeType, typeNames[tn_file]) == 0)
	{
		if(pCoreXml->findChildNode("value") != NULL) {
			return  pCoreXml->getNodeValue();
		}
	}
	else
	{
		if(pCoreXml->findChildNode("value") != NULL) {
			return pCoreXml->getNodeValue();
		}
	}
	
	return NULL;
}


//int MEvaluater::GetCoreXmlEnumValue(StrPro::CXML *coreXml, const char *key)
//{
//	string res = parseKeyFromMcCore(coreXml, key);
//	if (!res.empty()) {
//		return atoi(res.c_str());
//	}
//	return 0;
//}

// char* MEvaluater::GetCoreXmlEnumString(StrPro::CXML *prefXml, const char *key)
// {
// 	CXML *coreXml = GetMCcorePrefInstance();
// 	if (coreXml && key && prefXml)
// 	{
// 		string skey = key;
// 		string oldkey = key;		
// 		const char* ctype;
// 		char *token, *nextToken;
// 		const char* oldval = prefXml->getAttribute("value");
// 		if (!oldval) {
// 			oldval = prefXml->getNodeValue();
// 		}
// 
// 		if(oldval && !StrHelper::isThisStringNumber(oldval)) {
// 			return _strdup(oldval);
// 		}
// 
// 		//goto the key node
// 		token = strtok_s((char*)skey.c_str(), ".", &nextToken);
// 		while (token != NULL) {
// 			if (coreXml->goChild() && coreXml->findNode("node", "key", token)) {
// 				token = strtok_s(NULL, ".", &nextToken);
// 			} else {
// 				break;
// 			}
// 		}
// 		//parse type
// 		ctype = coreXml->getAttribute("type");
// 		//enum to index
// 		if (ctype && oldval && strcmp(ctype, typeNames[tn_enum]) == 0)
// 		{
// 			int oindex = atoi(oldval);
// 			int index = 0;
// 			void* node = coreXml->findChildNode("enum");
// 			while(node) {
// 				const char* enumval = coreXml->getNodeValue();
// 				if(enumval && oindex == index) {
// 					return _strdup(enumval);
// 				}
// 				index++;
// 				node = coreXml->findNextNode("enum");
// 			}
// 		}
// 		else if (oldval) {
// 			return _strdup(oldval);
// 		}
// 	}
// 	return NULL;
// }

const char* MEvaluater::GetCoreXmlFormatValue(const char *key, const char* prefKeyVal)
{
	if(!key) return NULL;

	// Get value of key in preset
	if(prefKeyVal && *prefKeyVal) {
		 if(StrHelper::isThisStringNumber(prefKeyVal) || !strcmp(prefKeyVal, "true") ||
			 !strcmp(prefKeyVal, "false"))  {	// Number or bool type, return immediately
			 return NULL/*prefKeyVal*/;		// Original value, don't change
		 }
	}
		
	// If type is enum or prefKeyVal is null, get index or default value from mccore
	CXML2 *coreXml = GetMCcoreDoc();
	if(!coreXml) return NULL;

	char* tmpKeyStr = _strdup(key);
	char* tmpStr = tmpKeyStr;
	char* curKey = NULL;
	while (curKey = Strsep(&tmpStr, ".")) {
		if (!coreXml->goChild() || !coreXml->findNode("node", "key", curKey)) {
			break;
		}
	}
	free(tmpKeyStr);

	const char* nodeType = coreXml->getAttribute("type");
	if(!nodeType || !(*nodeType)) {	// Unknown type
		if(prefKeyVal && *prefKeyVal) {
			return NULL/*prefKeyVal*/;		// Original value, don't change
		} 
		return NULL;
	} 

	// If node type is known and prefKeyVal is invalid, then get default value from mccore
	if(!prefKeyVal || !(*prefKeyVal)) {
		return getMcCoreNodeValue(coreXml, nodeType);
	}

	if (strcmp(nodeType, typeNames[tn_enum]) != 0) {	// not enum type
		return NULL/*prefKeyVal*/;		// Original value, don't change
	} else {	// enum type
		//enum to index
		int index = 0;
		void* node = coreXml->findChildNode("enum");
		while(node) {
			const char* enumval = coreXml->getNodeValue();
			if (enumval && strcmp(enumval, prefKeyVal) == 0) {
				switch(index) {	// Count won't exceed 30
				case 0:  return "0";  case 1:  return "1";  case 2:  return "2";  case 3:  return "3";  case 4:  return "4";
				case 5:  return "5";  case 6:  return "6";  case 7:  return "7";  case 8:  return "8";  case 9:  return "9";
				case 10: return "10"; case 11: return "11"; case 12: return "12"; case 13: return "13"; case 14: return "14";
				case 15: return "15"; case 16: return "16"; case 17: return "17"; case 18: return "18"; case 19: return "19";
				case 20: return "20"; case 21: return "21"; case 22: return "22"; case 23: return "23"; case 24: return "24";
				case 25: return "25"; case 26: return "26"; case 27: return "27"; case 28: return "28"; case 29: return "29";
				case 30: return "30"; case 31: return "31"; case 32: return "32"; case 33: return "33"; case 34: return "34";
				default: return "0";
				}
			}
			index++;
			node = coreXml->findNextNode("enum");
		}
	}

	return NULL;
}

//#include "encrypt.h"
CXML2* MEvaluater::GetMCcoreDoc()
{
	static CXML2 xml;
	if (xml.goRoot()) {
		return &xml;
	} else {
//#ifdef TS_AUTH_BY_HTTP
		if(xml.Open(MCCORE_XML) == 0) {
			return &xml;
		}
//#else
//		std::string mcntContent = DecryptMcnt();
//		if(xml.Read(mcntContent.c_str()) == 0) {
//			return &xml;
//		}
//#endif
	}
	return NULL;
}

const char* MEvaluater::GetDefaultNodeValueByKey(const char* key)
{
	CXML2* defaultPref = getDefaultPrefInstance();
	if(!defaultPref) return NULL;

	defaultPref->goRoot();
	if(defaultPref->findChildNode("node", "key", key)) {
		return defaultPref->getNodeValue();			
	}
	return NULL;
}

CXML2* MEvaluater::getDefaultPrefInstance()
{
	static CXML2 defaultPref;
	if (defaultPref.goRoot()) {
		return &defaultPref;
	} else {
		CXML2* mcCorePref = GetMCcoreDoc();
		if(mcCorePref && mcCorePref->goRoot()) {
			if(initDefaultPresetFromMcCore(mcCorePref, defaultPref)) {
				return &defaultPref;
			}
		}
	}
	return NULL;
}

const char* MEvaluater::GetPresetKeyValue(CXML2* prefSetXml, const char *key)
{
	if (!prefSetXml || !key)
	{
		return false;
	}
	prefSetXml->goRoot();
	if(prefSetXml->findChildNode("node", "key", key))
	{
		return prefSetXml->getNodeValue();
	}
	else 
		return 0;	
}

bool MEvaluater::SetPresetKeyValue(StrPro::CXML2* prefSetXml, const char* key, const char* value, bool isAdd)
{
	void* currentNode = 0;
	if (!prefSetXml || !key)
	{
		return false;
	}
	prefSetXml->goRoot();
	currentNode = prefSetXml->findChildNode("node", "key", key);
	if (currentNode)
	{	
		const char* defval; 
		prefSetXml->SetCurrentNode(currentNode);
		defval = prefSetXml->getNodeValue();
		if(defval && strcmp(defval, value) == 0)
		{
			prefSetXml->setAttribute("flag", 0);
		}
		else
		{
			prefSetXml->setAttribute("flag", 1);
		}
		
		prefSetXml->setNodeValue(value, true);
		return true;
	}
	else
	{
		if (isAdd)
		{
			prefSetXml->goRoot();
			void* node = prefSetXml->addChild("node");			
			if(node)
			{
				prefSetXml->SetCurrentNode(node);
				prefSetXml->addAttribute("key", key);
				prefSetXml->setAttribute("flag", 1);
				prefSetXml->setNodeValue(value, true);
			}
			else 
				return false;			
			return true;
		}
		else
			return false;
	}

}
bool MEvaluater::SetPresetKeyValue(StrPro::CXML2* prefSetXml, const char* key, int value, bool isAdd)
{
	void* currentNode = 0;
	if (!prefSetXml || !key)
	{
		return false;
	}
	prefSetXml->goRoot();
	currentNode = prefSetXml->findChildNode("node", "key", key);
	if (currentNode)
	{	
		const char* defval; 
		prefSetXml->SetCurrentNode(currentNode);
		defval = prefSetXml->getNodeValue();
		if (defval && atoi(defval) == value)
		{
			prefSetXml->setAttribute("flag", 0);
		}
		else
		{
			prefSetXml->setAttribute("flag", 1);
		}
		prefSetXml->setNodeValue(value);
		return true;
	}
	else
	{
		if (isAdd)
		{
			prefSetXml->goRoot();
			void* node = prefSetXml->addChild("node");			
			if(node)
			{
				prefSetXml->SetCurrentNode(node);
				prefSetXml->addAttribute("key", (char*)key);
				prefSetXml->setAttribute("flag", 1);
				prefSetXml->setNodeValue(value);
			}
			else 
				return false;			
			return true;
		}
		else
			return false;
	}

}


bool MEvaluater::SetPresetKeyValue(StrPro::CXML2* prefSetXml, const char* key, float value, bool isAdd)
{
	void* currentNode = 0;
	if (!prefSetXml || !key)
	{
		return false;
	}
	prefSetXml->goRoot();
	currentNode = prefSetXml->findChildNode("node", "key", key);
	if (currentNode)
	{	
		const char* defval; 
		prefSetXml->SetCurrentNode(currentNode);
		defval = prefSetXml->getNodeValue();
		if (defval && atof(defval) == value)
		{
			prefSetXml->setAttribute("flag", 0);
		}
		else
		{
			prefSetXml->setAttribute("flag", 1);
		}
		prefSetXml->setNodeValue(value);
		return true;
	}
	else
	{
		if (isAdd)
		{
			prefSetXml->goRoot();
			void* node = prefSetXml->addChild("node");			
			if(node)
			{
				prefSetXml->SetCurrentNode(node);
				prefSetXml->addAttribute("key", (char*)key);
				prefSetXml->setAttribute("flag", 1);
				prefSetXml->setNodeValue(value);
			}
			else 
				return false;			
			return true;
		}
		else
			return false;
	}

}

bool MEvaluater::UpdatePresets(StrPro::CXML2 *pref, StrPro::CXML2& outPrefs)
{
	if(pref && pref->goRoot())
	{
		void* curNode = pref->goChild();
		while(curNode)
		{
			const char* key = 0;
			const char* val = 0;
			//int flag = pref = pref->getAttribute("flag");
			key = pref->getAttribute("key");
			val = pref->getNodeValue();
			if (key && val)
			{
				SetPresetKeyValue(&outPrefs, key, val, true);
			}
			curNode = pref->goNext();
		}
		return true;
	}
	
	return false;
}

bool MEvaluater::RevertPresets(StrPro::CXML2& outPrefs)
{
	if (outPrefs.goRoot())
	{
		void* curNode = outPrefs.goChild();
		while (curNode)
		{
			const char* val = outPrefs.getAttribute("default");
			if (val) {
				outPrefs.setNodeValue(val);
			} else {
				outPrefs.setNodeValue(" ");
			}
			curNode = outPrefs.goNext();
		}
		return true;
	}
	return false;
}


bool MEvaluater::initDefaultPresetFromMcCore(CXML2 *mcCorePref, CXML2 &outPrefs)
{
	void* currentNode = mcCorePref->goRoot();
	if (currentNode == NULL || strcmp(mcCorePref->getNodeName(), "MediaCoderPrefs") != 0) {
		logger_err(LOGM_UTIL_ROOTPREFS, "Bad format of mediacoder prefs.\n");
		return false;
	}

	outPrefs.New("1.0", "MediaCoderPrefs");
	
	list<string> keyToks;
	for(;;) {
		currentNode = mcCorePref->findChildNode("node");
		if (currentNode != NULL) {		
			const char* type = mcCorePref->getAttribute("type");
			if (strcmp(type, "node") == 0) {
				//go deep
				keyToks.push_back(mcCorePref->getAttribute("key"));
			} else {
				//add nodes
				string nodeKeyPrefix = "";
				list<string>::iterator iter = keyToks.begin();
				for (;iter != keyToks.end();) {
					nodeKeyPrefix += (*iter) + ".";
					++iter;
				}
				//parse key
				for(; currentNode != NULL; ) {	
					//mcCorePref->SetCurrentNode(currentNode);
					std::string nodeKey = nodeKeyPrefix + mcCorePref->getAttribute("key"); 
					const char* nodeType = mcCorePref->getAttribute("type");
					const char* nodeValue = "";
					if(nodeType) {
						nodeValue = getMcCoreNodeValue(mcCorePref, nodeType);
						mcCorePref->SetCurrentNode(currentNode);
					}
					
					if(!nodeKey.empty() && nodeKey.back() != '.') {
						outPrefs.goRoot();
						void* cnode = outPrefs.addChild("node");
						outPrefs.SetCurrentNode(cnode);
						outPrefs.addAttribute("key", nodeKey.c_str());
						outPrefs.setNodeValue(nodeValue, true);
					}
					currentNode = mcCorePref->findNextNode("node");
				}
			}
		} else {	
			keyToks.pop_back();	
			currentNode = mcCorePref->findNextNode("node");
			if (currentNode != NULL) {
				keyToks.push_back(mcCorePref->getAttribute("key"));
				continue;
			}
		}

		//nodes up
		if(currentNode == NULL) {
			while(keyToks.size() > 0 ) {
				mcCorePref->goParent();
				keyToks.pop_back();
				currentNode = mcCorePref->findNextNode("node");
				if (currentNode != NULL) {
					keyToks.push_back(mcCorePref->getAttribute("key"));
					break;
				}
			}	
		}

		//exit
		if (keyToks.size() == 0 && currentNode == NULL) {
			break;
		}	
	}
	
// 	char* tempstr;
// 	outPrefs.Dump(&tempstr);
	return true;
}

bool MEvaluater::SavePresetFile(CXML2* prefs, const char* path)
{
	char* buffer = 0;
	int size = 0, rsize;

#ifdef WIN32
	int ofd =_open(path, _O_CREAT | _O_BINARY | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
#else
	int ofd =_open(path, _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE);
#endif

	if (ofd < 0) return false;

	size = prefs->Dump(&buffer);
	if(buffer) {
		rsize = _write(ofd, buffer, size);
		prefs->FreeDump(&buffer);
	}
	_close(ofd);
	return rsize == size;
}

//bool MEvaluater::CreateRootXml(StrPro::CXML2& outPrefs)
//{
//	const char* str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><root><input/><output/></root>";
//
//	if(outPrefs.Read(str)!= 0)
//		return false;
//	return true;
//}

//CXMLPref* MEvaluater::CreateStreamXml(const char* streamType)
//{
//	CXMLPref* pPref = new CXMLPref(streamType);
//	return pPref;
//}

//bool MEvaluater::SetInputNodeValue(StrPro::CXML* streamsPrefs, const char* nodeName,  const char* value)
//{
//	if(streamsPrefs && streamsPrefs->goRoot() && streamsPrefs->findChildNode("input"))
//	{
//		if (streamsPrefs->findChildNode(nodeName))
//		{
//			streamsPrefs->setNodeValue(value);
//		}
//		else
//		{
//			streamsPrefs->addChild(nodeName, value);
//		}
//		return true;
//	}
//	return false;
//}
//
//const char* MEvaluater::GetInputNodeValue(StrPro::CXML* streamsPrefs, const char* nodeName)
//{
//	if(streamsPrefs && streamsPrefs->goRoot() && streamsPrefs->findChildNode("input") && streamsPrefs->findChildNode(nodeName))
//	{
//		return streamsPrefs->getNodeValue();
//	}
//
//	return NULL;
//}

//int MEvaluater::AddStream(StrPro::CXML* streamsPrefs, const char* type)
//{
//	if(streamsPrefs && streamsPrefs->goRoot() && streamsPrefs->findChildNode("output"))
//	{		
//		int count = streamsPrefs->getChildCount();
//		void* node = streamsPrefs->addChild("stream");
//		streamsPrefs->SetCurrentNode(node);
//		streamsPrefs->setAttribute("type", type);
//		
//		//return Index
//		return count;
//	}
//	return -1;
//}
//
//int	MEvaluater::AddMuxer(StrPro::CXML* streamsPrefs)
//{
//	if(streamsPrefs && streamsPrefs->goRoot() && streamsPrefs->findChildNode("output"))
//	{		
//		int count = streamsPrefs->getChildCount();
//		streamsPrefs->addChild("muxer");
//		
//		//return Index
//		return count;
//	}
//	return -1;
//}

//int MEvaluater::GetStreamsCount(StrPro::CXML* streamsPrefs)
//{
//	if(streamsPrefs && streamsPrefs->goRoot() && streamsPrefs->findChildNode("output"))
//	{		
//		int count = 0;
//		void *node = streamsPrefs->findChildNode("stream");
//		while(node)
//		{
//			++count;
//			node = streamsPrefs->findNextNode("stream");
//		}
//		return count;		
//	}
//	return 0;
//}
//
//int MEvaluater::GetMuxersCount(StrPro::CXML* streamsPrefs)
//{
//	if(streamsPrefs && streamsPrefs->goRoot() && streamsPrefs->findChildNode("output"))
//	{		
//		int count = 0;
//		void *node = streamsPrefs->findChildNode("muxer");
//		while(node)
//		{
//			++count;
//			node = streamsPrefs->findNextNode("muxer");
//		}
//		return count;		
//	}
//	return 0;
//}
//
//const char* MEvaluater::GetOutputKeyValue(StrPro::CXML* streamsPrefs, const char* key, int index /* = 0 */)
//{
//	if(streamsPrefs && streamsPrefs->goRoot()&& streamsPrefs->findChildNode("output"))
//	{		
//		int count = -1;
//		void* node = streamsPrefs->goChild();
//		while(count < index && node)
//		{
//			node = streamsPrefs->goNext();
//			count++;
//		}
//		if (node)
//		{
//			if(streamsPrefs->findChildNode("node", "key", key))
//				return streamsPrefs->getNodeValue();
//		}		
//	}
//	return NULL;
//}
//
//bool MEvaluater::SetOutputKeyValue(StrPro::CXML* streamsPrefs, const char* key, const char* value, int index /* = 0 */, bool isAdd /* = false */)
//{
//	if(streamsPrefs && streamsPrefs->goRoot()&& streamsPrefs->findChildNode("output"))
//	{
//		int count = -1;
//		void* node = streamsPrefs->goChild();
//		while(count < index && node)
//		{
//			node = streamsPrefs->goNext();
//			count++;
//		}
//		if (node)
//		{
//			if(streamsPrefs->findChildNode("node", "key", key))
//			{
//				streamsPrefs->setNodeValue(value);
//			}
//			else
//			{
//				if (isAdd)
//				{
//					void* cnode = streamsPrefs->addChild("node");
//					if(cnode)
//					{
//						streamsPrefs->SetCurrentNode(cnode);
//						streamsPrefs->addAttribute("key", key);
//						streamsPrefs->setNodeValue(value);
//						return true;
//					}				
//				}
//			}
//			return true;
//		}
//	}
//	return false;
//}
//
//bool MEvaluater::SetOutputKeyValue(StrPro::CXML* streamsPrefs, const char* key, int value, int index /* = 0 */, bool isAdd /* = false */)
//{
//	if(streamsPrefs && streamsPrefs->goRoot()&& streamsPrefs->findChildNode("output"))
//	{
//		int count = -1;
//		void* node = streamsPrefs->goChild();
//		while(count < index && node)
//		{
//			node = streamsPrefs->goNext();
//			count++;
//		}
//		if (node)
//		{
//			if(streamsPrefs->findChildNode("node", "key", key))
//			{
//				streamsPrefs->setNodeValue(value);
//			}
//			else
//			{
//				if (isAdd)
//				{
//					void* cnode = streamsPrefs->addChild("node");
//					if(cnode)
//					{
//						streamsPrefs->SetCurrentNode(cnode);
//						streamsPrefs->addAttribute("key", key);
//						streamsPrefs->setNodeValue(value);
//						return true;
//					}				
//				}
//			}
//			return true;
//		}
//	}
//	return false;
//}
//
//bool MEvaluater::SetOutputKeyValue(StrPro::CXML* streamsPrefs, const char* key, float value, int index /* = 0 */, bool isAdd /* = false */)
//{
//	if(streamsPrefs && streamsPrefs->goRoot()&& streamsPrefs->findChildNode("output"))
//	{
//		int count = -1;
//		void* node = streamsPrefs->goChild();
//		while(count < index && node) {
//			node = streamsPrefs->goNext();
//			count++;
//		}
//
//		if (node)
//		{
//			if(streamsPrefs->findChildNode("node", "key", key)) {
//				streamsPrefs->setNodeValue(value);
//			} else {
//				if (isAdd) {
//					void* cnode = streamsPrefs->addChild("node");
//					if(cnode) {
//						streamsPrefs->SetCurrentNode(cnode);
//						streamsPrefs->addAttribute("key", key);
//						streamsPrefs->setNodeValue(value);
//						return true;
//					}				
//				}
//			}
//			return true;
//		}
//	}
//	return false;
//}

// bool MEvaluater::InitStreamsXml(StrPro::CXML *streamsPrefs, StrPro::CXML* mcXml)
// {
// 	const char* str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><MediaCoderPrefs/>";
// 	CXML2 tempPref;
// 	tempPref.Read(str);
// 	if (streamsPrefs && streamsPrefs->goRoot() && streamsPrefs->findChildNode("output")) {	
// 		//group	
// 		if(mcXml) {
// 			void* stream = streamsPrefs->goChild();
// 			while(stream) {
// 				void* rNode = NULL;
// 				//use temp to hold the prefs
// 				tempPref.goRoot();
// 				tempPref.replaceNode(stream);
// 				//merge the default
// 				UpdatePresets(&tempPref, *mcXml);
// 				rNode = mcXml->goRoot();
// 				if(rNode) {
// 					stream = streamsPrefs->replaceNode(rNode);
// 					streamsPrefs->SetCurrentNode(stream);
// 					//use the old attribute
// 					tempPref.goRoot();
// 					streamsPrefs->setNodeName(tempPref.getNodeName());
// 					if(tempPref.getAttribute("type"))
// 						streamsPrefs->setAttribute("type", tempPref.getAttribute("type"));
// 				}					
// 				stream = streamsPrefs->goNext();
// 			}
// 		}
// 		return true;
// 	}
// 	return false;
// }

// std::string MEvaluater::FillOutputNodeWithPresetFile(const char* rootPref)
// {
// 	CXML2 doc;
// 	if (!rootPref || doc.Read(rootPref) != 0) {
// 		return "";
// 	}
// 
// 	if(doc.findChildNode("output") == NULL) {
// 		return rootPref;
// 	}
// 
// 	void* node = doc.findChildNode("stream");
// 	while(node) {
// 		const char* preset = doc.getAttribute("preset");
// 		CXML2 tempDoc;
// 		if (preset && tempDoc.Open(preset) == 0) {
// 			void* knode = tempDoc.findChildNode("node");
// 			while(knode) {
// 				const char* key = tempDoc.getAttribute("key");
// 				if(!doc.findChildNode("node","key",key)) {
// 					doc.addChild(knode);
// 					knode = tempDoc.findNextNode("node");
// 				} else {
// 					doc.goParent();
// 					knode = tempDoc.findNextNode("node");
// 				}
// 			}					
// 		}
// 		doc.SetCurrentNode(node);
// 		doc.removeAttribute("preset");
// 		node = doc.findNextNode("stream");
// 	}
// 			
// 	doc.goParent();
// 	node = doc.findChildNode("muxer");
// 	while(node) {
// 		const char* preset = doc.getAttribute("preset");
// 		CXML2 tempDoc;
// 		if (preset && tempDoc.Open(preset) == 0) {
// 			void* knode = tempDoc.findChildNode("node");
// 			while(knode) {
// 				const char* key = tempDoc.getAttribute("key");
// 				if(!doc.findChildNode("node","key",key)) {
// 					doc.addChild(knode);
// 					knode = tempDoc.findNextNode("node");
// 				} else {
// 					doc.goParent();
// 					knode = tempDoc.findNextNode("node");
// 				}
// 			}					
// 		}
// 		doc.SetCurrentNode(node);
// 		doc.removeAttribute("preset");
// 		node = doc.findNextNode("muxer");
// 	}
// 	char* ostr = NULL; 
// 	doc.Dump(&ostr);
// 	return string(ostr);
// }
