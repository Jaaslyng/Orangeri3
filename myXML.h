
#ifndef myXML   // if x.h hasn't been included yet...

#define myXML  //   #define this so the compiler knows it has been included


//pugi::xml_document doc;
//std::string XMLName = "C:\\C_Lib\\pugixml-1.9\\docs\\samples\\orangeri.xml";

/*
Opretter en ny xml fil med de nødvendige data
*/

bool createXML();
/*
Søger efter en xpath i xml fil, fx "/Orangeri/StatusValues/LastWatering1" og returnerer værdien for noden

Der returneres en tom string hvis feltet ikke findes.
*/

std::string GetXMLValueString(std::string xpath);


double GetXMLValueDouble(std::string xpath, double defaultValue);

/*
Søger efter en xpath i xml fil, fx "/Orangeri/StatusValues/LastWatering1" og sætter en ny værdi for noden
value: string der rummer ny værdi
xpath: xpath til node fx. "/Orangeri/StatusValues/LastWatering1"

Eksempel: SetXMLValue("12.1","/Orangeri/StatusValues/WindowsPct");
*/

bool SetXMLValueString(std::string xpath, std::string value);
bool SetXMLValueDouble(std::string xpath, double value);

bool myopenXMLFile();
bool mycloseXMLFile();


#endif