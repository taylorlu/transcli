
#include <fcntl.h>
#include "bit_osdep.h"
#include "xmlaccessor2.h"
#include "libxml/xpath.h"

_MC_STRPRO_BEGIN


#ifdef WIN32
CXML2::CXML2(const char* filename):cur(0), m_doc(0),hLock(0)
{
	//Init();
#else
CXML2::CXML2(const char* filename):cur(0),m_doc(0)
{
    //Init();
    pthread_mutex_init(&mMutex, NULL);
#endif
	Open(filename);
}

CXML2::~CXML2()
{
	Close();
}

void CXML2::Init()
{
	xmlInitParser();
}

void CXML2::Cleanup()
{
	xmlCleanupParser();
}

void CXML2::New(const char* xmlVersion, const char* rootNodeName, const char* xsltPath)
{
	Close();
	//xmlInitParser();
	m_doc = xmlNewDoc((xmlChar*)xmlVersion);
	cur = xmlNewDocNode(m_doc, 0, (xmlChar*)rootNodeName, 0);
	xmlDocSetRootElement(m_doc, cur);
	cur = xmlDocGetRootElement(m_doc);
	if (xsltPath) {
		char *style = (char*)malloc(strlen(xsltPath) + 32);
		sprintf(style, "type=\"text/xsl\" href=\"%s\"", xsltPath);
		xmlNodePtr pi = xmlNewDocPI(m_doc, (const xmlChar*)"xml-stylesheet ", (const xmlChar *)style);
		free(style);
		xmlAddPrevSibling(cur, pi);
	}
}

int CXML2::Open(const char* filename, const char* encoding)
{
	if(!filename || !(*filename)) return -1;

	Close();
	m_doc = xmlReadFile(filename, NULL, XML_PARSE_RECOVER | XML_PARSE_NODICT);
	if (m_doc == NULL ) {
		return -1;
	}
	
	cur = xmlDocGetRootElement(m_doc);
	return 0;
}

int CXML2::Save(const char* filename, const char* encoding)
{
	int ret = 0;
	if (encoding)
		ret = xmlSaveFileEnc(filename, m_doc, "UTF-8");
	else
		ret = xmlSaveFile(filename, m_doc);
	return ret;
}

int CXML2::Read(const char* xmlbuf, int length, const char* url, const char* encoding)
{
	Close();
	if (!xmlbuf) return -1;
	if (!length) length = (int)strlen(xmlbuf);
	m_doc = xmlReadMemory(xmlbuf, length, url ? url : "data.xml", encoding, XML_PARSE_NODICT);
	if (!m_doc) return -1;
	cur = xmlDocGetRootElement(m_doc);
	return cur ? 0 : -1;
}

int CXML2::Dump(char** buffer, char* encoding)
{
	int bufsize = 0;
	if(!buffer) return 0;
	if (!encoding)
		xmlDocDumpFormatMemory(m_doc, (xmlChar**)buffer, &bufsize, 1);
	else
		xmlDocDumpFormatMemoryEnc(m_doc, (xmlChar**)buffer, &bufsize, encoding, 1);
	return bufsize;
}

void CXML2::FreeDump(char** buffer)
{
	if (*buffer) {
		xmlFree(*buffer);
		*buffer = NULL;
	}
}

int CXML2::DumpCurrentNode(char** buffer, char* encoding)
{
	*buffer = (char*)xmlNodeGetContent(cur);
	return 0;
}

void CXML2::Close()
{
	if (m_doc) {
		xmlFreeDoc(m_doc);
		m_doc = 0;
	}
}

void* CXML2::goToKey(const char* path, int index)
{
	xmlXPathObjectPtr result;
	xmlNodePtr node = 0;
	xmlXPathContextPtr context = xmlXPathNewContext(m_doc);
	if (context == NULL) {
		printf("Error in xmlXPathNewContext\n");
		return 0;
	}
	result = xmlXPathEvalExpression((xmlChar*)path, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		printf("Error in xmlXPathEvalExpression\n");
		return 0;
	}
	if (result->nodesetval && result->nodesetval->nodeNr > index) {
		node = result->nodesetval->nodeTab[index];
		cur = node;
	}
	xmlXPathFreeObject(result);
	return node;
}

int CXML2::getChildCount()
{
	int count = 0;
	for (xmlNodePtr node = cur->children; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE) count++;
	}
	return count;
}

