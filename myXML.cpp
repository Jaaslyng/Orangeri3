/*
XML Håndtering til Orangeri projekt
*/

#include <stdio.h>
#include <string.h>
#include <iostream>
#include "pugixml.hpp"
#include <errno.h>
#include "myXML.h"

using namespace std;
pugi::xml_document doc;
//string XMLName = "C:\\C_Lib\\pugixml-1.9\\docs\\samples\\orangeri.xml";
string XMLName = "/home/jesper/Documents/Orangeri2/Orangeri/Orangeri.xml";

/*
Opretter en ny xml fil med de nødvendige data
*/

bool createXML()
{

	pugi::xml_node root = doc.append_child("Orangeri");

	root.append_child("StatusValues").append_attribute("Opdateret").set_value("xxx");

	root.last_child().append_child("LastWatering1").text().set("xxx");
	root.last_child().append_child("LastWatering2").text().set("xxx");
	root.last_child().append_child("LastWatering3").text().set("xxx");
	root.last_child().append_child("LastWatering4").text().set("xxx");
	root.last_child().append_child("WindowsPct").text().set("xxx");

	root.append_child("StatisticsValues").append_attribute("Opdateret").set_value("xxx");
	root.last_child().append_child("RainDayAccum").text().set("0");
	root.last_child().append_child("RainHourAccum").text().set("0");
	root.last_child().append_child("WaterUseCount").text().set("0");
	root.last_child().append_child("WaterUseAccum").text().set("0");

	root.append_child("RessourceUse").append_attribute("Opdateret").set_value("xxx");
	root.last_child().append_child("TimeWaterValve1").text().set("xxx");
	root.last_child().append_child("TimeWaterValve2").text().set("xxx");
	root.last_child().append_child("TimeWaterValve3").text().set("xxx");
	root.last_child().append_child("TimeWaterValve4").text().set("xxx");
	root.last_child().append_child("FreshWaterToTank").text().set("xxx");
	root.last_child().append_child("TimeHeatOn").text().set("xxx");
	root.last_child().append_child("LightGreenhouse").text().set("xxx");

	root.last_child().append_child("EnergyUse").text().set("xxx");

	doc.print(cout);
	bool saveSucceeded = doc.save_file(XMLName.c_str(), PUGIXML_TEXT("  "));
	return 1;
}


/*
Søger efter en xpath i xml fil, fx "/Orangeri/StatusValues/LastWatering1" og returnerer værdien for noden

Der returneres en tom string hvis feltet ikke findes.
*/

string GetXMLValueString(string xpath) {

	pugi::xml_document doc;
	string mytxt = "";
	cout << "XML fil " << XMLName.c_str() << endl;
	if (doc.load_file(XMLName.c_str()))
	{
		const char *cstr = xpath.c_str();
		pugi::xml_node mynode = doc.first_element_by_path(cstr);
		mytxt = mynode.text().get();
		if (mytxt == "")
		  {mytxt = "missing";}
	    cout << "cstr " << cstr << "  " << mytxt << endl;
	}
	return mytxt;
}

double GetXMLValueDouble(string xpath, double defaultValue) {
	char *endptr;
    //cout << "XML læsning af sti " << xpath << endl;
	string mystr = GetXMLValueString(xpath);
	//cout << "GetXMLValueDouble mystr :" << mystr << ":" << endl;
	double value = strtod(mystr.c_str(), &endptr);
	if (endptr == mystr.c_str())
	{
		// Not a valid number at all
		value = defaultValue;
		cout << "Fejl 1 ved XML læsning af " << xpath << endl;
	}
	else if (*endptr != '\0')
	{
		// String begins with a valid number, but also contains something else after the number
		value = defaultValue;
		cout << "Fejl 2 ved XML læsning af " << xpath << endl;
	}

	return value;
}


/*
Søger efter en xpath i xml fil, fx "/Orangeri/StatusValues/LastWatering1" og sætter en ny værdi for noden
value: string der rummer ny værdi
xpath: xpath til node fx. "/Orangeri/StatusValues/LastWatering1"

Eksempel: SetXMLValue("12.1","/Orangeri/StatusValues/WindowsPct");
*/

bool myopenXMLFile(){
	cout << "openXMLFile start" << endl;
	pugi::xml_document doc;
	bool saveSucceeded = false;
	cout << "XML filename: |" << XMLName.c_str() << "|" << endl;
	if (doc.load_file(XMLName.c_str()))
	{
		//const char *cstr1 = xpath.c_str();
		//const char *cstr2 = value.c_str();
		//pugi::xml_node mynode = doc.first_element_by_path(cstr1);
		//mynode.text().set(cstr2);
	   cout << "OpenXMLFile - file opend with succes" << endl;


	   saveSucceeded = true;
	} else
	{
      cout << "openXMLFile - Error - file not opend" << endl;
      createXML();
      saveSucceeded = false;
	}
	return saveSucceeded;
}

bool mycloseXMLFile(){
	cout << "closeXMLFile Start" << endl;
	cout << "Filename: " << XMLName.c_str() << endl;
	bool saveSucceeded = true;
	saveSucceeded = doc.save_file(XMLName.c_str(), PUGIXML_TEXT("  "));
	cout << "closeXMLFile result: " << saveSucceeded << endl;
	return saveSucceeded;
	}

bool SetXMLValueString(string xpath, string value) {

	//pugi::xml_document doc;
	string mytxt = "";
	bool saveSucceeded = false;
	//cout << "XML filename " << XMLName.c_str() << endl;
	//if (doc.load_file(XMLName.c_str()))
	//{
		const char *cstr1 = xpath.c_str();
		const char *cstr2 = value.c_str();
		pugi::xml_node mynode = doc.first_element_by_path(cstr1);
		mynode.text().set(cstr2);
		saveSucceeded = true;
	//}
	//saveSucceeded = doc.save_file(XMLName.c_str(), PUGIXML_TEXT("  "));
	return saveSucceeded;
}

bool SetXMLValueDouble(string xpath, double value) {
  string myStr;
  bool saveSucceeded;
  myStr = std::to_string(value);
  saveSucceeded = SetXMLValueString(xpath, myStr.c_str());
  cout << "XML Save " << saveSucceeded <<  "  " << xpath << " " << value << endl;
  return saveSucceeded;
}
