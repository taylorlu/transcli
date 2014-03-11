
#ifndef XMLACCESSOR_LIBXML2_H
#define XMLACCESSOR_LIBXML2_H

#include <stdio.h>
#include "StrProMacro.h"

#ifndef WIN32
#include <pthread.h>
#endif
typedef struct _xmlDoc xmlDoc;
typedef struct _xmlNode xmlNode;

_MC_STRPRO_BEGIN

class MC_STRPRO_EXT CXML2
{
public:
	CXML2(const char* filename = 0);
	~CXML2();
	void New(const char* xmlVersion, const char* rootNodeName, const char* xsltPath = 0);
	int Open(const char* filename, const char* encoding = 0);
	int Open2(const char* filename) {return 0; /*not implement*/};
	int Save(const char* filename, const char* encoding = 0);
	int Read(const char* xmlbuf, int length = 0, const char* url = 0, const char* encoding = 0);
	int Dump(char** buffer, char* encoding = 0);	// Need to free buffer
	int DumpCurrentNode(char** buffer, char* encoding = 0); // Need to free buffer
	static void FreeDump(char** buffer);

	//navigation methods
	void* goToKey(const char *path, int index = 0);
	void* goRoot();
	void* goNext();
	void* goPrev();
	void* goFirst();
	void* goChild();
	void* goParent();

	//get/set methods
	// Clone and append node to childrens
	void* addChild(void* node);
	// Just append node to childrens
	void* appendChild(void* node);
	void* addChild(const char* nodename);
	void* addChild(const char* nodename, const char* value);
	void* addChild(const char* nodename, int value);
	void* addChild(const char* nodename, float value);
	void* addText(const char* text);
	void* addText(int value);
	void* addAttribute(const char* name, const char* value);
	void* setAttribute(const char* name, const char* value);
	void* setAttribute(const char* name, int value);
	void* setAttribute(const char* name, float value);
	void  removeAttribute(const char* name);
	void* findNode(const char* name, const char* attrname = 0, const char* attrvalue = 0);
	void* findNextNode(const char* name, const char* attrname = 0, const char* attrvalue = 0);
	void* findPrevNode(const char* name, const char* attrname = 0, const char* attrvalue = 0);
	void* findChildNode(const char* name, const char* attrname = 0, const char* attrvalue = 0);
	int getChildCount();
	void setNodeName(const char* nodename);
	void setNodeValue(const char* value, bool isCData = false);
	void setNodeValue(int value);
	void setNodeValue(float value);
	char* getNodeName();
	const char* getNodeValue(int index = 0);
	int getNodeValueInt(int index = 0);
	float getNodeValueFloat(int index = 0);
	const char* getNodeValueByName(const char* name, const char* defaultValue = 0);
	const char* getChildNodeValue(const char* name, const char* attrname = 0,  const char* attrvalue = 0, int index = 0);
	int getChildNodeValueInt(const char* name, const char* attrname = 0, const char* attrvalue = 0, int index = 0);
	float getChildNodeValueFloat(const char* name, const char* attrname = 0, const char* attrvalue = 0, int index = 0);
	void setChildNodeValue(const char* name, const char* nodeVal, const char* attrname = 0, const char* attrvalue = 0);
	void setChildNodeValue(const char* name, int nodeVal, const char* attrname = 0, const char* attrvalue = 0);
	void setChildNodeValue(const char* name, float nodeVal, const char* attrname = 0, const char* attrvalue = 0);
	const char* getAttribute(const char* propname, const char* defaultValue = 0, void* node = 0);
	int getAttributeInt(const char* propname, int def = 0, void* node = 0);
	float getAttributeFloat(const char* propname, float def = 0.f, void* node = 0);
	// unlink and free current node, cur become it's parent, return cur
	void* removeNode();		
	// unlink but not free current node, cur become it's parent, return unlinked node
	void* unlinkNode();		
	void* replaceNode(const void* rNode);
	bool isAttrMatched(const char* propname, const char* match);
	bool isMatched(const char* match, bool matchCase = false);
	void SetCurrentNode(void* node);
	void Close();
	void Lock();
	void Unlock();
	static void Init();
	static void Cleanup();
protected:
	xmlNode* cur;
private:
	xmlDoc* m_doc;
	
#ifndef WIN32
    pthread_mutex_t mMutex;
#endif
	void* hLock;
};

_MC_STRPRO_END

#endif