void* CXML2::goRoot()
{
	return (cur = xmlDocGetRootElement(m_doc));
}

void* CXML2::goNext()
{
	xmlNodePtr node = cur->next;
	while (node && node->type != XML_ELEMENT_NODE) node = node->next;
	if (node) cur = node;
	return node;
}

void* CXML2::goPrev()
{
	xmlNodePtr node = cur->prev;
	while (node && node->type != XML_ELEMENT_NODE) node = node->prev;
	if (node) cur = node;
	return node;
}

void* CXML2::goFirst()
{
	if (!cur) return 0;
	xmlNodePtr node = cur;
	while (node->prev) node = node->prev;
	while (node->type != XML_ELEMENT_NODE) node = node->next;
	if (node) cur = node;
	return node;
}

void* CXML2::goChild()
{
	xmlNodePtr node = cur->children;
	while (node && node->type != XML_ELEMENT_NODE) node = node->next;
	if (node) cur = node;
	return node;
}

void* CXML2::goParent()
{
	xmlNodePtr node = cur->parent;
	while (node && node->type != XML_ELEMENT_NODE) node = node->parent;
	if (node) cur = node;
	return node;
}

void CXML2::SetCurrentNode(void* node)
{
	cur = (xmlNodePtr)node;
}

#define FIND_NODE_CRITERIA bool bFind = false; \
	if(node->type == XML_ELEMENT_NODE && !xmlStrcmp(node->name, (const xmlChar*)name)) { \
		if(attrname && attrvalue) { \
			if( !strcmp(attrvalue, getAttribute(attrname, "", node)) ) bFind = true; \
		} else if(attrname) { \
			if(getAttribute(attrname, NULL, node)) bFind = true; \
		} else {bFind = true;} \
	} \
	if(bFind) {cur = node; return node;}

void* CXML2::findNextNode(const char* name, const  char* attrname, const char* attrvalue)
{
	if (!cur || !name) return 0;
	xmlNodePtr node = cur->next;
	while (node) {
		FIND_NODE_CRITERIA;
		node = node->next;
	}
	return 0;
}

void* CXML2::findNode(const char* name, const char* attrname, const char* attrvalue)
{
	if (!cur || !name) return 0;
	xmlNodePtr node = cur;

	while (node) {
		FIND_NODE_CRITERIA;
		node = node->next;
	}
	return 0;
}
void* CXML2::findPrevNode(const char* name, const char* attrname, const char* attrvalue)
{
	if (!cur || !name) return 0;
	xmlNodePtr node = cur->prev;
	// look backward
	while (node) {
		FIND_NODE_CRITERIA;
		node = node->prev;
	}
	return 0;
}

void* CXML2::findChildNode(const char* name, const char* attrname, const char* attrvalue)
{
	if (!cur || !name) return 0;
	xmlNodePtr node = cur->children;
	while (node) {
		FIND_NODE_CRITERIA;
		node = node->next;
	}
	return 0;
}
#undef FIND_NODE_CRITERIA

bool CXML2::isAttrMatched(const char* propname, const char* match)
{
	const char *attr = getAttribute(propname);
	return (attr && !strcmp(attr, match));
}

bool CXML2::isMatched(const char* match, bool matchCase)
{
	if (matchCase)
		return !xmlStrcmp(cur->name, (const xmlChar *)match);
	else
		return !_stricmp((char*)cur->name, match);
}

const char* CXML2::getNodeValue(int index)
{
	xmlNodePtr node = cur->children;
	int i = 0;
	while (node) {
		if ((node->type == XML_TEXT_NODE || node->type == XML_CDATA_SECTION_NODE) && i++ == index) break;
		node = node->next;
	}
	if (!node) return 0;
	char* text = (char*)node->content;
	while (*text == '\n' || *text == '\r' || *text == '\t' || *text == ' ') text++;
	return *text ? text : 0;
}

int CXML2::getNodeValueInt(int index)
{
	const char* val = getNodeValue(index);
	return val ? atoi(val) : 0;
}

