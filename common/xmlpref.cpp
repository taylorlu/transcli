#include <stdio.h>
#include <fcntl.h>	
#include <sys/stat.h>
#include <list>

#include "MEvaluater.h"
#include "bit_osdep.h"

using namespace StrPro;
using namespace std;

CXMLPref::CXMLPref(const char* rootName)
{
	m_pXml = new StrPro::CXML2;
	if(rootName) {
		m_pXml->New("1.0", rootName);
	}
}

bool CXMLPref::CreateFromXmlFile(const char* fileName)
{
	if(m_pXml) {
		return (m_pXml->Open(fileName) == 0);
	}
	return false;
}


CXMLPref* CXMLPref::Clone()
{
	CXMLPref* pPref = new CXMLPref;
	char* xmlBuf = NULL;
	m_pXml->Dump(&xmlBuf);
	pPref->m_pXml = new StrPro::CXML2;
	pPref->m_pXml->Read(xmlBuf);
	m_pXml->FreeDump(&xmlBuf);
	return pPref;
}

CXMLPref::~CXMLPref()
{
	if(m_pXml) {
		delete m_pXml;
		m_pXml = NULL;
	}
}

int CXMLPref::GetInt(const char* key, const char* nodeName /*= NULL*/)
{
	const char* res = NULL;
	if (m_pXml->findNode(nodeName? nodeName : "node", "key", key)) {
		res =  m_pXml->getNodeValue();
		if(res) return atoi(res);
	} else if(m_pXml->goRoot() && m_pXml->findChildNode(nodeName? nodeName : "node", "key", key)) {
		res = m_pXml->getNodeValue();
		if (res) return atoi(res);		
	}		

	if(!res) {
		res = MEvaluater::GetDefaultNodeValueByKey(key);
		if(res) return atoi(res);
		fprintf(stderr, "No value in pref setting relate to: %s\n", key);
		return PREF_ERR_NO_INTNODE;
	}

	return PREF_ERR_NO_INTNODE;
}

void* CXMLPref::replaceNode(void* newNode)
{
	if(m_pXml) {
		return m_pXml->replaceNode(newNode);
	}
	return NULL;
}

void* CXMLPref::goRoot()
{
	if(m_pXml) {
		return m_pXml->goRoot();
	}
	return NULL;
}

int CXMLPref::Dump(char** buffer, char* encoding)
{
	if(m_pXml) {
		return m_pXml->Dump(buffer, encoding);
	}
	return -1;
}

void CXMLPref::FreeDump(char** buffer)
{
	if(m_pXml) {
		m_pXml->FreeDump(buffer);
	}
}

void* CXMLPref::setAttribute(const char* name, const char* value)
{
	if(m_pXml) {
		return m_pXml->setAttribute(name, value);
	}
	return NULL;
}

void CXMLPref::setNodeName(const char* nodename)
{
	if(m_pXml) {
		m_pXml->setNodeName(nodename);
	}
}

const char* CXMLPref::getAttribute(const char* attrName, const char* defaultValue, void* node)
{
	if(m_pXml) {
		return m_pXml->getAttribute(attrName, defaultValue, node);
	}
	return NULL;
}

const char* CXMLPref::getChildNodeValue(const char* name, const char* attrname, const char* attrvalue, int index)
{
	if(m_pXml) {
		return m_pXml->getChildNodeValue(name, attrname, attrvalue, index);
	}
	return NULL;
}

float CXMLPref::GetFloat(const char* key, const char* nodeName /*= NULL*/)
{
	const char* res = NULL;
	if (m_pXml->findNode(nodeName? nodeName : "node", "key", key)) {
		res =  m_pXml->getNodeValue();
		if(res) return (float)atof(res);
	} else if(m_pXml->goRoot() && m_pXml->findChildNode(nodeName? nodeName :"node", "key", key)) {
		res = m_pXml->getNodeValue();
		if (res) return (float)atof(res);		
	}		

	if(!res) {
		res = MEvaluater::GetDefaultNodeValueByKey(key);
		if (res) return (float)atof(res);				
		fprintf(stderr, "No value in pref setting relate to: %s\n", key);
		return PREF_ERR_NO_FLOATNODE;
	}
	return PREF_ERR_NO_FLOATNODE;
}

const char* CXMLPref::GetString(const char* key, const char* nodeName /* = NULL */)
{
	const char* res = NULL;
	const char* childNodeName = nodeName ? nodeName : "node";
	if (m_pXml->findNode(childNodeName, "key", key)) {
		res = m_pXml->getNodeValue();
		if (res) return res;	
	} else if(m_pXml->goRoot() && m_pXml->findChildNode(childNodeName, "key", key)) {
		res = m_pXml->getNodeValue();
		if (res) return res;
	}

	if(!res) {
		res = MEvaluater::GetDefaultNodeValueByKey(key);
		if(res) return res;
		fprintf(stderr, "No value in pref setting relate to: %s\n", key);
	}
	
	return "";
}