float CXML2::getNodeValueFloat(int index)
{
	const char* val = getNodeValue(index);
	return val ? (float)atof(val) : 0.f;
}

const char* CXML2::getNodeValueByName(const char* name, const char* defaultValue)
{
	if (!cur) return defaultValue;
	xmlNodePtr node = cur;
	do {
		if (node->type == XML_ELEMENT_NODE && !xmlStrcmp(node->name, (xmlChar*)name)) {
			cur = node;
			return getNodeValue();
		}
		node = node->prev;
	} while (node);
	goNext();
	return findNode(name) ? getNodeValue() : defaultValue;
}

const char* CXML2::getChildNodeValue(const char* name, const char* attrname /* = 0 */, const char* attrvalue /* = 0 */, int index /* = 0 */)
{
	if (findChildNode(name, attrname, attrvalue)) {
		const char* val = getNodeValue(index);
		goParent();
		return val;
	}
	return NULL;
}

int CXML2::getChildNodeValueInt(const char* name, const char* attrname, const char* attrvalue , int index)
{
	const char* val = getChildNodeValue(name, attrname, attrvalue, index);
	return val ? atoi(val) : 0;
}

float CXML2::getChildNodeValueFloat(const char* name, const char* attrname, const char* attrvalue, int index)
{
	const char* val = getChildNodeValue(name, attrname, attrvalue, index);
	return val ? (float)atof(val) : 0.f;
}

void CXML2::setChildNodeValue(const char* name, const char* nodeVal, const char* attrname, const char* attrvalue)
{
	if (findChildNode(name, attrname, attrvalue)) {
		setNodeValue(nodeVal);
		goParent();
	}
}

void CXML2::setChildNodeValue(const char* name, int nodeVal, const char* attrname, const char* attrvalue)
{
	if (findChildNode(name, attrname, attrvalue)) {
		setNodeValue(nodeVal);
		goParent();
	}
}

void CXML2::setChildNodeValue(const char* name, float nodeVal, const char* attrname, const char* attrvalue)
{
	if (findChildNode(name, attrname, attrvalue)) {
		setNodeValue(nodeVal);
		goParent();
	}
}

const char* CXML2::getAttribute(const char* propname, const char* defaultValue, void* node)
{
	for (xmlAttrPtr attr = ((xmlNodePtr)(node ? node : cur))->properties; attr; attr = attr->next) {
		if (propname && !xmlStrcmp(attr->name, (const xmlChar*)propname)) {
			return attr->children ? (const char*)attr->children->content : defaultValue;
		}
	}
	return defaultValue;
}

int CXML2::getAttributeInt(const char* propname, int def, void* node)
{
	for (xmlAttrPtr attr = ((xmlNodePtr)(node ? node : cur))->properties; attr; attr = attr->next) {
		if (!xmlStrcmp(attr->name, (xmlChar*)propname)) {
			return attr->children && attr->children->content ? atoi((char*)attr->children->content) : def;
		}
	}
	return 0;
}

float CXML2::getAttributeFloat(const char* propname, float def, void* node)
{
	for (xmlAttrPtr attr = ((xmlNodePtr)(node ? node : cur))->properties; attr; attr = attr->next) {
		if (!xmlStrcmp(attr->name, (xmlChar*)propname)) {
			return attr->children && attr->children->content ? (float)atof((char*)attr->children->content) : def;
		}
	}
	return 0;
}

char* CXML2::getNodeName()
{
	if (!cur) return 0;
	return (char*)cur->name;
}

void CXML2::setNodeName(const char* nodename)
{
	//xmlNodeSetContent(cur, (const xmlChar*)value);
	if (cur) xmlNodeSetName(cur, (xmlChar*)nodename);
}

void CXML2::setNodeValue(const char* value, bool isCData)
{
	xmlNodeSetContent(cur, (const xmlChar*)value);
}

void CXML2::setNodeValue(int value)
{
	char buf[16] = {0};
	sprintf(buf, "%d", value);
	xmlNodeSetContent(cur, (const xmlChar*)buf);
}

void CXML2::setNodeValue(float value)
{
	char buf[16] = {0};
	sprintf(buf, "%1.4f", value);
	xmlNodeSetContent(cur, (const xmlChar*)buf);
}

void* CXML2::addChild(void* node)
{
	xmlNodePtr newNode = xmlCopyNode((xmlNodePtr)node, 1);
	newNode = xmlAddChild(cur, newNode);
	return newNode;
}

void* CXML2::appendChild(void* node)
{
	xmlAddChild(cur, (xmlNodePtr)node);
	return node;
}

void* CXML2::addChild(const char* nodename)
{
	xmlNodePtr node = xmlNewTextChild(cur, NULL, (const xmlChar*)nodename, 0);
	//xmlAddNextSibling((xmlNodePtr)cur, xmlNewText((const xmlChar*)"\n"));
	return node;
}

void* CXML2::addChild(const char* nodename, const char* value)
{
	xmlNodePtr node = xmlNewTextChild(cur, NULL, (const xmlChar*)nodename, (const xmlChar*)value);
	//xmlAddNextSibling(node, xmlNewText((const xmlChar*)"\n"));
	return node;
}

void* CXML2::addChild(const char* nodename, int value)
{
	char buf[16] = {0};
	sprintf(buf, "%d", value);
	xmlNodePtr node = xmlNewTextChild(cur, NULL, (const xmlChar*)nodename, (const xmlChar*)buf);
	//xmlAddNextSibling(node, xmlNewText((const xmlChar*)"\n"));
	return node;
}

void* CXML2::addChild(const char* nodename, float value)
{
	char buf[16] = {0};
	sprintf(buf, "%1.4f", value);
	xmlNodePtr node = xmlNewTextChild(cur, NULL, (const xmlChar*)nodename, (const xmlChar*)buf);
	//xmlAddNextSibling(node, xmlNewText((const xmlChar*)"\n"));
	return node;
}

void* CXML2::addText(int value)
{
	char text[32] = {0};
	sprintf(text, "%d", value);
	return xmlAddNextSibling(cur, xmlNewText((xmlChar*)text));
}

void* CXML2::addText(const char* text)
{
	return xmlAddNextSibling(cur, xmlNewText((xmlChar*)text));
}

void* CXML2::addAttribute(const char* name, const char* value)
{
	return xmlNewProp(cur, (xmlChar*)name, (xmlChar*)value);
}

void* CXML2::setAttribute(const char* name, const char* value)
{
	return xmlSetProp(cur, (xmlChar*)name, (xmlChar*)value);
}

void* CXML2::setAttribute(const char* name, int value)
{
	char buf[32] = {0};
	sprintf(buf, "%d", value);
	return xmlSetProp(cur, (xmlChar*)name, (xmlChar*)buf);
}

void* CXML2::setAttribute(const char* name, float value)
{
	char buf[32] = {0};
	sprintf(buf, "%f", value);
	return xmlSetProp(cur, (xmlChar*)name, (xmlChar*)buf);
}

void  CXML2::removeAttribute(const char* name)
{
	xmlAttrPtr aptr = xmlHasProp(cur, (xmlChar*)name);
	xmlRemoveProp(aptr);
}

void* CXML2::removeNode()
{
	xmlNodePtr node = cur;
	goParent();
	xmlUnlinkNode(node);
	xmlFreeNode(node);
	if (cur == node) cur = 0;
	return cur;
}

void* CXML2::unlinkNode()
{
	xmlNodePtr node = cur;
	goParent();
	xmlUnlinkNode(node);
	if (cur == node) cur = 0;
	return node;
}

void* CXML2::replaceNode(const void* rNode)
{
	xmlNodePtr newNode = xmlCopyNode((xmlNodePtr)rNode, 1);
	xmlNodePtr node = cur;
	goParent();	
	node = xmlReplaceNode(node, newNode);
	xmlFreeNode(node);	//??
	if (cur == node) cur = 0;
	return newNode;
}

void CXML2::Lock()
{
#ifdef WIN32
	if (!hLock)
		hLock = CreateMutex(0, TRUE, 0);
	else
		WaitForSingleObject(hLock, INFINITE);
#else
    pthread_mutex_lock(&mMutex);
#endif
}

void CXML2::Unlock()
{
#ifdef WIN32
	ReleaseMutex(hLock);
#else
    pthread_mutex_lock(&mMutex);
#endif
}
_MC_STRPRO_END