bool CXMLPref::GetBoolean(const char* key, const char* nodeName /* = NULL */)
{
	const char* res = NULL;
	if (m_pXml->findNode(nodeName? nodeName : "node", "key", key)) {
		res = m_pXml->getNodeValue();
		if (res) {
			return strcmp(res, "true") == 0 ? true : false;
		} 
	} if(m_pXml->goRoot() && m_pXml->findChildNode(nodeName? nodeName : "node", "key", key)) {
		res = m_pXml->getNodeValue();
		if (res) {
			return strcmp(res, "true") == 0 ? true : false;
		} 
	}

	if (!res) {
		res = MEvaluater::GetDefaultNodeValueByKey(key);
		if(res) {
			return strcmp(res, "true") == 0 ? true : false;
		}
		fprintf(stderr, "No value in pref setting relate to: %s\n", key);
		return false;
	}
	
	return false;
}

bool CXMLPref::SetBoolean(const char* key, bool value, const char* nodeName /* = NULL */)
{
	if (m_pXml->findNode(nodeName? nodeName : "node", "key", key)) {
		m_pXml->setAttribute("flag", 1);
		m_pXml->setNodeValue(value ? "true" : "false");
		return true;
	}
	if (m_pXml->goRoot() && m_pXml->findChildNode(nodeName? nodeName : "node", "key", key)) {
		m_pXml->setAttribute("flag", 1);
		m_pXml->setNodeValue(value ? "true" : "false");
		return true;
	}
	if(	m_pXml->goRoot())
	{
		void* node = m_pXml->addChild(nodeName? nodeName : "node", value ? "true" : "false");
		m_pXml->SetCurrentNode(node);
		m_pXml->addAttribute("key", key);
		m_pXml->setAttribute("flag", 1);
		return true;
	}
	
	return false;
}

bool  CXMLPref::SetInt(const char* key, int value, const char* nodeName /*= NULL*/)
{
	if (m_pXml->findNode(nodeName? nodeName : "node", "key", key)) {
		m_pXml->setAttribute("flag", 1);
		m_pXml->setNodeValue(value);
		return true;
	}
	if (m_pXml->goRoot() && m_pXml->findChildNode(nodeName? nodeName : "node", "key", key)) {
		m_pXml->setAttribute("flag", 1);
		m_pXml->setNodeValue(value);
		return true;
	}
	if(	m_pXml->goRoot())
	{
		void* node = m_pXml->addChild(nodeName? nodeName : "node", value);
		m_pXml->SetCurrentNode(node);
		m_pXml->addAttribute("key", key);
		m_pXml->setAttribute("flag", 1);
		return true;
	}
	
	return false;
}

bool  CXMLPref::SetFloat(const char* key, float value, const char* nodeName /*= NULL*/)
{
	if (m_pXml->findNode(nodeName? nodeName : "node", "key", key)) {
		m_pXml->setAttribute("flag", 1);
		m_pXml->setNodeValue(value);
		return true;
	}
	if (m_pXml->goRoot() && m_pXml->findChildNode(nodeName? nodeName : "node", "key", key)) {
		m_pXml->setAttribute("flag", 1);
		m_pXml->setNodeValue(value);
		return true;
	}
	if(	m_pXml->goRoot())
	{
		void* node = m_pXml->addChild(nodeName? nodeName : "node", value);
		m_pXml->SetCurrentNode(node);
		m_pXml->addAttribute("key", key);
		m_pXml->setAttribute("flag", 1);
		return true;
	}
	
	return false;
}

bool  CXMLPref::SetString(const char* key, const char* value, bool cdata, const char* nodeName /*= NULL*/)
{
	if (m_pXml->findNode(nodeName? nodeName : "node", "key", key)) {
		m_pXml->setAttribute("flag", 1);
		m_pXml->setNodeValue(value, cdata);		
		return true;
	}
	if (m_pXml->goRoot() && m_pXml->findChildNode(nodeName? nodeName : "node", "key", key)) {
		m_pXml->setAttribute("flag", 1);
		m_pXml->setNodeValue(value, cdata);		
		return true;
	}
	if(	m_pXml->goRoot())
	{
		void* node = m_pXml->addChild(nodeName? nodeName : "node");
		m_pXml->SetCurrentNode(node);
		m_pXml->setNodeValue(value, cdata);
		m_pXml->addAttribute("key", key);
		m_pXml->setAttribute("flag", 1);
		return true;
	}
	
	return false;
}

bool CXMLPref::isCurrentKeyValueDefault(const char* key)
{
	if (m_pXml->goRoot() && m_pXml->findChildNode("node", "key", key)) {
		int flag = m_pXml->getAttributeInt("flag");
		return flag == 0;
	}
	return false;
}

bool CXMLPref::ExistKey(const char* key)
{
	if (m_pXml->goRoot() && m_pXml->findChildNode("node", "key", key)) {
		return true;
	}
	return false;
}