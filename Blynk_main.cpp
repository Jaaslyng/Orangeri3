#define BLYNK_TEMPLATE_ID "TMPL2pVmUmsE"
#define BLYNK_DEVICE_NAME "Orangeri"
#define BLYNK_PRINT stdout
//#define BLYNK_DEBUG
//#define BLYNK_DEBUG_ALL
//#define DebugWater   //Debug af vandning og friskvand
#define RASPBERRY
#define LINUX

//SQL units
#include "Database.h"

#include <BlynkApiWiringPi.h>
#include <BlynkSocket.h>
#include <BlynkOptionsParser.h>
#include <string>
//#include <bme680_jeaa.h>
#include <mariadb/conncpp.hpp>



static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);
#include <BlynkWidgets.h>

bool AfterOrangeriSetup = FALSE; //Sættes til Ture når setup er gennemført
bool FirstBlynkUpdate = TRUE;  //Sættes til FALSE efter første update. Bruges til at nulstille tabel m.m.

 Database db;

using namespace std;

BlynkTimer OrangeriTimer;
#include <iostream>
#include <Orangeri_lib.h>
//#include <pugixml.hpp>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <myXML.h>
#include <math.h>
#include <sht31-d.h>
#include <thread>
#include <chrono>
#include <sys/stat.h>
#include <sys/types.h>

//XMLNAme defineres 2 steder. Her og i myXML.cpp
char* XMLName = "/home/orangeri/Documents/Orangeri2/Orangeri/Orangeri.xml";

int BlynkError = 0; //Tæller der holder antal  fejl registreret
bool ForceBlynkUpdate = true; //Hvis den er true opdateres blynk værdier uanset om de e rændret eller ikke er
bool SetupFinished = false; //sættes til true efter at orangesetup er gennemført
int Weather_sht31file; //Filnummer svarende til SHT31
int delayBeforeIsp = 10; //delay der benyttes til at tjekke om det er støj eller en ændring der trigger interrupt
int blynkDelay = 0;  //d0elay der benyttes for at undgå at det uploades for mange værdier til blynk
//BME680Results  myBME680Result; //Struct with data

//holde styr på om udluftning sker efter extra temp sensor eller den almindelige
bool WinterTent = TRUE;

//--------------------Brug af indput/output pinne -------------------

//Liste over brug af digitale input/output

//******GPIO Porte********

/*
I2C:
GPIO 2
GPIO 3
Onewire:
GPIO 4

*/


//digitale input
#define Button_WaterPump_Pin 12
#define RAIN_GAUGE_PIN 17
#define Button_StopVents_Pin 20
#define Button_Disp_Pin 21
#define Water_Flow_Pin 27  //Benyttes denne???????
#define WaterLevelPin1 6  //Vandniveau måling for fyldt palletank   høj: fyldt lav: ikke fyldt
#define WaterLevelPin2 11 //Vandniveau måling for tom palletank    høj: tom   lav: ikke tom
#define WaterFlow1_PIN 26
#define WaterFlow2_PIN 19
#define WaterFlow3_PIN 16

//digitale output
//#define LED_WaterPumpButton_Pin 13
//#define LED_StopVentsButton_Pin 19
#define LedPin 22       //LED der viser at det kører
//#define LED_DispButton_Pin 26



//Expander Pi porte
#define Pi_Relay_Heat 1
#define Pi_Relay_WaterPump 2
#define Pi_Relay_LightGreenhouse 3
#define Pi_Relay_FreshWater 4  //Vandhanevand til tank#define Relay_LightPlants 5   //er ikkef orbundet elektrisk

#define Pi_Relay_WaterValve1 5 //COM 7 på relækort
#define Pi_Relay_WaterValve2 6 //COM 8 på relækort
#define Pi_Relay_WaterValve3 7 //COM 5 på relækort
#define Pi_Relay_WaterValve4 8 //COM 8 på relækort

#define Pi_Relay_LED_WaterPump 9
#define Pi_Relay_LED_StopVents 10
#define Pi_Relay_LED_Disp 11

#define Pi_Relay_Motor1_dir 12
#define Pi_Relay_Motor1_pwm 13


//Liste over brug af analoge input
#define LightPin 2
#define AirFlowPin 3
#define SoilMoistureSensorPin1 3
#define SoilMoistureSensorPin2 4

//Liste ovr brug af Virtuelle pins

#define RainAccumDay_Virtural_Pin V0
#define Weather_temperature_Virtural_Pin V1
#define Windows_Menu_Pin V2;   //er senere kaldt med V2 da det ikke virker at bruge det definerede navn. Mystisk!
#define Greenhouse_temperature_Virtural_Pin V3
#define Greenhouse_humidity_Virtural_Pin V4
#define Extra1_temperature_Virtural_Pin V5
#define Extra1_humidity_Virtural_Pin V6
#define Water_PumpOn_minutes_VP V7
#define ControlBoxTemperature_Virtual_Pin V8
#define TemperatureSetpointNow_Virtual_Pin V9
#define AutomaticControl_Virtual_Pin V10

#define Terminal_Virtual_Pin V11
// Attach virtual serial terminal to Virtual Pin V1
WidgetTerminal terminal(Terminal_Virtual_Pin);
#define LightGreenhouse_Virtual_Pin V12
#define RainAccumHour_Virtural_Pin V13
#define SidsteVandingLabel3_Virtual_Pin V14
//#define WaterValve1_Virtual_Pin V15
//#define WaterValve2_Virtual_Pin V16
#define TemperatureDay_Virtual_Pin V17
#define TemperatureNight_Virtual_Pin V18
#define TemperatureUdluftning_Virtual_Pin V19

//#define Window1_Virtual_Pin V20

//#define WaterValve3_Virtual_Pin V21
//#define WaterValve4_Virtual_Pin V22

#define SensorToControlwindow_Pin V24
#define ManualChangeWindow_Virtual_Pin V25
#define WindowPct_Virtual_Pin V26
#define AutoWindow_Virtual_Pin V27
#define WaterLevel1_Msg_Virtual_Pin V28
#define AirFlowValue_Virtual_Pin V29
#define SidsteVandingLabel4_Virtual_Pin V30

#define Extra2_temperature_Virtural_Pin V31
#define Extra2_humidity_Virtural_Pin V32
#define LastRunTimeStamp_Virtual_Pin V33
#define Table_Virtual_Pin V34
#define VandingEkstra_Menu_Pin V35

#define VandingStartTime1_Virtual_Pin V37
#define VandingStartTime2_Virtual_Pin V38
#define VandingsTime1_Virtual_Pin V39
#define VandingsTime2_Virtual_Pin V40
#define DayNight_Virtual_Pin V41
#define Vanding1Header_Virtual_Pin V42
#define Vanding2Header_Virtual_Pin V43
#define SidsteVandingLabel1_Virtual_Pin V44
#define SidsteVandingLabel2_Virtual_Pin V45

#define Vanding3Header_Virtual_Pin V47
#define AddWaterValveLabel_Virtual_Pin V48
#define Vanding4Header_Virtual_Pin V49
#define TemperatureUdluftningComfort_Virtual_Pin V50
#define ComfortDayPeriod_Virtual_Pin V51
#define LastWateringTimeLength_Virtual_Pin V52
#define TempAvgLastDay_Virtual_Pin V53  //Holder temepraturgennemsnit for dagen  før beregnet kl 24. Bruges til at øge vanding når denne værdi er høj
#define TempAvgDay_Virtual_Pin V54  //Holder temepraturgennemsnit for aktuel dag beregnet de kl 00 og frem#define TempAvgDay_Virtual_Pin V54  //Holder temepraturgennemsnit for aktuel dag beregnet de kl 00 og frem
//#define VandingEkstra4_Menu_Pin V55

#define LogEmptyNow_Virtual_Pin V57
//#define P_HeatUp_Virtual_Pin V58
//#define I_HeatUp_Virtual_Pin V59

#define LastSetupRunTimeStamp_Virtual_Pin V61 //Viser sidste tidspunkt for kørsel af setup rotine, altså upload eller WDT reset
#define CPUTemperature_Virtual_Pin V62
//#define WaterPump_Virtual_LED V63
//#define WaterTap_Virtual_LED V64
#define RainTable_Virtual_Pin V65
#define Rain3h_Virtual_Pin V66
#define Rain6h_Virtual_Pin V67
#define ControlBoxPressure_Virtual_Pin V70
//#define WaterPumpButtonActive_Virtual_Pin V71
//#define StopVentsButtonActive_Virtual_Pin V72
//#define DispButtonActive_Virtual_Pin V73
//#define test_Pin V74
#define StopVents_minutes_Virtual_Pin V75

#define VandingsInterval3_Virtual_Pin V77
#define VandingsInterval4_Virtual_Pin V78
#define VandingsTime3_Virtual_Pin V79
#define VandingsTime4_Virtual_Pin V80


double UdluftningsTemperatur_Aften = 20;
double UdluftningsTemperatur = 20;
double OpvarmningsTemperatur_Dag = 20;
double OpvarmningsTemperatur_Nat = 10;
int Temperatur_Styring = 1;
int VandingsKontrol = 1;
string Vanding1_Start = "09:00";
string Vanding2_Start = "17:00";
int VandingsTid_Ventil1 = 5;
int VandingsTid_Ventil2 = 5;
int VandingsTid_Ventil3 = 5;
int VandingsTid_Ventil4 = 5;
int VinduesKontrol = 5;
int VinduesStyring = 1;
int Aarstid = 2;
string DagStart = "07:00";
string NatStart = "22:00";
string KonfortPeriodeStart = "17:00";
string KonfortPeriodeSlut = "22:00";
int TvangslukkedeVinduer_Minutter = 30;


void ReadSetpoints()
{
   string sqlError = "";

   string Parameter = "UdluftningTemperatur_Aften";
   if (!db.Get_Setpoint(Parameter, UdluftningsTemperatur_Aften, 20, sqlError))
           {out << "Setpoint read error for parameter: " << Parameter << endl;}

   Parameter = "UdluftningTemperatur";
   if (!db.Get_Setpoint(Parameter, UdluftningsTemperatur, 20, sqlError))
           {out << "Setpoint read error for parameter: " << Parameter << endl;}


}




void mem_usage(double& vm_usage, double& resident_set)
{
    vm_usage = 0.0;
    resident_set = 0.0;
    ifstream stat_stream("/proc/self/stat",ios_base::in);
    //create some variables to get info
    string pid, comm, state, ppid, pgrp, session, tty_nr;
    string tpgid, flags, minflt, cminflt, majflt, cmajflt;
    string utime, stime, cutime, cstime, priority, nice;
    string O, itrealvalue, starttime;
    unsigned long vsize;
    long rss;
    stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
                >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
                >> utime >> stime >> cutime >> cstime >> priority >> nice
                >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest
    stat_stream.close();
    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // for x86-64 is configured to use 2MB pages
    vm_usage = vsize / 1024.0;
    resident_set = rss * page_size_kb;
}
//-------------------OrangeriLog----------------------------------

void myDelay(int delay)
{
    int test;
    test = 0;
    int timestamp = time(NULL);
    //cout << "myDelay Start " << timestamp << endl;

    while (time(NULL) - timestamp < delay)
    {
        test++;
        Blynk.run();
    }
    //cout << "myDelay Slut" << time(NULL) << endl;

}


string showDate(time_t TimeInput)
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    //strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
    //strftime(date, sizeof(date), "%H%M%S", tm);

    return std::ctime(&t_c);
}


//***********************************************************************
//Tæller der sikre at der ikke kommer for mange data til Blynk pr sekund
//***********************************************************************
int BlynkUpdateCounter = 0; //Counter der holder styr på blynk opdateringer, så man undgår for mange data på en gang
int BlynkUpdateCounterDay = 0; //Counter der holder styr på blynk opdateringer, så man undgår for mange data på en gang
int UpdateWeekDay;
int BlynkUpdateTime = 0;
const int MaxUpdatePerSecond = 20;

bool UpdateBlynkOk()
{
    return true;
    /*
    cout << "UpdateBlynkOk" << BlynkUpdateTime << "  "  << time(NULL) << endl;

    if (BlynkUpdateTime != time(NULL))
    {
      BlynkUpdateCounter = 0;
      BlynkUpdateTime = time(NULL);
    };

    // Get the current time
    std::time_t t = std::time(nullptr);
    // Convert to local time
    std::tm* localTime = std::localtime(&t);
    // Get the day of the week (0 = Sunday, 1 = Monday, ..., 6 = Saturday)
    int weekday = localTime->tm_wday;
    if (weekday != UpdateWeekDay)
    {
        BlynkUpdateCounterDay = 0;
        UpdateWeekDay = weekday;
    }

    //Tjek for om der er for mange daglige Blynk opdateringer
    BlynkUpdateCounterDay++;

    //Tjek for om der er for mange opdataeringer pr sekund
    if (BlynkUpdateCounter < MaxUpdatePerSecond )
    {
        BlynkUpdateCounter++;
        cout << "Blynk opdateres" << endl;
        return true;
    }
    else
    {
        return false;
        cout << "Blynk data ikke opdateret for at undgå afbrydelse af forbindelse" << endl;
    }

*/
}


string OrangeriLogName   = "/home/jesper1/Dokumenter/Orangeri/Log/Orangeri.txt";

ofstream Log;
int enableLog; //Angiver om der skrives data til fejlfindings log
int emptyLogNow; //Tømmer log hvis den er lig 1

void EmptyLog()
{
    cout << "Log tømmes" << endl;
    Log.open(OrangeriLogName,std::ofstream::out | std::ofstream::trunc);

}

void Write2Log(string msg)
{
    cout << "Write2log " << msg << endl;
    if (enableLog == 1)
    {
        Log.open(OrangeriLogName,ios::app) ;
        Log << showDate(time(NULL)) << " " << msg << endl;
        Log.close();
    }
}

// Checker for internet setup og forbindelse
//   Returnerer 1 hvis der er forbindelse

bool CheckInternetConnection(void)
{

    int i;
    FILE *output;
    i = 0;
    if(!(output = popen("/sbin/route -n | grep -c '^0\\.0\\.0\\.0'","r")))
    {
        i = -1;   //kan ikke åbne forbindelse til nettet
    }

    if (i >= 0)
    {
        fscanf(output,"%u",&i);
        if(i == 1)
            cout << "There is internet connection\n";
        else if(i == 0)
            cout << "There is no internet connection\n";
        pclose(output);
    }
    return i;
}


time_t getFileCreationTime(char *path)
{
    time_t FileTime;
    struct stat attr;
    stat(path, &attr);
    printf("XML last modified time: %s", ctime(&attr.st_mtime));
    FileTime = attr.st_mtime;
    return FileTime;
}


int getOpenFileCount(void)
{



    int j, n = 0;


// count open file descriptors
    for (j = 0;  j < FOPEN_MAX;  ++j)     // FDMAX should be retrieved from process limits,
        // but a constant value of >=4K should be
        // adequate for most systems
    {
        int fd = dup (j);
        if (fd < 0)
            continue;
        ++n;
        close (fd);
    }

    return n;

}

/*
extern int errno;

void CountOpenFiles() {
int i = 0;
int fd_counter = 0;
int max_fd_number = 0;
struct stat stats;
struct rlimit rlimits;
max_fd_number = getdtablesize();
getrlimit(RLIMIT_NOFILE, &rlimits);
printf( "max_fd_number: %d\n", max_fd_number );
printf( " rlim_cur: %d\n", rlimits.rlim_cur );
printf( " rlim_max: %d\n", rlimits.rlim_max );
for ( i = 0; i <= max_fd_number; i++ ) {
	fstat(i, &stats);
    if ( errno != EBADF ) { fd_counter++; } }
	printf( " open files: %d\n", fd_counter );
return 0; }*/



//==================Expander Pi Start ================================
#include "ABE_ExpanderPi.h"
using namespace ABElectronics_CPP_Libraries;
ExpanderPi ABE_expi;

// Holder antal fejl regisreret ved henholdsvis opsætning, læsning og
// skrivning til IOPort på ExpanderPi
int IOPortSetupError = 0;
int IOPortReadError = 0;
int IOPortWriteError = 0;
int SetADCError = 0;
int ReadADCError = 0;


void ExpanderPiSetup(void)
{

//Opsætning af ExpanderPi kortet
    //All prote sættes til Output
    //ExPi er defineret til kald af kortet
    // 0x00 sætter 8 porte til output
    // 0xFF sætter 8 porte til input
    if (ABE_expi.adc_open() != 1)
    {
        printf("ADC Expander Pi kan ikke åbnes");
    }
    ABE_expi.io_set_port_direction(0, 0x00); // set bank 0 to be outputs
    ABE_expi.io_set_port_direction(1, 0x00); // set bank 1 to be outputs
    ABE_expi.io_set_port_pullups(0, 0xFF);   // enable internal pullups for bank 0
    ABE_expi.io_invert_port(0, 0xFF);		  // invert output so bank will read as 0
    ABE_expi.io_set_port_pullups(1, 0xFF);   // enable internal pullups for bank 0
    ABE_expi.io_invert_port(1, 0xFF);		  // invert output so bank will read as 0


}


//Læser DAC værdi fra Expandr Pi
double ExPi_ReadADC(int ADCChn)
{
    double ADCValue;
    try
    {


        if (ABE_expi.adc_open() != 1)
        {
            cout << "ADC Expander Pi kan ikke åbnes" << endl;
        }
        ADCValue = ABE_expi.adc_read_voltage(ADCChn, 0);
        ABE_expi.dac_close();
    }


    catch (const std::out_of_range& oor)
    {
        cout << "ReadADC - Out of Range error: " << oor.what() << endl;
        ReadADCError++;
    }

    catch (const std::runtime_error& oor)
    {
        cout << "ReadADC -  Out of Range error: " << oor.what() << endl;
        ReadADCError++;
    }

    catch (...)
    {
        cout << "ReadADC - Fejl ved ReadADC " << endl;
        ReadADCError++;
    }

    return ADCValue;
}

void ExPi_SetADC(int ADCChan, double Voltage)
{
    try
    {
        if (ABE_expi.dac_open() == 1)
        {
            // open the DAC spi channel
            ABE_expi.dac_set_voltage(Voltage, ADCChan, 2); // set the voltage on channel 1 to 1.2V and the gain to 2
        }
        ABE_expi.dac_close();
    }

    catch (const std::out_of_range& oor)
    {
        cout << "SetADC - Out of Range error: " << oor.what() << endl;
        SetADCError++;
    }

    catch (const std::runtime_error& oor)
    {
        cout << "SetADC -  Out of Range error: " << oor.what() << endl;
        SetADCError++;
    }

    catch (...)
    {
        cout << "SetADC - Fejl ved ReadADC " << endl;
        SetADCError++;
    }

}

/* initialise one of the io pi buses on i2c address  default address for bus 1
 * Der er 2 sæt porte, kaldet bank 1 og bank 2
 * Sidste bit (mindst værdi) angiver den første port, næstsidste den anden port osv
 * 00001111 angiver at port 1-4 skal være input og 5-8 output
 *
 * 0 angiver output
 * 1 angiver input
 * 0x00 sætte alle 8 til værdien 0
 * 0xFF sætter alle 8 til værdien 1
 */

void ExPi_IOPortsSetup(char Bank1, char Bank2)
{
    try
    {
        ABE_expi.io_set_port_direction(0, Bank1); // set bank 0 to be inputs
        ABE_expi.io_set_port_direction(1, Bank1); // set bank 1 to be inputs
        ABE_expi.io_set_port_pullups(0, 0xFF);   // enable internal pullups for bank 0
        ABE_expi.io_invert_port(0, 0xFF);		  // invert output so bank will read as 0
        ABE_expi.io_set_port_pullups(1, 0xFF);   // enable internal pullups for bank 0
        ABE_expi.io_invert_port(1, 0xFF);		  // invert output so bank will read as 0
        ABE_expi.io_write_port(0, 0x00);
        ABE_expi.io_write_port(1, 0x00);
    }

    catch (exception &e)
    {
        //cout << e.what();
        cout << "***** IOPortSetup " << e.what() << endl;
        IOPortSetupError++;
    }

    /*
     * catch (const std::out_of_range& oor) {
         cout << "IOPortSetup - Out of Range error: " << oor.what() << endl;
       IOPortSetupError++;}

    catch (const std::runtime_error& oor) {
         cout << "IOPortSetup -  Out of Range error: " << oor.what() << endl;
       IOPortSetupError++;}

    catch (...){
       cout << "IOPortSetup - Fejl ved ExPi_IOPortSetup " << endl;
         IOPortSetupError++;}
    */

}


void ExPi_IOPortWrite(int PortNbr, int PortState)
{
    try
    {
        /*Relay Module fra Kiwi Eælectronic virker modsat.
          Trækkes når signalet er 0. Derfor vendes signalet her
          Det betyder at ExPi_IOPortWrite kaldes med PortState = 1
          for at trække relæet og 0 for ikke at række det.
          Relæet er forbundet til port 4 til og med 11 på expanderPi
        */
        if (PortNbr >= 4 & PortNbr <= 11)
        {
            if (PortState == 1)
            {
                PortState = 0;
            }
            else
            {
                PortState = 1;
            }
        }
        ABE_expi.io_write_pin(PortNbr, PortState);

        //cout << "ExPi_IOPortWrite Port: " << PortNbr << " " << PortState << endl;
    }
    catch (exception &e)
    {
        //cout << e.what();
        cout << "***** IOPortWrite " << " Port " << PortNbr << " PortState " << PortState << " " << e.what() << endl;
        IOPortWriteError++;
    }

    catch(...)
    {
        cout << "***** IOPortWrite Uspecificeret " << endl;
    }

    /*
    catch (const std::out_of_range& oor) {
       cout << "***** IOPortWrite - Out of Range error: " << " Port " << PortNbr << " PortState " << PortState << " " << oor.what() << endl;
       IOPortWriteError++;}

    catch (const std::runtime_error& oor) {
       cout << "***** IOPortWrite -  Runtime error: " << "Port " << PortNbr << " PortState " << PortState << " " << oor.what() << endl;
       IOPortWriteError++;
        }

    catch (...){
    cout << "***** IOPortWrite - Fejl ved ExPi_IOPortWrite "  << "Port " << PortNbr << " PortState " << PortState << endl;
    IOPortWriteError++;}
    */

}



int ExPi_IOPortRead(int PortNbr)
{
    int PortResult;

    try
    {
        PortResult = ABE_expi.io_read_pin(PortNbr);
    }

    catch (exception &e)
    {
        //cout << e.what();
        cout << "***** IOPortRead " << " Port " << PortNbr << " " << e.what() << endl;
        IOPortReadError++;
    }

    /*
     * catch (const std::out_of_range& oor) {
         cout << "**** IOPortRead - Out of Range error: " << "  PortNbr " << PortNbr << " " << oor.what() << endl;
       IOPortReadError++;}

    catch (const std::runtime_error& oor) {
         cout << "'''' IOPortRead -  Runtime error: " << " PortNbr " << PortNbr << " " << oor.what() << endl;
       IOPortReadError++;}

    catch (...){
       cout << "**** IOPortRead - Fejl ved ExPi_IOPortRead " << " PortNbr " << PortNbr << " " << endl;
       IOPortReadError++;}
    */

    return PortResult;
}

//==================Expander Pi Slut ================================

// -------------------------------------------------------------------------
//Opsætning af vandflow måleree. Typen YF-B5
// -------------------------------------------------------------------------

volatile int WaterFlow1EventCounter = 0;
float WaterFlow1;   //Holder flow i en periode
float WaterTotalFlow1 = 0; //Holder flow siden program start

void WaterFlow1Interrupt(void)  // myInterrupt:  called every time an event occurs
{
    WaterFlow1EventCounter++;
    //cout << "WaterFlow1Interrupt " << WaterFlow1EventCounter << endl;
}

void GetWaterFlow1()
{
    //Vandflow
    WaterFlow1 = WaterFlow1EventCounter/6.6; //L/min
    WaterFlow1EventCounter = 0;
    WaterTotalFlow1 = WaterTotalFlow1 + WaterFlow1;
    //cout << "WaterFlow1 " << WaterFlow1 << "Total: " << WaterTotalFlow1 << endl;
}



volatile int WaterFlow2EventCounter = 0;
float WaterFlow2;   //Holder flow i en periode
float WaterTotalFlow2 = 0; //Holder flow siden program start

void WaterFlow2Interrupt(void)  // myInterrupt:  called every time an event occurs
{
    WaterFlow2EventCounter++;
    //cout << "WaterFlow2Interrupt " << WaterFlow2EventCounter << endl;
}

void GetWaterFlow2()
{
    //Vandflow
    WaterFlow2 = WaterFlow2EventCounter/6.6; //L/min
    WaterFlow2EventCounter = 0;
    WaterTotalFlow2 = WaterTotalFlow2 + WaterFlow2;
    //cout << "WaterFlow2 " << WaterFlow2 << "Total: " << WaterTotalFlow2 << endl;
}

volatile int WaterFlow3EventCounter = 0;
float WaterFlow3;   //Holder flow i en periode
float WaterTotalFlow3 = 0; //Holder flow siden program start

void WaterFlow3Interrupt(void)  // myInterrupt:  called every time an event occurs
{
    WaterFlow3EventCounter++;
    //cout << "WaterFlow3Interrupt " << WaterFlow3EventCounter << endl;
}

void GetWaterFlow3()
{
    //Vandflow
    WaterFlow3 = WaterFlow3EventCounter/6.6; //L/min
    WaterFlow3EventCounter = 0;
    WaterTotalFlow3 = WaterTotalFlow3 + WaterFlow3;
    //cout << "WaterFlow3 " << WaterFlow3 << "Total: " << WaterTotalFlow3 << endl;
}





double getCPUTemperature()
{
    string val;
    string preparedTemp;
    double temperature;
    ifstream temperatureFile ("/sys/class/thermal/thermal_zone0/temp");
    /* if (temperatureFile < 0) {
         cout << "Failed to open temperatureFile!\n";
         return -1;
     }
    */
    // The temperature is stored in 5 digits.  The first two are degrees in C.  The rest are decimal precision.
    temperatureFile >> val;

    temperatureFile.close();

    preparedTemp = val.insert(2, 1, '.');
    temperature = std::stod(preparedTemp);

    return temperature;
}


//==================Interrupt rutiner start ======================
//LED og knapper der er tilgængelige på den grå boks
//LED viser at funktion er aktiv uanset om det skyldes tryk på knappen eller en automatisk bestemt tilstand

//-------------------- Disp knap der endnu ikke benyttes til en spcifik funktion -----------------------------
bool DispButtonState;
time_t DispButtonActive;
int DispButton_isp_Now;
int DispButton_isp_Last;
int DispButton_isp_Pin;
int DispButton_minutes;
//WidgetLED led_DispButton(DispButtonActive_Virtual_Pin);

void ControlDispButton(time_t myTime)
{
    if (myTime > 0)   //Pumpe og LED tændes
    {
        //digitalWrite(LED_DispButton_Pin, HIGH);
        ExPi_IOPortWrite(Pi_Relay_LED_Disp, true);
        //led_DispButton.on();
        //Blynk.virtualWrite(DispButtonActive_Virtual_Pin, 1);
    }
    else
    {
        //led_DispButton.off();
        //Blynk.virtualWrite(DispButtonActive_Virtual_Pin, 0);
        //digitalWrite(LED_DispButton_Pin, LOW);
        ExPi_IOPortWrite(Pi_Relay_LED_Disp, false);
    }
}

void DispButton_isp_Interrupt()
{
    string msg5;
    if (AfterOrangeriSetup == FALSE)
    {
        return;
    }
    //std::this_thread::sleep_for((std::chrono::milliseconds(5)));
    delay(delayBeforeIsp);
    DispButtonState = digitalRead(Button_Disp_Pin);
    if (DispButtonState == 1)
    {
        return;
    }

    DispButton_isp_Now = time(NULL);
    if (DispButton_isp_Now-DispButton_isp_Last > 1 and SetupFinished == TRUE)
    {
        //delay(40);
        cout << "!!!! Disp Interrupt !!!" << endl;

        DispButtonState = 1; //angiver at der er trykket på knappen

        cout << "DispButtonActive før" << DispButtonActive << endl;
        DispButton_isp_Last = time(NULL);

        //Checker status på den fysiske knap og sammenholder med om WaterPump er aktiv
        if (DispButtonActive > 0 )
        {
            DispButtonActive = 0;
            DispButtonState = 0;
            cout << "Funktion for Disp knap deaktiveret" << endl;
        } //Nulstiller knap så den ikke bruges igen til at ændre status
        else
        {
            DispButtonActive = time(NULL)+ DispButton_minutes*60;
            cout << "Disp funktion aktiveres " << endl;
            cout << "Disp funktion stoppes i minutter " << DispButton_minutes << endl; //msg5 = showDate(WaterPumpButtonActive);
            cout << "Tiden nu er " << time(NULL);
            cout << "DispButtonActive indtil" << DispButtonActive << endl;
        }  //WaterPump sættes aktiv og tiden sættes til nu
        ControlDispButton(DispButtonActive);   //slut første if
    }
}

void DispButton_isp_Setup(int Pin1)
{
    int err;
    DispButtonActive = 0;
    //digitalWrite(LED_DispButton_Pin, LOW);
    ExPi_IOPortWrite(Pi_Relay_LED_Disp, false);
    DispButton_isp_Pin = Pin1;

    pinMode (Pin1, INPUT);
    pullUpDnControl (Pin1, PUD_UP);
    // set Pin 17/0 generate an interrupt on high-to-low transitions
    // and attach myInterrupt() to the interrupt
    err = wiringPiISR (Pin1, INT_EDGE_FALLING, &DispButton_isp_Interrupt);
    DispButtonActive = 0;
    DispButton_isp_Last=0;
    DispButton_minutes = 60;

}

//-------------------- Ingen styring af vinduer knap -----------------------------

bool StopVentsButtonState;
time_t StopVentsButtonActive;
int StopVentsButton_minutes;
int StopVentsButton_isp_Now;
int StopVentsButton_isp_Last;
int StopVentsButton_isp_Pin;
//WidgetLED led_StopVentsButton(StopVentsButtonActive_Virtual_Pin);

void ControlStopVentsButton(time_t myTime)
{

    cout << "ContolStopVentsButton "  << myTime << endl;

    if (myTime > 0)   //Pumpe og LED tændes
    {
        //digitalWrite(LED_StopVentsButton_Pin, HIGH);
        ExPi_IOPortWrite(Pi_Relay_LED_StopVents, true);
        //Blynk.virtualWrite(StopVentButtonActive_Virtual_Pin,"1");
    }
    else
    {
        //digitalWrite(LED_StopVentsButton_Pin, LOW);
        ExPi_IOPortWrite(Pi_Relay_LED_StopVents, false);
        //Blynk.virtualWrite(StopVentButtonActive_Virtual_Pin,"0");

    }
}

void StopVentsButton_isp_Interrupt()
{
    string msg5;

    if (AfterOrangeriSetup == FALSE)
    {
        return;
    }
    //Venter og tjekekr igen om det er korrekt at der er trykket
    //std::this_thread::sleep_for((std::chrono::milliseconds(5)));
    delay(delayBeforeIsp);

    StopVentsButtonState = digitalRead(Button_StopVents_Pin);
    if (StopVentsButtonState == 1)
    {
        return;
    }
    cout << "StopVentsButton_isp_Interrupt"  << endl;
    StopVentsButton_isp_Now = time(NULL);

    if (StopVentsButton_isp_Now-StopVentsButton_isp_Last > 1)
    {
        cout << "!!!! Stop Vents Interrupt !!!" << endl;

        StopVentsButtonState = 1; //angiver at der er trykket på knappen
        StopVentsButton_isp_Last = time(NULL);

        cout << "StopVentsButtonActive før" << StopVentsButtonActive << endl;

        if (StopVentsButtonActive > 0)
        {
            StopVentsButtonActive = 0;
            StopVentsButtonState = 0;
            cout << "Vinduer bevæger sig igen " << endl;
        } //Nulstiller knap så den ikke bruges igen til at ændre status
        else
        {
            StopVentsButtonActive = time(NULL)+ StopVentsButton_minutes*60;
            cout << "Vinduer bevægelser stoppes " << endl;
            cout << "Bevægelse stoppes i minutter " << StopVentsButton_minutes << endl; //msg5 = showDate(WaterPumpButtonActive);
            cout << "Tiden nu er " << time(NULL);
            cout << "StopVentsButtonActive indtil" << StopVentsButtonActive << endl;
        }  //WaterPump sættes aktiv og tiden sættes til nu

        //Tænder LED hvis vandpumpen er sat til at kører manuelt
        ControlStopVentsButton(StopVentsButtonActive);

    }  //slut første if

}

void StopVentsButton_isp_Setup(int Pin1)
{
    int err;
    StopVentsButtonActive = 0;
    //digitalWrite(LED_StopVentsButton_Pin, LOW);

    StopVentsButton_isp_Pin = Pin1;

    pinMode (Pin1, INPUT) ;
    pullUpDnControl (Pin1, PUD_UP) ;
    // set Pin 17/0 generate an interrupt on high-to-low transitions
    // and attach myInterrupt() to the interrupt
    err = wiringPiISR (Pin1, INT_EDGE_FALLING, &StopVentsButton_isp_Interrupt);

    StopVentsButtonActive = 0;
    StopVentsButton_isp_Last = 0;
    StopVentsButton_minutes = 60;
}


//-------------------- Vandings knap -----------------------------



//Status på om de fysiske knapper er trykket ned
bool WaterPumpButtonState;
time_t WaterPumpButtonActive;
//int last_waterbutton_pin_status;
//int waterbutton_pin_status; //husker status for rain_pin
int waterbutton_PumpOn_minutes;
int waterbutton_isp_Now;
int waterbutton_isp_Last;
int waterbutton_isp_Pin;
//WidgetLED led_WaterPumpButton(WaterPumpButtonActive_Virtual_Pin);

int Status_WaterPump = 0;
int CurretStatus_WaterPump = 0; //Holder styr på den aktuelt valgte status for vandpumpen

void SetWaterPump(int Status_WaterPump)
{
    //delay(1);
#if defined(WaterPump)
    cout << "SetWaterPump "  << Status_WaterPump << " " << CurretStatus_WaterPump << endl;
#endif // defined
    if (Status_WaterPump !=	CurretStatus_WaterPump)
    {
        CurretStatus_WaterPump = Status_WaterPump;
        if (Status_WaterPump == 1)
        {
            //digitalWrite(LED_WaterPumpButton_Pin, HIGH);
#if defined(WaterPump)
            cout << "WaterPumpOn" << endl;
#endif // defined

            ExPi_IOPortWrite(Pi_Relay_LED_WaterPump, true);
            ExPi_IOPortWrite(Pi_Relay_WaterPump, true);
            Blynk.logEvent("WaterPumpOn");
            //Blynk.virtualWrite(WaterPumpButtonActive_Virtual_Pin,"1");
        }
        else
        {
            //digitalWrite(LED_WaterPumpButton_Pin, LOW);
            ExPi_IOPortWrite(Pi_Relay_LED_WaterPump,false);
            ExPi_IOPortWrite(Pi_Relay_WaterPump, false);
#if defined(WaterPump)
            cout << "WaterPumpOff" << endl;
#endif
            //Blynk.virtualWrite(WaterPumpButtonActive_Virtual_Pin,"0");
            Blynk.logEvent("WaterPumpOff");
        }
    }

}

//Tænder og slukker for diverse relæer i henhold til om der pumpes vand pga tryk på knappen
void myControlWaterPumpButton(time_t myTime)
{
//Tænder LED hvis vandpumpen er sat til at kører manuelt
    cout << "myControlWaterPumpButton"  << endl;
    if (myTime > 0)   //Pumpe og LED tændes
    {
        Status_WaterPump = 1;
        SetWaterPump(Status_WaterPump);
        //digitalWrite(LED_WaterPumpButton_Pin, HIGH);
        ExPi_IOPortWrite(Pi_Relay_LED_WaterPump, true);
        ExPi_IOPortWrite(Pi_Relay_WaterPump, true);
        cout << "WaterPumpOn1" << endl;
    }
    else
    {
        //cout << "Pumpe slukket" << endl;
        Status_WaterPump = 0;
        SetWaterPump(Status_WaterPump);
        //digitalWrite(LED_WaterPumpButton_Pin, LOW);
        ExPi_IOPortWrite(Pi_Relay_LED_WaterPump, false);
        ExPi_IOPortWrite(Pi_Relay_WaterPump, false);
        cout << "WaterPumpOff1" << endl;
        //SetWaterValve3(FALSE);
    }
}

//Udfører funktion enten på baggrund af interrupt tryk eller Blynk tryk
void waterbutton_click()
{


}



void waterbutton_isp_Interrupt()
{
    string msg5;

    if (AfterOrangeriSetup == FALSE)
    {
        return;
    }

    //std::this_thread::sleep_for((std::chrono::milliseconds(5)));
    delay(delayBeforeIsp); //venter 3 ms og tjekekr igen on knappen er trykket ind ellers var det et falskt interrupt

    WaterPumpButtonState =  digitalRead(Button_WaterPump_Pin);
    if (WaterPumpButtonState == 1)
    {
        return;
    }

    waterbutton_isp_Now = time(NULL);
    cout << "Interrupt" << waterbutton_isp_Now << " " << waterbutton_isp_Last << endl;
    if (waterbutton_isp_Now-waterbutton_isp_Last > 1)
    {

        cout << "!!!! Water Interrupt !!!WF" << endl;
        WaterPumpButtonState = 1; //angiver at der er trykket på knappen
        waterbutton_isp_Last = time(NULL);
        cout << "WaterPumpButtonActive før" << WaterPumpButtonActive << endl;
        //Checker status på den fysiske knap og sammenholder med om WaterPump er aktiv
        if (WaterPumpButtonActive > 0) //and waterbutton_isp_Now-waterbutton_isp_Last > 5)
        {
            WaterPumpButtonActive = 0;
            WaterPumpButtonState = 0;
            cout << "Pumpe stoppes " << endl;
        } //Nulstiller knap så den ikke bruges igen til at ændre status
        else
        {
            WaterPumpButtonActive = time(NULL)+ waterbutton_PumpOn_minutes*60;
            cout << "Pumpe startes " << endl;
            cout << "waterbutton_PumpOn_minutes " << waterbutton_PumpOn_minutes << endl; //msg5 = showDate(WaterPumpButtonActive);
            cout << "Tiden nu er " << time(NULL) << endl;
            cout << "WaterPumpButtonActive indtil " << WaterPumpButtonActive << endl;  //WaterPump sættes aktiv og tiden sættes til nu
        }

        cout << "Inden ControlWaterPumpButton " << endl;
        //jeaa fejl der skal forhindre at deen startets ved en fejl
        WaterPumpButtonState = 0;
        myControlWaterPumpButton(WaterPumpButtonActive);
    }
}

void waterbutton_isp_Setup(int Pin1)
{
    int err;
    WaterPumpButtonActive = 0;
    //digitalWrite(LED_WaterPumpButton_Pin, HIGH);
    waterbutton_isp_Pin = Pin1;
    pinMode (Pin1, INPUT) ;
    pullUpDnControl (Pin1, PUD_UP) ;
    // set Pin 17/0 generate an interrupt on high-to-low transitions
    // and attach myInterrupt() to the interrupt
    err = wiringPiISR (Pin1, INT_EDGE_FALLING, &waterbutton_isp_Interrupt);
    waterbutton_isp_Last = 0;
    WaterPumpButtonActive = 0;
    waterbutton_PumpOn_minutes = 5;
}



//--------------------Argent Rain sensor -------------------------

//Rain variable
int last_rain_pin_status, rain_pin_status; //husker status for rain_pin
float RainDayAccum = 0.0;   //Rain accumulator since start of sample
float RainHourAccum = 0.0;   //Rain accumulator since start of sample
#define RAIN_FACTOR 0.2794  //Argent raingauge 0.2794 mm se https://www.argentdata.com/catalog/product_info.php?products_id=168&osCsid=4gBQZzEt1wYYnSUJ7P9qN3
float Rain24h [24] = { };  //holder sidste 24 timeres regn
int rain_isp_Now;
int rain_isp_Last;
int rain_isp_Pin;
volatile int rain_isp_event;


int lastButtonState; //gememr sidste læsnings værdi
time_t lastDebounceTime;
int debounceDelay;
int buttonState;

void rain_isp_Interrupt()
{

    myDelay(10);
    rain_pin_status = digitalRead(rain_isp_Pin);

    //time(&isp1_Now);
    rain_isp_Now = millis();
    //cout << "Rain_pin_status " << rain_pin_status << " last_rain_pin_status " << last_rain_pin_status << endl;
    //cout << "rain_isp_Now " << rain_isp_Now << " rain_isp_Last " << rain_isp_Last << " dif " << rain_isp_Now-rain_isp_Last << endl;

    if (rain_isp_Now-rain_isp_Last >= 800 && rain_pin_status != last_rain_pin_status)
        //if (rain_isp_Now-rain_isp_Last >= 400)
    {
        if (last_rain_pin_status != 5)   //5 er en opstartsværdi og skal ikke tælles med
        {
            rain_isp_event++;
        }
        rain_isp_Last = millis();
        //cout << "Regn events " << rain_isp_event << endl;
        last_rain_pin_status = rain_pin_status;
    }
}

void rain_isp_Setup(int Pin1)
{
    int err;

    rain_isp_Now = millis();
    rain_isp_Pin = Pin1;
    //cout << "rain_isp_setup" << Pin1 << endl ;
    pinMode (Pin1, INPUT) ;
    pullUpDnControl (Pin1, PUD_UP) ;
    // set Pin 17/0 generate an interrupt on high-to-low transitions
    // and attach myInterrupt() to the interrupt
    err = wiringPiISR (Pin1, INT_EDGE_BOTH, &rain_isp_Interrupt);
    //if {
    //fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
    //  printf("Unable to setup ISR Pin1: %s\n");
// }
    rain_isp_event = 0;
    last_rain_pin_status = 5; //Opstartsværdi der sikre at den er forskellig fra lav og høj.

}

double getUnitRain()
{
    unsigned long reading= rain_isp_event;
    double unit_rain=reading* RAIN_FACTOR;
    rain_isp_event = 0;
    cout << "Regn er beregnet til " <<  unit_rain << endl;
    return unit_rain;
}

int isp2_Now;
int isp2_Last;
int isp2_Pin;
volatile int isp2_event;

// myInterrupt:  called every time an event occurs
void isp2_Interrupt()
{
    //time(&isp1_Now);
    isp2_Now = millis();
    if (isp2_Now-isp2_Last >= 500)
    {
        isp2_event++;
        isp2_Last = millis();
        //cout << "isp2 event " << isp2_event << endl;
    }
}

void isp2_Setup(int Pin2)
{
    int err;

    isp2_Now = millis();
    isp2_Pin = Pin2;

    pinMode (Pin2, INPUT) ;
    pullUpDnControl (Pin2, PUD_UP) ;
    // set Pin 17/0 generate an interrupt on high-to-low transitions
    // and attach myInterrupt() to the interrupt
    err = wiringPiISR (Pin2, INT_EDGE_FALLING, &isp2_Interrupt);
    //if {
    //fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
    //  printf("Unable to setup ISR Pin1: %s\n");
// }
    isp2_event = 0;

}

int isp2_GetCount()
{
    return isp2_event;
}

//==================Interrupt rutiner slut =========================



void OrangeriSetup(void);
void ControlGreenhouseVerySlowTimer();
void ControlGreenhouseSlowTimer();
void ControlGreenhouseFastTimer();

#include <MyStat.h>
#include <ctime>
#include <string>
//#include <bmp180.h>         //Læsning af barometertryk på Bosch kort
#include <DS18B20.h>        //Læsning af temperatur på one wire forbindelse
#include <fstream>
bool Use_WDT = false; //Angiver om Watchdog skal være aktiv i hele programmet eller om det kun benyttes ved BlynkConnect.

//Statistik variable
MyStat TempStats;
MyStat RainDayStats;
MyStat RainHourStats;

//VAriable der sættes for at kunne sende en email
char* emailAdress = "jesper@aaslyng.dk";
char* emailSubject = "";
const char* emailMessage;

//Debug muligheder
//1: lukker vinduer ved upload af nyt program 0: vinduer ændres ikke ved start
//0: læser værdi i xml-fil
bool CloseWindowsOnStart = 1;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

time_t LastRunTime = 0;
time_t NextRunTime = 0;
time_t nu = 0;
time_t LastSetupRunTime= 0;//husker sidste tid der er kørt setup rutine
time_t LastVerySlowTimerEvent; //Holder tyr på sidste timerevent udført
time_t LastSlowTimerEvent; //Holder tyr på sidste timerevent udført
time_t LastFastTimerEvent; //Holder tyr på sidste timerevent udført
time_t LastScreenUpdate; //Holder styr på update af skærm med info om de næste step i programmet
int LastDayChanged = 0;     //Holder styr på den sidte dag der blev iwindow      nitialiseret pga skift af dag
int LastHourChanged = 0;    //Som ovenfor, men for time værdi.
int BlynkResetCount = 0;   // Tæller der holder styr på hvormange gange Blynk ikke har kunne forbindes
int MaxBlynkResetCount = 40; // hvis ikke der er forbindelse til Blynk tælles op til før der gennemføres hardware reset

//String time_tStr;
bool DateInitialized = false; //bruges til at initialiserer diverse variable ved start af ny dag, nulstilling af tællere etc

bool BottonState1;
bool BottonState2;
bool BottonState3;
bool BottonState4;

//Timer object that control rutines in the program. Ingen BLYNK rutiner direkte i loop,
//da det medfører for mange kald og lukning af forbidelsen
//WidgetLED led_HeatOn(HeatUp_Virtual_LED);
//WidgetLED led_VandPumpe(WaterPump_Virtual_LED);
//WidgetLED led_VandHane(WaterTap_Virtual_LED);

//String der løbende benyttes til fejlbeskeder der skal smales til tekst
char ErrStr[200];
//int myBlynkDelay = 0; //Delay der skal sikre at Blynk data ikke overføres for hutigt til server
int RowIndex = 0;  //index der holder værdi for den række der tilføjes i Blynk table. Der lægges 1 til før første start
int TimeForWindowToOpen = 50; //sekunder til at åbne vinduerne helt
int MinSecBetweenMotorMoving = 300; //mindste tid i sekunder mellem at motorerne bevæger sig
int WindowTimerStepInPct = 10; //jeaa ændret beregner hvormange sekunder der rykkes og hvad det svarer til i % åbning/lukning
int WindowOpening_P_Value = 16; //Faktor der ganges med temperatur-fejlen og medfører en ændring i vinduernes åbning på det antal procent
int Status_AutomaticControl = 1;
int Status_HeatUp = 0;
int Status_LightGreenhouse = 0;
int Status_FreshWaterToTankValve = 0;
int Status_FreshWaterToTankValve_last = 0;

int Status_WaterValve1 = 0;
int Status_WaterValve2 = 0;
int Status_WaterValve3 = 0;
int Status_WaterValve4 = 0;
int VandingEkstraMenuSelected = 0;

//Beregning af tid vandingsventiler er åbne
int WaterValve1On = 0;  //Status på om der vandes eller ikke vandes
time_t Stat_TimeWaterValve1;   //Samler sum over vandingstig. Sum gemmes i xml fil og kører indtild er manuelt nulstilles
time_t WaterValve1TimeOn; //holder tid hvor en vanding startes, så der kan beregnes vandingstid

int WaterValve2On = 0;  //Status på om der vandes eller ikke vandes
time_t Stat_TimeWaterValve2;   //Samler sum over vandingstig. Sum gemmes i xml fil og kører indtild er manuelt nulstilles
time_t WaterValve2TimeOn; //holder tid hvor en vanding startes, så der kan beregnes vandingstid

int WaterValve3On = 0;  //Status på om der vandes eller ikke vandes
time_t Stat_TimeWaterValve3;   //Samler sum over vandingstig. Sum gemmes i xml fil og kører indtild er manuelt nulstilles
time_t WaterValve3TimeOn; //holder tid hvor en vanding startes, så der kan beregnes vandingstid

int WaterValve4On = 0;  //Status på om der vandes eller ikke vandes
time_t Stat_TimeWaterValve4;   //Samler sum over vandingstig. Sum gemmes i xml fil og kører indtild er manuelt nulstilles
time_t WaterValve4TimeOn; //holder tid hvor en vanding startes, så der kan beregnes vandingstid


int FreshWaterToTankOn = 0;  //Status på om der vandes eller ikke vandes
time_t Stat_TimeFreshWaterToTank;   //Samler sum over vandingstig. Sum gemmes i xml fil og kører indtild er manuelt nulstilles
time_t FreshWaterToTankTimeOn; //holder tid hvor en vanding startes, så der kan beregnes vandingstid

int HeatOn = 0;  //Status på om der vandes eller ikke vandes
time_t Stat_TimeHeat;   //Samler sum over vandingstig. Sum gemmes i xml fil og kører indtild er manuelt nulstilles
time_t HeatTimeOn; //holder tid hvor en vanding startes, så der kan beregnes vandingstid

int LightOn = 0;  //Status på om der vandes eller ikke vandes
time_t Stat_TimeLight;   //Samler sum over vandingstig. Sum gemmes i xml fil og kører indtild er manuelt nulstilles
time_t LightTimeOn; //holder tid hvor en vanding startes, så der kan beregnes vandingstid

//int Status_AddWaterValve =  nu ;
//int Status_AddFreshWaterValve = 0; //Holder status på tilstand af ventil, der åbner for vandhanevand til palletank.
int Status_WaterManual1 = 0;   //sættes til 1 når vanding kører manuel ellers 1
int Status_WaterManual2 = 0;   //sættes til 1 når vanding kører manuel ellers 1
int Status_WaterManual3 = 0;   //sættes til 1 når vanding kører manuel ellers 1
int Status_WaterManual4 = 0;   //sættes til 1 når vanding kører manuel ellers 1
int Status_VandingStartTime1 = 9;  //Tid for start af første vanding
int Status_VandingStartTime2 = 20; //Tid fors tart af anden vanding
int Status_VandingsInterval3 = 16;
int Status_VandingsInterval4 = 16;
int Status_VandingsTid1 = 5;
int Status_VandingsTid2 = 5;
int Status_VandingsTid3 = 5;
int Status_VandingsTid4 = 5;

int LedPinOn = 0;

time_t Status_LastWatering1;
time_t Status_LastWatering2;
time_t Status_LastWatering3;
time_t Status_LastWatering4;
time_t Status_LastStartTime1 = 0;
time_t Status_LastStartTime2 = 0;

time_t StopWatering1 = 0;
time_t StopWatering2 = 0;
time_t StopWatering3 = 0;
time_t StopWatering4 = 0;
time_t StopHeating = 0;

float CPUTemperature;
float CPUTemperature_last;

float Setpoint_TemperatureDay = 10;   //Temperatur sætpunkt om dagen
float Setpoint_TemperatureNight = 10; //Temperatur sætpunkt om natten
float Setpoint_UdluftningTemperature = 24;   //Temperatur hvor over der begynder at blvie luftet
float Setpoint_UdluftningTemperatureComfort = 27;   //Temperatur hvor over der begynder at blvie luftet
float Setpoint_Udluftning = Setpoint_UdluftningTemperature; //Holder udluftningstemperaturen der bestemmes efter komfort eller ikke komfort
float Setpoint_MaxTemperature = 30;   //Max tilladt temperatur før luftning
float Setpoint_TemperatureNow = 25;   //Temperatur der styres efter lige nu. Beregnet på baggrund af dag/dat m.m.

float Error_HeatUp = 0; //Variabel der holder accumuleret fejl ved beregning
float P_HeatUp = 30; //P-værdie der ganges med temp forskel for at definerer antal sekunder der skal varmes op for at holde temperaturen
float I_HeatUp = 0; //I-værdi der benyttes ved at kigge på accumuleret tempearaturfejl
int AddHeatTime;     //holder værdi der benyttes ved beregning af hvor lang tid der skal tilføres varme i PI beregningen

//Nyt ønske om vinduesåbning samt status på den nuværende åbning
float NewStatus_Window1;
float NewStatus_Window2;
float NewStatus_Window3;
float NewStatus_Window4;
float CurrentStatus_Window1;
float CurrentStatus_Window2;
float CurrentStatus_Window3;
float CurrentStatus_Window4;
float TmpDif;
float WinOpn;

int DayStartHour = 6;
int NightStartHour = 22;
int ComfortDayStartHour = 16;
int ComfortDayEndHour = 22;


time_t StopMotor1 = 0;
time_t StopMotor2 = 0;
time_t StopMotor3 = 0;
time_t StopMotor4 = 0;
bool Motor1_Moving;
bool Motor2_Moving;
bool Motor3_Moving;
bool Motor4_Moving;
bool AutoWindow = true;
time_t WindowsChangedTime; //holder styr på hvornår der sidst blev styret vinduer
bool isFirstConnect = true; //er true indtil initialisering og sync med Blynk er gennemført

//Justering af vanding efter høj temperatur
float TempAvgLastDay = 20;
float TempAvgDay = 20;
float AddExtraWaterTime = 1.0; //faktor der ganges med for at tilfører ekstra vand ved høje temperaturer dagen før eller den aktuelle temperatur

//Parametre der holder gemte værdier at blynk uploadeded værdier
float ControlBoxTemperature_last = 0;
float ControlBoxPressure_last = 0;
float ControlBoxHumidity_last = 0;
float Setpoint_TemperatureNow_last = 0;
float AirFlowValue;
float AirFlowValue_last = 0;
float SoilMoistureValue1_last = 0;
float SoilMoistureValue2_last = 0;
float SoilMoistureValue1;
float SoilMoistureValue2;
float SoilMoistureValue3;


float Greenhouse_temperature;
float Greenhouse_temperature_last = 0;
float Temperature4Vent;
float TempAvgDay_last = 0;

float Greenhouse_humidity;
float Greenhouse_humidity_last = 0;

float Extra1_temperature;
float Extra1_temperature_last = 0;
float Extra2_temperature;
float Extra2_temperature_last = 0;

float Weather_temperature;
float Weather_temperature_last = 0;
float Weather_humidity;
float Weather_humidity_last = 0;

float TempAvgLastDay_last = 0;
float RainHourAccum_last = 0;
float RainDayAccum_last = 0;


//--------------------
std::string str ("Test string");
std::string SidsteVandingsLabel1 = "-";
std::string SidsteVandingsLabel1_last = "-";
std::string SidsteVandingsLabel2 = "-";
std::string SidsteVandingsLabel2_last = "-";
std::string SidsteVandingsLabel3 = "-";
std::string SidsteVandingsLabel3_last = "-";
std::string SidsteVandingsLabel4 = "-";
std::string SidsteVandingsLabel4_last = "-";

std::string WaterLevel1_Msg_Virtual_Pin_msg = "-";
std::string WaterLevel1_Msg_Virtual_Pin_msg_last = "-";


//kontrollerede adresser
DS18B20 Probe03("28-0316622393ff");  //Virker muligvis ikke længere, gammel vejrstation sensor der gav problemer
DS18B20 Probe02("28-0316473b91ff");  //Drivhus på lang ledning (orange)

//skal kontrolleres igen
DS18B20 Probe01("28-0316555af8ff");  //Gul, ændret til Vejrstation
DS18B20 Probe04("28-0316556970ff");  //sort
//DS18B20 Probe06("28-041650bc06ff");  //grøn
DS18B20 Probe06("28-030a97940080");  //grøn



DS18B20 Probe05("28-06BC5016040B");  //Klar

DS18B20 Probe07("28-0316622393ff");  //Rød+Grøn



//--------------------DHT Temperature Sensor -------------------------
//DHT 22  (AM2302), AM2321
//DHT 21 (AM2301)
//DHT11   // DHT 11

#include <pi_2_dht_read.h>  //Læsning af DHT sensorer DHT11/DHT22/AM2302
#include <pi_2_mmio.h>      //Benyttes som ovenover til læsning af DHT sensorer

int dht_res;
float dht_humidity;
float dht_temperature;

/*
DHT dht_Greenhouse(DHT_Greenhouse_PIN, DHT22);
DHT dht_Extra1(DHT_Extra1_PIN, DHT22);
DHT dht_Extra2(DHT_Extra2_PIN, DHT22);
*/



float Probe01_Temp, Probe02_Temp, Probe03_Temp, Probe04_Temp, Probe05_Temp, Probe06_Temp, Probe07_Temp;

//WaterLevel status
bool Status_WaterLevel1 = true;
bool Status_WaterLevel2 = true;

int WaterReadTop;
int WaterReadButtom;

//------------------Lys sensorer ---------------------------------
int LightValue;
//konvertering mangler at bliver programmeret

//------------------Jord fugtighed -------------------------------

// Here we are setting up some water thersholds that we will
// use later. Note that you will need to change these to match
// your soil type and environment.

int SoilMoistureThresholdUp = 400;
int SoilMoistureThresholdDown = 250;



//-------------------Water flow ----------------------------------
// The hall-effect flow sensor outputs approximately 4.5 pulses per second per litre/minute of flow.
float calibrationFactor = 7.5;
volatile int WaterFlowPulseCount;
float WaterFlowRate;
unsigned int WaterFlowMililitres;
unsigned long WaterTotalMililitres;
unsigned long oldTime;


void setupWaterFlowInts()
{
    pinMode(Water_Flow_Pin, INPUT);
    digitalWrite(Water_Flow_Pin, HIGH);
    WaterFlowPulseCount        = 0;
    WaterFlowRate          = 0.0;
    WaterFlowMililitres   = 0;
    WaterTotalMililitres  = 0;
    oldTime           = 0;
    // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
    // Configured to trigger on a FALLING state change (transition from HIGH
    // state to LOW state)
    //jeaa attachInterrupt(digitalPinToInterrupt(Water_Flow_Pin),WaterFlowClick, FALLING);
}

// Insterrupt Service Routine for vand flow måling
void WaterFlowClick()
{
    WaterFlowPulseCount++; // Increment the pulse counter
}


//--------------------BMP Sensor for Lufttryk-------------------------------
//SFE_BMP180 pressure; // You will need to create an SFE_BMP180 object, here called "pressure":%
//#define ALTITUDE 45.9 // Altitude aflæst på vores adresse på www.altitude-maps.com
float ControlBoxTemperature;  //Temperatur målt i kontrolboks med Tryksensor
float ControlBoxPressure;     //Tryk målt i kontrolboks med Tryksensor
float ControlBoxHumidity;

int getMonth(time_t TimeInput)
{
    struct tm *tm = localtime(&TimeInput);
    return 1 + tm->tm_mon;
}

int getDay(time_t TimeInput)
{
    struct tm *tm = localtime(&TimeInput);
    return tm->tm_mday;
}

int getHour(time_t TimeInput)
{
    struct tm *tm = localtime(&TimeInput);
    return tm->tm_hour;
}

int getMinute(time_t TimeInput)
{
    struct tm *tm = localtime(&TimeInput);
    return tm->tm_min;
}

int getSecond(time_t TimeInput)
{
    struct tm *tm = localtime(&TimeInput);
    return tm->tm_sec;
}

void ControlMotor(int MotorNumber)
{
    int pwm;
    int dir;
    int diff;
    bool moving;
    float currentstatus;
    float newstatus;
    time_t stopmotor;
    switch (MotorNumber)
    {
    case 1:
        //pwm = Motor1_pwm;
        //dir = Motor1_dir;
        moving = Motor1_Moving;
        currentstatus = CurrentStatus_Window1;
        newstatus = NewStatus_Window1;
        stopmotor =  StopMotor1;
        break;
    case 2:
        //pwm = Motor1_pwm;
        //dir = Motor1_dir;
        moving = Motor2_Moving;
        currentstatus = CurrentStatus_Window2;
        newstatus = NewStatus_Window2;
        stopmotor =  StopMotor2;
        break;
    case 3:
        //pwm = Motor1_pwm;
        //dir = Motor1_dir;
        moving = Motor3_Moving;
        currentstatus = CurrentStatus_Window3;
        newstatus = NewStatus_Window3;
        stopmotor =  StopMotor3;
        break;
    case 4:
        //pwm = Motor1_pwm;
        //dir = Motor1_dir;
        moving = Motor4_Moving;
        currentstatus = CurrentStatus_Window4;
        newstatus = NewStatus_Window4;
        stopmotor =  StopMotor4;
        break;
    }

    nu = time(NULL);
    string msg = showDate(nu);

    if (MotorNumber == 1)
    {
        //cout << "Motornumber " << MotorNumber << " Current " << currentstatus << " New " << newstatus << " Moving " << moving << endl;
        //cout << "----- Nu i motorstyring " << msg << "\n";
        //cout << "Moving " << moving << endl;
    }

    if (currentstatus != newstatus)
    {
        //cout << "Current != newstatus new/current" << newstatus << " " << currentstatus << endl;
        diff = newstatus - currentstatus;

        if (diff != 0 && moving == 0)
        {
            if (diff > 0)
            {
                //ExPi_IOPortWrite(dir, HIGH);
                ExPi_IOPortWrite(Pi_Relay_Motor1_dir, 1);
            } //controls the direction the motor}

            else
            {
                //ExPi_IOPortWrite(dir, LOW);
                ExPi_IOPortWrite(Pi_Relay_Motor1_dir, 0);
            }

            ExPi_IOPortWrite(Pi_Relay_Motor1_pwm, 1);
            //ExPi_IOPortWrite(pwm, HIGH);

            moving = 1;
            diff = abs(diff);
            diff = (int) diff*TimeForWindowToOpen/100;
            stopmotor = nu + diff;

            if (newstatus==0 || newstatus>96)
            {
                stopmotor = stopmotor + 150;   //hvis der gås til lukket/helt åbent vindue lægges ekstra tid på for at sikre at vinduerne lukker helt
            }

        }
    }

    if (nu >= stopmotor && moving == 1)
    {
        cout << "Motor stoppes new/current " << newstatus << " " << currentstatus << endl;

        //ExPi_IOPortWrite(pwm, LOW);
        ExPi_IOPortWrite(Pi_Relay_Motor1_pwm, 0);

        moving = 0;
        currentstatus = newstatus;
        if (UpdateBlynkOk()) {Blynk.virtualWrite(V2,5);} //Sætter vindues menu til 5 (=ingen ændring)
    }


    switch (MotorNumber)
    {
    case 1:
        StopMotor1 = stopmotor;
        Motor1_Moving = moving;
        CurrentStatus_Window1 = currentstatus;
        NewStatus_Window1 = newstatus;
        break;
    case 2:
        StopMotor2 = stopmotor;
        Motor2_Moving = moving;
        CurrentStatus_Window2 = currentstatus;
        NewStatus_Window2 = newstatus;
        break;
    case 3:
        StopMotor3 = stopmotor;
        Motor3_Moving = moving;
        CurrentStatus_Window3 = currentstatus;
        NewStatus_Window3 = newstatus;

        break;
    case 4:
        StopMotor4 = stopmotor;
        Motor4_Moving = moving;
        CurrentStatus_Window4 = currentstatus;
        NewStatus_Window4 = newstatus;

        break;
    }

    if (MotorNumber == 1)
    {
        //cout << "Afslutning på ControlWindows procedure \n";
        //cout << endl;
    }

}

//Styrer vinduerne 1-3
void ControlWindows()
{
//cout << "ControlWindows" << endl;
//if (StopVentsButtonActive == 0)
    for (int i=1; i <= 4; i++)
    {
        ControlMotor(i);
    }

}

//-------------------Rutiner der kaldes når værdier i Blynk widgets ændres, altså på de mobile enheder.-------------


BLYNK_WRITE(ComfortDayPeriod_Virtual_Pin)
{

    cout << "ComfortDayPeriod ændret \n";

    TimeInputParam t(param);

// Process start time

    if (t.hasStartTime())
    {
        ComfortDayStartHour = t.getStartHour();
        cout << "Comfort Start: " << t.getStartHour() << ":" << t.getStartMinute() << ":" << t.getStartSecond() << "\n";
    }

    // Process stop time

    if (t.hasStopTime())
    {
        ComfortDayEndHour = t.getStopHour();
        if (t.getStopMinute()  > 29)
        {
            NightStartHour = NightStartHour+1;
        }
        cout << "Comfort Stop: " << t.getStopHour() << ":" << t.getStopMinute() << ":" << t.getStopSecond() << "\n";
    }
}


BLYNK_WRITE(DayNight_Virtual_Pin)
{
    TimeInputParam t(param);

// Process start time

    if (t.hasStartTime())
    {
        DayStartHour = t.getStartHour();
        cout << "Start: " << t.getStartHour() << ":" << t.getStartMinute() << ":"  << t.getStartSecond() << "\n";
    }

    // Process stop time

    if (t.hasStopTime())
    {
        NightStartHour = t.getStopHour();
        if (t.getStopMinute()  > 29)
        {
            NightStartHour = NightStartHour+1;
        }
        cout << "Stop: " << t.getStopHour() << ":" << t.getStopMinute() << ":" << t.getStopSecond() << "\n";
    }
}

BLYNK_WRITE(WindowPct_Virtual_Pin)
{
    /*    CurrentStatus_Window1 = param.asFloat(); // assigning incoming value from pin V1 to a variable
        CurrentStatus_Window2 = CurrentStatus_Window1;
        CurrentStatus_Window3 = CurrentStatus_Window1;
        CurrentStatus_Window4 = CurrentStatus_Window1;
        NewStatus_Window1 = CurrentStatus_Window1;
        NewStatus_Window2 = CurrentStatus_Window1;
        NewStatus_Window3 = CurrentStatus_Window1;
        NewStatus_Window4 = CurrentStatus_Window1;
        //JEAA Motor NB !fjernet da der var pdroblemer med værdien LastRunTime ikke passede ved statistik beregninger, men effekten her er ikke helt kendt
        //LastRunTime = 0;


        cout << "CurrentStatus_Window1: ";
        cout << CurrentStatus_Window1 << "\n";      */
}


BLYNK_WRITE(AutomaticControl_Virtual_Pin)
{
    Status_AutomaticControl = param.asInt(); // assigning incoming value from pin V1 to a variable
    cout << "AutomaticControl: ";
    cout << Status_AutomaticControl << "\n";
}


/*BLYNK_WRITE(P_HeatUp_Virtual_Pin)
 {
    P_HeatUp = param.asInt(); // assigning incoming value from pin V1 to a variable
    cout << "P_HeatUp: ";
    cout << P_HeatUp << "\n";
  }


BLYNK_WRITE(I_HeatUp_Virtual_Pin)
 {
    I_HeatUp = param.asInt(); // assigning incoming value from pin V1 to a variable
    cout << "I_HeatUp: ";
    cout << I_HeatUp << "\n";
  }
*/


//waterbutton_PumpOn_minutes_Virtual_Pin   V74
BLYNK_WRITE(V7)
{
    cout << "!!!! waterbutton_PumpOn_minutes " << param.asInt() << endl;
    waterbutton_PumpOn_minutes = param.asInt(); // assigning incoming value from pin V1 to a variable
}


BLYNK_WRITE(StopVents_minutes_Virtual_Pin)
{
    cout << "Stop Vents minutes " << param.asInt() << endl;
    StopVentsButton_minutes = param.asInt(); // assigning incoming value from pin V1 to a variable
}


BLYNK_WRITE(VandingsTime1_Virtual_Pin)
{
    #if defined(DebugWater)
    cout << "VandingsTid 1 " << param.asInt() << endl;
    #endif // defined
    Status_VandingsTid1 = param.asInt(); // assigning incoming value from pin V1 to a variable
}

BLYNK_WRITE(VandingsTime2_Virtual_Pin)
{
    #if defined(DebugWater)
    cout << "VandingsTid 2 " << param.asInt() << endl;
    #endif // defined
    Status_VandingsTid2 = param.asInt(); // assigning incoming value from pin V1 to a variable
}


BLYNK_WRITE(VandingsTime3_Virtual_Pin)
{
    #if defined(DebugWater)
    cout << "VandingsTid 3 " << param.asInt() << endl;
    #endif // defined
    Status_VandingsTid3 = param.asInt(); // assigning incoming value from pin V1 to a variable
}


BLYNK_WRITE(VandingsTime4_Virtual_Pin)
{
    #if defined(DebugWater)
    cout << "VandingsTid 4 " << param.asInt() << endl;
    #endif // defined
    Status_VandingsTid4 = param.asInt(); // assigning incoming value from pin V1 to a variable
}

BLYNK_WRITE(VandingStartTime1_Virtual_Pin)
{

    #if defined(DebugWater)
      cout << "VandingsStartTime 1 ";
    #endif // defined

    TimeInputParam t(param);

// Process start time
    if (t.hasStartTime())
    {
        DayStartHour = t.getStartHour();
    #if defined(DebugWater)
        cout << "Start: " << t.getStartHour() << ":" << t.getStartMinute() << ":"  << t.getStartSecond() << "\n";
    #endif // defined
    }

    Status_VandingStartTime1 = t.getStartHour();

}

BLYNK_WRITE(VandingStartTime2_Virtual_Pin)
{
    #if defined(DebugWater)
    cout << "VandingsStartTime 2 ";
    #endif // defined

    TimeInputParam t(param);

// Process start time
    if (t.hasStartTime())
    {
        DayStartHour = t.getStartHour();
        #if defined(DebugWater)
        cout << "Start: " << t.getStartHour() << ":" << t.getStartMinute() << ":"  << t.getStartSecond() << "\n";
         #endif // defined
    }


    Status_VandingStartTime2 = t.getStartHour();

}


BLYNK_WRITE(VandingsInterval3_Virtual_Pin)
{
    #if defined(DebugWater)
      cout << "Vandingsinterval 3 ";
      cout << param.asInt() << "\n";
    #endif // defined
    Status_VandingsInterval3 = param.asInt(); // assigning incoming value from pin V1 to a variable
}


BLYNK_WRITE(VandingsInterval4_Virtual_Pin)
{
    #if defined(DebugWater)
      cout << "Vandingsinterval 4 ";
      cout << param.asInt() << "\n";
    #endif // defined
    Status_VandingsInterval4 = param.asInt(); // assigning incoming value from pin V1 to a variable
}


BLYNK_WRITE(TempAvgLastDay_Virtual_Pin)
{
    cout << "Temperatur gennemsnit sidste dag ";
    cout << param.asFloat() << "\n";
    TempAvgLastDay = param.asFloat();
}


BLYNK_WRITE(TempAvgDay_Virtual_Pin)
{
    cout << "Temperatur gennemsnit aktuel dag ";
    cout << param.asFloat() << "\n";
    TempAvgDay = param.asFloat();
}

//Gør det muligt at vælge en anden sensor ti at styre vunduere efter
//enbd den almindelige Orangeri temperatur, der også styrer opvarmning.
BLYNK_WRITE(SensorToControlwindow_Pin)
{
    switch (param.asInt())
    {
    case 1: //Standrad orangeri sensor
    {
        WinterTent = FALSE;
        cout << "Orangeri temperatursensor benyttes til styring af vinduer" << endl;
    }
    case 2: // extra sensor
    {
        WinterTent = TRUE;
        cout << "Ekstra temperatursensor benyttes til styring af vinduer" << endl;
    }
    }
}

BLYNK_WRITE(LogEmptyNow_Virtual_Pin)
{
    int CurrentLogStatus = enableLog;
    switch (param.asInt())
    {
    case 0:
        enableLog = 1;
        cout << "Log bruges" << endl;

        break;
    case 1:
        enableLog = 0;
        cout << "Log bruges ikke" << endl;
        break;
    case 2:
        EmptyLog();
        cout << "Log tømt for data" << endl;
        if (UpdateBlynkOk()) {Blynk.virtualWrite(LogEmptyNow_Virtual_Pin, CurrentLogStatus);}
        break;
    }

}


BLYNK_WRITE(VandingEkstra_Menu_Pin)     //af mystiske grunde virker det ikke når jeg bruger navnet der er defineres som for de øvrige virtuelle pinde
{

    cout <<"VandingEkstra1" << endl;

    nu = time(NULL);
    VandingEkstraMenuSelected = param.asInt();
    cout << "Menu selected "  << VandingEkstraMenuSelected << endl;
    switch (param.asInt())
    {
    case 1:
    {
        // Ingen vanding
        /*
        Status_WaterPump = 0; // starter vandpumpe
        Status_WaterValve1 = 0; // åbner ventil 1
        Status_WaterManual1 = 0; //Ikke manuel vanding
        Status_WaterValve2 = 0; // åbner ventil 1
        Status_WaterManual2 = 0; //Ikke manuel vanding
        Status_WaterValve3 = 0; // åbner ventil 1
        Status_WaterManual3 = 0; //Ikke manuel vanding
        Status_WaterValve4 = 0; // åbner ventil 1
        Status_WaterManual4 = 0; //Ikke manuel vanding
        */
        break;
    }
    case 2:
    {
        // 5 min ekstra vand
        cout << "5 min vand til kanal 1" << endl;
        Status_WaterValve1 = 1; // åbner ventil 1
        Status_LastWatering1 =  nu ;
        SidsteVandingsLabel1 = showDate(nu);
        StopWatering1 = nu + 300;
        Status_WaterManual1 = 0; //Ikke manuel vanding efter tryk på knap
        break;
    }

    case 3:
    {
        // 5 min ekstra vand
        cout << "5 min vand til kanal 2" << endl;
        Status_WaterValve2 = 1; // åbner ventil 1
        Status_LastWatering2 =  nu ;
        SidsteVandingsLabel2 = showDate(nu);
        StopWatering2 = nu + 300;
        Status_WaterManual2 = 0; //Ikke manuel vanding
        break;
    }


    case 4:
    {
        // 5 min ekstra vand
        cout << "5 min vand til kanal 3" << endl;
        Status_WaterValve3 = 1; // åbner ventil 1
        Status_LastWatering3 =  nu ;
        SidsteVandingsLabel3 = showDate(nu);
        StopWatering3 = nu + 300;
        Status_WaterManual3 = 0; //Ikke manuel vanding
        break;
    }


    case 5:
    {
        // 5 min ekstra vand
        cout << "5 min vand til kanal 4" << endl;
        Status_WaterValve4 = 1; // åbner ventil 1
        Status_LastWatering4 =  nu ;
        SidsteVandingsLabel4 = showDate(nu);
        StopWatering4 = nu + 300;
        Status_WaterManual4 = 0; //Ikke manuel vanding
        break;
    }

    case 6:
    {
        // 5 min ekstra vand
        cout << "5 min vand til palletank" << endl;

        Status_WaterValve1 = 1; // åbner ventil 1
        Status_LastWatering1 =  nu ;
        SidsteVandingsLabel1 = showDate(nu);
        StopWatering1= nu + 300;

        Status_WaterManual2 = 0; //Ikke manuel vanding
        Status_WaterValve2 = 1; // åbner ventil 1
        Status_LastWatering2 =  nu ;
        SidsteVandingsLabel2= showDate(nu);
        StopWatering2 = nu + 300;

        Status_WaterManual3 = 0; //Ikke manuel vanding
        Status_WaterValve3 = 1; // åbner ventil 1
        Status_LastWatering3 =  nu ;
        SidsteVandingsLabel3 = showDate(nu);
        StopWatering3 = nu + 300;
        Status_WaterManual3 = 0; //Ikke manuel vanding

        Status_WaterValve4 = 1; // åbner ventil 1
        Status_LastWatering4 =  nu ;
        SidsteVandingsLabel4 = showDate(nu);
        StopWatering4 = nu + 300;
        Status_WaterManual4 = 0; //Ikke manuel vanding

        break;
    }

    case 7:
    {
        // Vanding startes og sluttes manuelt
        string msg;
        Status_FreshWaterToTankValve = 1; // åbner ventil 3 der tilfører vand til vandtank
        cout << "Vandhanevand tændes i 5 minutter" << endl;
        break;
    }


    }
}

BLYNK_WRITE(V2)     //af mystiske grunde virker det ikke når jeg bruger navnet der er defineres som for de øvrige virtuelle pinde
{

    WindowsChangedTime = 0;  //Nulstiller sidst brugte justerings tid
    switch (param.asInt())
    {
    case 1:
    {
        // Item 1 Luk vinduer helt
        cout << "Item 1 selected \n";
        NewStatus_Window1 = 0;
        NewStatus_Window2 = 0;
        NewStatus_Window3 = 0;
        NewStatus_Window4 = 0;
        break;
    }
    case 2:
    {
        // Item 2 åben vinduer helt
        cout << "Item 2 selected \n";
        NewStatus_Window1 = 100;
        NewStatus_Window2 = 100;
        NewStatus_Window3 = 100;
        NewStatus_Window4 = 100;
        break;
    }
    case 3:
    {
        // Item 3 åben vinduer et trin mere
        cout << "Item 3 selected \n";
        NewStatus_Window1 = NewStatus_Window1 + WindowTimerStepInPct;
        NewStatus_Window2 = NewStatus_Window2 + WindowTimerStepInPct;
        NewStatus_Window3 = NewStatus_Window3 + WindowTimerStepInPct;
        NewStatus_Window4 = NewStatus_Window4 + WindowTimerStepInPct;
        if (NewStatus_Window1 >= 100)
        {
            NewStatus_Window1 = 100;
        }
        if (NewStatus_Window2 >= 100)
        {
            NewStatus_Window2 = 100;
        }
        if (NewStatus_Window3 >= 100)
        {
            NewStatus_Window3 = 100;
        }
        if (NewStatus_Window4 >= 100)
        {
            NewStatus_Window4 = 100;
        }
        break;
    }

    case 4:
    {
        // Item 4 luk vinduer et trin mere
        cout << "Item 4 selected \n";
        NewStatus_Window1 = NewStatus_Window1 - WindowTimerStepInPct;
        NewStatus_Window2 = NewStatus_Window2 - WindowTimerStepInPct;
        NewStatus_Window3 = NewStatus_Window3 - WindowTimerStepInPct;
        NewStatus_Window4 = NewStatus_Window4 - WindowTimerStepInPct;
        if (NewStatus_Window1 <= 0)
        {
            NewStatus_Window1 = 0;
        }
        if (NewStatus_Window2 <= 0)
        {
            NewStatus_Window2 = 0;
        }
        if (NewStatus_Window3 <= 0)
        {
            NewStatus_Window3 = 0;
        }
        if (NewStatus_Window4 <= 0)
        {
            NewStatus_Window4 = 0;
        }
        break;
    }
    case 5:
    {
        //ingenting sker
    }
    }

}


BLYNK_WRITE(HeatUp_Virtual_Pin)
{
    //Status_HeatUp = param.asInt(); // assigning incoming value from pin V1 to a variable
    //cout << "Heat value change: ";
    //cout << Status_HeatUp << "\n";
}

BLYNK_WRITE(LightGreenhouse_Virtual_Pin)
{
    Status_LightGreenhouse = param.asInt(); // assigning incoming value from pin V1 to a variable
    cout << "LightGreenhouse value change: ";
    cout << Status_LightGreenhouse << "\n";
}

BLYNK_WRITE(FreshWaterValve_Virtual_Pin)
{
    //Status_FreshWaterToTankValve = param.asInt(); // assigning incoming value from pin V1 to a variable
    //cout << "Vandhanevand ventil ændret: " << Status_FreshWaterToTankValve << "\n";
}

BLYNK_WRITE(WaterValve1_Virtual_Pin)
{
    Status_WaterValve1 = param.asInt(); // assigning incoming value from pin V1 to a variable
    cout << "Water Valve 1 change: ";
    cout << Status_WaterValve1 << "\n";
}



BLYNK_WRITE(WaterValve2_Virtual_Pin)
{
    Status_WaterValve2 = param.asInt(); // assigning incoming value from pin V1 to a variable
    cout << "Water Valve 2 change: ";
    cout << Status_WaterValve2 << "\n" ;
}
BLYNK_WRITE(WaterValve3_Virtual_Pin)
{
    Status_WaterValve3 = param.asInt(); // assigning incoming value from pin V1 to a variable
    cout << "Water Valve 3 change: ";
    cout << Status_WaterValve3 << "\n" ;
}

BLYNK_WRITE(WaterValve4_Virtual_Pin)
{
    Status_WaterValve4 = param.asInt(); // assigning incoming value from pin V1 to a variable
    cout << "Water Valve 4 change: ";
    cout << Status_WaterValve4 << "\n" ;
}

BLYNK_WRITE(TemperatureDay_Virtual_Pin)
{
    Setpoint_TemperatureDay = param.asFloat(); // assigning incoming value from pin V1 to a variable
    cout << "Dag temperatur ændret: ";
    cout << Setpoint_TemperatureDay << "\n";
}

BLYNK_WRITE(TemperatureNight_Virtual_Pin)
{
    Setpoint_TemperatureNight = param.asFloat(); // assigning incoming value from pin V1 to a variable
    cout << "Dag temperatur ændret: ";
    cout << Setpoint_TemperatureNight << "\n";
}

BLYNK_WRITE(TemperatureUdluftning_Virtual_Pin)
{
    Setpoint_UdluftningTemperature = param.asFloat(); // assigning incoming value from pin V1 to a variable
    cout << "Udluftningstemperatur ændret: ";
    cout << Setpoint_UdluftningTemperature << "\n";
}

BLYNK_WRITE(TemperatureUdluftningComfort_Virtual_Pin)
{
    Setpoint_UdluftningTemperatureComfort = param.asFloat(); // assigning incoming value from pin V1 to a variable
    cout << "Udluftningstemperatur ved komfort ændret: ";
    cout << Setpoint_UdluftningTemperatureComfort << "\n";
}


BLYNK_WRITE(ManualChangeWindow_Virtual_Pin)
{
    int pinData = param.asInt();
    if (pinData == 1 && Motor1_Moving == 0)
    {
        NewStatus_Window1 = NewStatus_Window1 + WindowTimerStepInPct;
        if (NewStatus_Window1 >= 100)
        {
            NewStatus_Window1 = 0;
        }
        cout <<("********** Windues åbning ændret ved tryk på knap *************** ");
        cout << NewStatus_Window1 << "\n";
        if (UpdateBlynkOk()) {Blynk.virtualWrite(V25, 0);}
    }
}


BLYNK_WRITE(AutoWindow_Virtual_Pin)
{

    int pinData = param.asInt();

    if (pinData == 1)
    {
        AutoWindow = true;
    }
    else
    {
        AutoWindow = false;
    }

}



BLYNK_CONNECTED()
{
    if (isFirstConnect)
    {
        Blynk.syncAll();
        isFirstConnect = false;
        Blynk.syncAll();
    }

    Blynk.syncAll();

}


void SetFreshWatertoTank(bool FreshWaterToTank)
{

    if (FreshWaterToTank == 0)
    {
        ExPi_IOPortWrite(Pi_Relay_FreshWater, 0);
    }
    else
    {
        ExPi_IOPortWrite(Pi_Relay_FreshWater, 1);
    }

    //ExPi_IOPortWrite(Pi_Relay_FreshWater, FreshWaterToTank);

#if defined(DebugWater)
    cout << "SetFreshWaterToTank " << FreshWaterToTank << endl;
#endif // defined

    if (FreshWaterToTankOn != FreshWaterToTank)    //status på vanding ændres
    {
        if (FreshWaterToTank == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeFreshWaterToTank = Stat_TimeFreshWaterToTank + nu - FreshWaterToTankTimeOn;
            FreshWaterToTankOn = 0;
            Blynk.logEvent("WaterToContainerOff");
        }

        if (FreshWaterToTank == 1)    //der er blevet tændt for vandet
        {
            FreshWaterToTankTimeOn = time(0);
            FreshWaterToTankOn = 1;
            Blynk.logEvent("WaterToContainerOn");
        }
    }
}

void SetWaterValve1(bool WaterValve1)
{

#if defined(DebugWater)

    cout << "SetWaterValve1 kaldt med værdien " << WaterValve1 <<endl;
    cout << "WaterValve1On " << WaterValve1On << endl;
#endif // defined

    if (WaterValve1On != WaterValve1)    //status på vanding ændres
    {
        if (WaterValve1 == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeWaterValve1 = Stat_TimeWaterValve1 + nu - WaterValve1TimeOn;
            WaterValve1On = 0;
#if defined(DebugWater)
            cout << "Der slukkes for vandet (1)" << endl;
            cout << "+++++" << Stat_TimeWaterValve1 << " nu " << " WaterValve1TimeOn " << WaterValve1TimeOn  << endl;
#endif // defined
        }

        if (WaterValve1 == 1)    //der er blevet tændt for vandet
        {
            WaterValve1TimeOn = time(0);
            WaterValve1On = 1;

#if defined(DebugWater)
            cout << "Der tændes for vandet(1)" << endl;
#endif // defined

            Blynk.logEvent("Watering1");
        }
    }

    //Sætter relæ
    ExPi_IOPortWrite(Pi_Relay_WaterValve1, WaterValve1);

}


void SetWaterValve2(bool WaterValve2)
{
    //cout << "SetWaterValve2 " << WaterValve2 << " " << WaterValve2On << endl;

#if defined(DebugWater)
    cout << "SetWaterValve2 kaldt med værdien " << WaterValve2 <<endl;
    cout << "WaterValve2On " << WaterValve2On << endl;
#endif // defined

    if (WaterValve2On != WaterValve2)    //status på vanding ændres
    {
        if (WaterValve2 == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeWaterValve2 = Stat_TimeWaterValve2 + nu - WaterValve2TimeOn;
            WaterValve2On = 0;
#if defined(DebugWater)
            cout << "Der slukkes for vandet(2) " << endl;
            cout << "+++++" << Stat_TimeWaterValve2 << " nu " << " WaterValve2TimeOn " << WaterValve2TimeOn  << endl;
#endif // defined
        }

        if (WaterValve2 == 1)    //der er blevet tændt for vandet
        {
            WaterValve2TimeOn = time(0);
            WaterValve2On = 1;
#if defined(DebugWater)
            cout << "Der tændes for vandet (2)" << endl;
#endif // defined
            Blynk.logEvent("Watering2");
        }
    }

    //Sætter relæ
    ExPi_IOPortWrite(Pi_Relay_WaterValve2, WaterValve2);

}


void SetWaterValve3(bool WaterValve3)
{

#if defined(DebugWater)
    cout << "SetWaterValve3 kaldt med værdien " << WaterValve3 <<endl;
    cout << "WaterValve3On " << WaterValve3On << endl;
#endif // defined

    if (WaterValve3On != WaterValve3)    //status på vanding ændres
    {
        if (WaterValve3 == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeWaterValve3 = Stat_TimeWaterValve3 + nu - WaterValve3TimeOn;
            WaterValve3On = 0;
#if defined(DebugWater)
            cout << "Der slukkes for vandet(3) " << endl;
            cout << "+++++" << Stat_TimeWaterValve3 << " nu " << " WaterValve3TimeOn " << WaterValve3TimeOn  << endl;
#endif // defined
        }

        if (WaterValve3 == 1)    //der er blevet tændt for vandet
        {
            WaterValve3TimeOn = time(0);
            WaterValve3On = 1;
#if defined(DebugWater)
            cout << "Der tændes for vandet (3)" << endl;
#endif // defined
            Blynk.logEvent("Watering3");
        }
    }
    ExPi_IOPortWrite(Pi_Relay_WaterValve3, WaterValve3);
}

void SetWaterValve4(bool WaterValve4)
{


#if defined(DebugWater)
    cout << "SetWaterValve4 kaldt med værdien " << WaterValve4 <<endl;
    cout << "WaterValve4On " << WaterValve4On << endl;
#endif // defined

    if (WaterValve4On != WaterValve4)    //status på vanding ændres
    {
        if (WaterValve4 == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeWaterValve4 = Stat_TimeWaterValve4 + nu - WaterValve4TimeOn;
            WaterValve4On = 0;
#if defined(DebugWater)
            cout << "Der slukkes for vandet(4) " << endl;
            cout << "+++++" << Stat_TimeWaterValve4 << " nu " << " WaterValve4TimeOn " << WaterValve3TimeOn  << endl;
#endif // defined
        }

        if (WaterValve4 == 1)    //der er blevet tændt for vandet
        {
            WaterValve4TimeOn = time(0);
            WaterValve4On = 1;
#if defined(DebugWater)
            cout << "Der tændes for vandet (4)" << endl;
#endif // defined
            Blynk.logEvent("Watering4");
        }
    }
    ExPi_IOPortWrite(Pi_Relay_WaterValve4, WaterValve4);
}




void SetHeat(bool HeatRelay)
{

    if (AfterOrangeriSetup == FALSE)
    {
        HeatOn = 0;
        return;
    }

    ExPi_IOPortWrite(Pi_Relay_Heat, HeatRelay);
    //cout << "SetHeat " << HeatRelay << " " << HeatOn << endl;
    if (HeatOn != HeatRelay)    //status på vanding ændres
    {
        if (HeatRelay == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeHeat = Stat_TimeHeat + nu - HeatTimeOn;
            HeatOn = 0;
            cout << "Der slukkes for varmen " << endl;
            cout << "+++++" << Stat_TimeHeat << " nu " << " HeatTimeOn " << HeatTimeOn  << endl;
        }

        if (HeatRelay == 1)    //der er blevet tændt for vandet
        {
            HeatTimeOn = time(0);
            HeatOn = 1;
            cout << "Der tændes for varmen" << endl;
        }
    }
}

void SetLight(bool LightRelay)
{

    if (AfterOrangeriSetup == FALSE)
    {
        return;
    }
    //Ved vintertilstand med telst kan der ikke tændes lys, da stik benyttes til Luftpumpe
    if (WinterTent == FALSE)
    {
        return;
    }

    if (LightRelay == true)
    {
        ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, 0);
    }
    else
    {
        ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, 1);
    }

    //ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, LightRelay);

    //cout << "SetLight " << LightRelay << " " << LightOn << endl;
    if (LightOn != LightRelay)    //status på vanding ændres
    {
        if (LightRelay == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeLight = Stat_TimeLight + nu - LightTimeOn;
            LightOn = 0;
            cout << "Der slukkes for lyset " << endl;
            cout << "+++++" << Stat_TimeLight << " nu " << " LightTimeOn " << LightTimeOn  << endl;
        }

        if (LightRelay == 1)    //der er blevet tændt for vandet
        {
            LightTimeOn = time(0);
            LightOn = 1;
            cout << "Der tændes for lyset" << endl;
        }
    }
}








//Opdater data til Blynk systemet
void UpdateBlynkAfterFastTimer()
{
    char msg[30];
    string str_msg;
    cout << "Start UpdateBlynkAfterFastTimer"  << endl;

    bool BlynkResult = Blynk.connected();
    cout << "Blynk.Connected " << BlynkResult << endl;
    if (BlynkResult)
    {
        try
        {


            str_msg = showDate(LastRunTime);
            cout <<  "FastTimer 0.2" << str_msg << endl;
            strcpy(msg, str_msg.c_str());
            cout << "Start Update...FastTimer 0.3 " << msg << endl;
            if (UpdateBlynkOk()) {Blynk.virtualWrite(LastRunTimeStamp_Virtual_Pin, msg);}
            //cout << "Start Update...FastTimer 0.4" << endl;



#if defined(DebugWater)
            cout << "Tidspunkt nu: " << nu << endl;
            cout << "Vanding 1   stop: "  << StopWatering1 << endl;
            cout << "Vanding 2   stop: "  << StopWatering2 << endl;
            cout << "Vanding 3   stop: "  << StopWatering3 << endl;
            cout << "Vanding 4   stop: "  << StopWatering4 << endl;
#endif // defined

            if ( nu  >= StopWatering1  & VandingEkstraMenuSelected == 2)
            {
                if (UpdateBlynkOk()) {Blynk.virtualWrite(VandingEkstra_Menu_Pin,1);} //Sætter menu til "Ingen vanding"
                if (UpdateBlynkOk()) {Blynk.virtualWrite(Vanding1Header_Virtual_Pin," ");} //Sletter label der angiver at der vandes
            }


            if ( nu  >= StopWatering2 & VandingEkstraMenuSelected == 3)
            {
                if (UpdateBlynkOk()) {Blynk.virtualWrite(VandingEkstra_Menu_Pin,1);} //Sætter menu til "Ingen vanding"
                if (UpdateBlynkOk()) {Blynk.virtualWrite(Vanding2Header_Virtual_Pin,"");}
            }

            if ( nu  >= StopWatering3 & VandingEkstraMenuSelected == 4)
            {
                if (UpdateBlynkOk()) {(VandingEkstra_Menu_Pin,1);} //Sætter menu til "Ingen vanding"
                if (UpdateBlynkOk()) {(Vanding3Header_Virtual_Pin,"");}
            }

            if ( nu  >= StopWatering4 & VandingEkstraMenuSelected == 5)
            {
                if (UpdateBlynkOk()) {(VandingEkstra_Menu_Pin,1);} //Sætter menu til "Ingen vanding"
                if (UpdateBlynkOk()) {(Vanding4Header_Virtual_Pin,"");}
            }


            if ( nu  >= StopWatering1  & VandingEkstraMenuSelected == 6)
            {
                if (UpdateBlynkOk()) {(VandingEkstra_Menu_Pin,1);} //Sætter menu til "Ingen vanding"
                if (UpdateBlynkOk()) {(Vanding1Header_Virtual_Pin," ");} //Sletter label der angiver at der vandes

            }

            cout << "Motor1_Moving " << Motor1_Moving << endl;
            if (Motor1_Moving == 0)
            {
                (WindowPct_Virtual_Pin, NewStatus_Window1);
                cout << "NewStatus_Window1 " << NewStatus_Window1 << endl;
            }
            else
            {
                Blynk.virtualWrite(WindowPct_Virtual_Pin, -1);
            }

            int TimeLeft;
            string mymsg;

            if (WaterPumpButtonActive > 0)
            {
                TimeLeft = round((WaterPumpButtonActive-time(NULL)) / 60);
            }

            if (StopVentsButtonActive > 0)
            {
                TimeLeft = round((StopVentsButtonActive-time(NULL)) / 60);
            }

            if (DispButtonActive > 0)
            {
                TimeLeft = round((DispButtonActive-time(NULL)) / 60);
            }

            if (FirstBlynkUpdate == TRUE)
            {
                //Blynk.virtualWrite(RainTable_Virtual_Pin,"clr");
                //for (int a=0; a<24; a++)
                //{  mymsg = "Time " + to_string(a);
                //	Blynk.virtualWrite(RainTable_Virtual_Pin,"add",a, mymsg.c_str(),0);}
            }

            FirstBlynkUpdate = FALSE;
            myDelay(2*blynkDelay);
            cout << "UpdateBlynk slut" << endl;
        }

        catch (...)
        {
            cout << "!!!!!  Fejl ved UpdateBlynk" << endl;
            BlynkError = BlynkError +1;
        }
    }
    cout << "Slut UpdateBlynkAfterFastTimer"  << endl;
    cout << "Blynk updates pr second/dag " << BlynkUpdateCounter << " / " << BlynkUpdateCounterDay << endl;


}

//-----------------------Funktion der styrer og måler klimaet ---------------------------
// Kaldes af timer med fast mellemrum

void ControlGreenhouseFastTimer()
{

    cout << endl;;
    cout << "********* Nyt Fast Timer ******\n";
    cout << endl;
    //cout << "AfterOrangeriSetup " << AfterOrangeriSetup << endl;
    string msg1;
    msg1 = showDate(nu);
    cout << "Fast time     " << msg1 << endl;


    if (LedPinOn == 1)
    {
        digitalWrite(LedPin, LOW);
        LedPinOn = 0;

    }
    else
    {
        digitalWrite(LedPin, HIGH);


        LedPinOn = 1;
    }

    if (WinterTent == TRUE)
        {cout << "System i vintermode med lang temperatursensor" << endl;}
    else
        {cout << "System i sommermode";}

    //Lang sensor bruges til temp-måling i telt
    if (WinterTent == TRUE)
    {
        cout << "WinterTent " << WinterTent << endl;

        cout << "Greenhouse_temperature " << Greenhouse_temperature << endl;
        //Pumpe tændes når temperaturen overstiger 18C
        if (Greenhouse_temperature > 18)
        {
            cout << "Luftpumpe tændes" << endl;
            ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, 1);
        }
        else
        {
            ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, 0);
            cout << "Luftpumpe slukkes" << endl;
        }

    }

    //msg1 = showDate(WaterPumpButtonActive);
    //cout << "WaterPumpButtonActive indtil " << msg1 << endl;

    WaterPumpButtonState =  digitalRead(Button_WaterPump_Pin);
    StopVentsButtonState =  digitalRead(Button_StopVents_Pin);
    DispButtonState =  digitalRead(Button_Disp_Pin);

#if defined(DebugWater)
    cout << "WaterPumpButtonState: " << WaterPumpButtonState << " waterbutton_PumpOn_minutes: " << waterbutton_PumpOn_minutes << endl;
#endif // defined
    cout << "StopVentsButtonState: " << StopVentsButtonState << " StopVentsButton_minutes: " << StopVentsButton_minutes << endl;
    cout << "DispButtonState: " << DispButtonState << endl;

    //cout << "WaterPumpButtonActive " << WaterPumpButtonActive << " " << nu	<< endl;
    //Hvis Pumpe skal stoppes da tid efter tryk er udløbet,
    //Hvis den er 0 er den stoppet tidligere
    if (WaterPumpButtonActive < nu && WaterPumpButtonActive > 0)
    {
        cout << "Pumpe stoppes tid er udløbet " << endl;
        //Hvis WaterPumpButtonActive ikke er igang slukkes for vandet medmindre
        //der vandes automatisk med en af de 4 ventiler
        if (Status_WaterValve1 == 0 || Status_WaterValve2 == 0 || Status_WaterValve3 == 0 || Status_WaterValve4 == 0)
        {
            WaterPumpButtonActive  = 0;
            myControlWaterPumpButton(WaterPumpButtonActive);
        }
    }

    if (StopVentsButtonActive < nu)
    {
        //cout << "Vinduer styres igen " << endl;
        StopVentsButtonActive  = 0;
        ControlStopVentsButton(StopVentsButtonActive);
    }

    if (DispButtonActive < nu)
    {
        //cout << "Disp funktion afbrydes " << endl;
        DispButtonActive  = 0;
        ControlDispButton(DispButtonActive);
    }




    //Knap for at tvinge vinduerne til at gå i stå i aktuel position
    /*if (ClosedVentsButtonActive > 0 and ClosedVentsButtonState == 1)
     {ClosedVentsButtonActive = 0;
      ClosedVentsButtonState = 0;
     } //Nulstiller knap så den ikke bruges igen til at ændre status

    if (ClosedVentsButtonActive == 0 and ClosedVentsButtonState == 1)
     {ClosedVentsButtonActive = time(NULL);}
    */
    //Tænder LED hvis vandpumpen er sat til at kører manuelt
    /*if (ClosedVentsButtonActive > 0)
    {digitalWrite(LED_Stoped_Vents_Pin, HIGH);}
      else
    {digitalWrite(LED_Stoped_Vents_Pin, LOW);}
    */

    //SetRelays(IC2Relay1, IC2Relay2,IC2Relay3,IC2Relay4,IC2Relay5,IC2Relay6,IC2Relay7,IC2Relay8);

    // cout << "Vinduer:" << endl;
    // cout << "AutoWindow "  << AutoWindow << endl;

    //Styrer hvordan vinduer skal åbnes og lukkes hvis automatik er sat
    //til og knap for StopVents ikke er aktiv
    if (AutoWindow == 1 and StopVentsButtonActive == 0)
    {
        //cout << endl;
        //cout << "Motor1_Moving " << Motor1_Moving << "\n";
        //cout << "LastRunTime " << showDate(LastRunTime) << endl;
        //cout << "WindowsChangedTime " << showDate(WindowsChangedTime) << endl;
        //cout << "MinSecBetweenMotorMoving " << MinSecBetweenMotorMoving << endl;

        cout << "Greenhouse_temperature " << Greenhouse_temperature << endl;
        cout << "Extra1_temperature " << Extra1_temperature << endl;
        cout << "Extra2_temperature " << Extra2_temperature << endl;
        //cout << "Setpoint_Udluftning "  << Setpoint_Udluftning << endl;
        //cout << "Setpoint_UdluftningTemperature " << Setpoint_UdluftningTemperature << "\n";
        //cout << "Setpoint_UdluftningTemperatureComfort " << Setpoint_UdluftningTemperatureComfort << "\n";

        //Det bestemmes om der åbnes viduer efter om der telt i orangeriet
        Temperature4Vent = Greenhouse_temperature;   //Som standard benyttes den almindelige orangeritemperatur til vinduesstyring
        if (WinterTent == TRUE)
        {
            Temperature4Vent = Extra2_temperature;
        }
        cout << "**** Temperaturstyring ****" << endl;
        cout << "Greenhouse Temperature " << Greenhouse_temperature << endl;
        cout << "Temperature4Vent " << Temperature4Vent << endl;

        if (Motor1_Moving == 0 && (LastRunTime - WindowsChangedTime > MinSecBetweenMotorMoving)) //Ændre kun indstilling hvis motor ikke allerede er i bevægelse
        {

            if (Temperature4Vent > Setpoint_Udluftning)
            {
                TmpDif = (Temperature4Vent-Setpoint_Udluftning);
                //         cout << "TmpDif " << TmpDif << endl;
                WinOpn = TmpDif * WindowOpening_P_Value;
                if (WinOpn > 100)
                {
                    WinOpn = 100;
                }
                NewStatus_Window1 = WinOpn;
                NewStatus_Window2 = WinOpn;
                NewStatus_Window3 = WinOpn;
                WindowsChangedTime =  nu;  //husker tid hvor vinduer sidst blev ændret
            }

            //Styrer hvordan vinduer skal åbnes og lukkes
            if (Temperature4Vent < Setpoint_Udluftning)
            {
                NewStatus_Window1 = 0;
                NewStatus_Window2 = 0;
                NewStatus_Window3 = 0;
                WindowsChangedTime =  nu;  //husker tid hvor vinduer sidst blev ændret
            }
        }

    }

    //cout << "Vindues status " << NewStatus_Window1 << "\n";
    //cout << "Opvarmning status " << Status_HeatUp << "\n";

    //Vinduer holdes lukket
    if (AutoWindow == 0)
    {
        NewStatus_Window1 = 0;
        NewStatus_Window2 = 0;
        NewStatus_Window3 = 0;
        NewStatus_Window4 = 0;
    }

    //Lukker for varme hvis tidsinterval er opnået og der ikke er tale om manuel styring af vandet
    if ( nu  >= StopHeating )
    {
        Status_HeatUp = 0;
    }
    else
    {
        Status_HeatUp = 1;
    }

    //ExPi_IOPortWrite(Pi_Relay_Heat, Status_HeatUp);
    SetHeat(Status_HeatUp);


    //cout << "Nu " << nu << " StopWatering1 "  << StopWatering1 << endl;

    //Lukker for vand hvis tidsinterval er opnået og der ikke er tale om manuel styring af vandet
    if ( nu  >= StopWatering1 )
    {
        Status_WaterValve1 = 0;
#if defined(DebugWater)
        cout << "Vanding slukkes ventil 1 \n";
#endif // defined
    }

    if ( nu  >= StopWatering2 )
    {
        Status_WaterValve2 = 0;
        //cout << "Vanding slukkes ventil 2 \n";
    }

    if ( nu  >= StopWatering3 )
    {
        Status_WaterValve3 = 0;
        //cout << "Vanding slukkes ventil 2 \n";
    }

    if ( nu  >= StopWatering4 )
    {
        Status_WaterValve4 = 0;
        //cout << "Vanding slukkes ventil 2 \n";
    }
    //Styrer vinduer efter behov defineret tidligere
    ControlWindows();

#if defined(DebugWater)
    cout << endl;
    cout << "=====Status vandingsventiler===== " << endl;
    cout << "StatusWaterValve1 " <<  Status_WaterValve1 << endl;
    cout << "StatusWaterValve2 " <<  Status_WaterValve2 << endl;
    cout << "StatusWaterValve3 " <<  Status_WaterValve3 << endl;
    cout << "StatusWaterValve4 " <<  Status_WaterValve4 << endl;
    cout << endl;
#endif

    SetWaterValve1(Status_WaterValve1);
    SetWaterValve2(Status_WaterValve2);
    SetWaterValve3(Status_WaterValve3);
    SetWaterValve4(Status_WaterValve4);
    SetWaterPump(Status_WaterPump);

#if defined(DebugWater)
    cout << "Status_WaterPump " << Status_WaterPump << endl;
    cout << endl;
#endif // defined

    SetFreshWatertoTank(Status_FreshWaterToTankValve);
    SetLight(Status_LightGreenhouse);

    LastRunTime =  nu;  //Variable der holder styr på hornår der er kørt en styring

    //Blynk.virtualWrite(LastRunTimeStamp_Virtual_Pin, String(LastRunTime.day())+'/' + String(LastRunTime.month())+"  kl " +
    //String(LastRunTime.hour())+':' + String(LastRunTime.minute())+':' + String(LastRunTime.second()));

    //cout << endl;;
    cout << "********* Slut Fast iTmer ******\n";
    cout << "Blynk updates pr second/dag " << BlynkUpdateCounter << " / " << BlynkUpdateCounterDay << endl;

    cout << endl;


}


float UpdateValue_Float(int pin, float NewValue, float OldValue, int Decimals)
{
    /*cout << "UodateValue_Float Pin: " << pin << " New: "  << NewValue << " Old: " << OldValue << endl;
    switch(pin) {
    case 3:   //Greenhouse_temperature_Virtural_Pin
      cout << "UpdateGreenhouse temperature " << NewValue << "  "  << OldValue << endl;
      Greenhouse_temperature_last = NewValue;
    break;
    }
*/


    if (Blynk.connected())
    {
        //cout << "UpdateValue Check Pin " << pin << ": " << New << " " << Old << endl;
        if (NewValue != OldValue || ForceBlynkUpdate)
        {
            Blynk.virtualWrite(pin, NewValue);
        }

    }
    return NewValue;
}


//Opdater data til Blynk systemet
void UpdateBlynkAfterSlowTimer()
{
    char msg[30];
    string str_msg;

    myDelay(blynkDelay);
    cout << endl;;
    cout << "********* Start UpdateBlynkAfterSlow ******" << endl;
    cout << endl;

    //bool wifiCheck = CheckInternetConnection();
    //cout << "Wifi Check " << wifiCheck << endl;

    try
    {
        if (Blynk.connected())
        {
            cout << "Efter Connected check" << endl;

            str_msg = showDate(nu);
            cout << "UpdateAfterSlow 0.1" << endl;

            strcpy(msg, str_msg.c_str());

            cout << "UpdateAfterSlow 0.2" << endl;
            Weather_temperature_last = UpdateValue_Float(Weather_temperature_Virtural_Pin, Weather_temperature, Weather_temperature_last, 1);
            db.Update_CurrentValues("Weather_temperature",Weather_temperature);
            TempAvgLastDay_last = UpdateValue_Float(TempAvgLastDay_Virtual_Pin, TempAvgLastDay, TempAvgLastDay_last, 1);
            RainHourAccum_last = UpdateValue_Float(RainAccumHour_Virtural_Pin, RainHourAccum, RainHourAccum_last, 1);
            cout << "UpdateAfterSlow 1" << endl;

            db.Update_CurrentValues("TempAvgLastDay",TempAvgLastDay);
            db.Update_CurrentValues("RainHourAccum",RainHourAccum);


            myDelay(blynkDelay);
            RainDayAccum_last = UpdateValue_Float(RainAccumDay_Virtural_Pin, RainDayAccum, RainDayAccum_last, 1);
            db.Update_CurrentValues("RainAccumDay",RainDayAccum);

            Extra1_temperature_last = UpdateValue_Float(Extra1_temperature_Virtural_Pin, Extra1_temperature, Extra1_temperature_last, 1);
            db.Update_CurrentValues("Extra1_temperature",Extra1_temperature);
            Extra2_temperature_last = UpdateValue_Float(Extra2_temperature_Virtural_Pin, Extra2_temperature, Extra2_temperature_last, 1);
            db.Update_CurrentValues("Extra2_temperature",Extra2_temperature);
            ControlBoxTemperature_last = UpdateValue_Float(ControlBoxTemperature_Virtual_Pin, ControlBoxTemperature, ControlBoxTemperature_last, 1);
            db.Update_CurrentValues("ControlBoxTemperature",ControlBoxTemperature);
            ControlBoxPressure_last = UpdateValue_Float(ControlBoxPressure_Virtual_Pin, ControlBoxPressure, ControlBoxPressure_last, 1);
            db.Update_CurrentValues("ControlBoxPressure",ControlBoxPressure);

            Setpoint_TemperatureNow_last = UpdateValue_Float(TemperatureSetpointNow_Virtual_Pin,Setpoint_TemperatureNow, Setpoint_TemperatureNow_last, 1);
            db.Update_CurrentValues("Setpoint_TemperatureNow",Setpoint_TemperatureNow);

            cout << "UpdateAfterSlow 2" << endl;

            myDelay(blynkDelay);
            cout << AirFlowValue << endl;
            if (UpdateBlynkOk()) {(AirFlowValue_Virtual_Pin,AirFlowValue);}
            db.Update_CurrentValues("AirFlowValue",AirFlowValue);

            cout << "UpdateAfterSlow 2.1" << endl;
            //Blynk.virtualWrite(TempAvgDay_Virtual_Pin,TempAvgDay);
            cout << TempAvgDay << endl;
            TempAvgDay_last = UpdateValue_Float(TempAvgDay_Virtual_Pin, TempAvgDay, TempAvgDay_last, 1);
            db.Update_CurrentValues("TempAvgDay",TempAvgDay);

            cout << "UpdateAfterSlow 2.2" << endl;
            myDelay(blynkDelay);
            //Blynk.virtualWrite(Greenhouse_temperature_Virtural_Pin, Greenhouse_temperature);
            cout << "Greenhouse temperature to Blynk "  << Greenhouse_temperature << "  " << Greenhouse_temperature_last << endl;
            Greenhouse_temperature_last = UpdateValue_Float(Greenhouse_temperature_Virtural_Pin, Greenhouse_temperature, Greenhouse_temperature_last, 1);

            db.Update_CurrentValues("Greenhouse_temperature",   Greenhouse_temperature);

            cout << "UpdateAfterSlow 3" << endl;
            cout << "Blynk updates pr second/dag " << BlynkUpdateCounter << " / " << BlynkUpdateCounterDay << endl;

            Greenhouse_humidity_last = UpdateValue_Float(Greenhouse_humidity_Virtural_Pin, Greenhouse_humidity, Greenhouse_humidity_last, 1);
            db.Update_CurrentValues("Greenhouse_humidity",   Greenhouse_humidity);

            //if (UpdateBlynkOk()) {(Greenhouse_humidity_Virtural_Pin, Greenhouse_humidity);

            //Blynk.virtualWrite(CPUTemperature_Virtual_Pin,CPUTemperature);
            CPUTemperature_last = UpdateValue_Float(CPUTemperature_Virtual_Pin, CPUTemperature, CPUTemperature_last, 1);
            db.Update_CurrentValues("CPUTemperature",CPUTemperature);

            //Opdaterer niveau for vand i palletankl
            strcpy(msg, WaterLevel1_Msg_Virtual_Pin_msg.c_str());
            if (UpdateBlynkOk()) {Blynk.virtualWrite(WaterLevel1_Msg_Virtual_Pin, msg);}

            if (SidsteVandingsLabel1 != SidsteVandingsLabel1_last)
            {
                strcpy(msg, SidsteVandingsLabel1.c_str());
                if (UpdateBlynkOk()) {Blynk.virtualWrite(SidsteVandingLabel1_Virtual_Pin, msg);}
                SidsteVandingsLabel1_last = SidsteVandingsLabel1;

            }
            cout << "UpdateAfterSlow 4" << endl;
            cout << "Blynk updates pr second/dag " << BlynkUpdateCounter << " / " << BlynkUpdateCounterDay << endl;

            myDelay(blynkDelay);
            if (SidsteVandingsLabel2 != SidsteVandingsLabel2_last)
            {
                strcpy(msg, SidsteVandingsLabel2.c_str());
                if (UpdateBlynkOk()) {Blynk.virtualWrite(SidsteVandingLabel2_Virtual_Pin, msg);}
                SidsteVandingsLabel2_last = SidsteVandingsLabel2;

            }

            if (SidsteVandingsLabel3 != SidsteVandingsLabel3_last)
            {
                strcpy(msg, SidsteVandingsLabel3.c_str());
                if (UpdateBlynkOk()) {Blynk.virtualWrite(SidsteVandingLabel3_Virtual_Pin, msg);}
                SidsteVandingsLabel3_last = SidsteVandingsLabel3;

            }
            cout << "UpdateAfterSlow 5" << endl;
            cout << "Blynk updates pr second/dag " << BlynkUpdateCounter << " / " << BlynkUpdateCounterDay << endl;

            myDelay(blynkDelay);
            if (SidsteVandingsLabel4 != SidsteVandingsLabel4_last)
            {
                strcpy(msg, SidsteVandingsLabel4.c_str());
                if (UpdateBlynkOk()) {Blynk.virtualWrite(SidsteVandingLabel4_Virtual_Pin, msg);}
                SidsteVandingsLabel4_last = SidsteVandingsLabel4;

            }


            if (UpdateBlynkOk()) {Blynk.virtualWrite(LastWateringTimeLength_Virtual_Pin,Status_VandingsTid1*60*AddExtraWaterTime);}

            //Giver status på om ventil der åbner for vandhane vand til palletank er åben
            //cout <<"Status_FreshWaterToTankValve " << Status_FreshWaterToTankValve << endl;
            if (Status_FreshWaterToTankValve == 0)
            {
                if (UpdateBlynkOk()) {Blynk.virtualWrite(AddWaterValveLabel_Virtual_Pin,"Vandhane lukket");}
            }
            else
            {
                if (UpdateBlynkOk()) {Blynk.virtualWrite(AddWaterValveLabel_Virtual_Pin,"Vandhane åben");}
            }
            Status_FreshWaterToTankValve_last = Status_FreshWaterToTankValve;

//  Blynk.virtualWrite(SidsteVandingValue2_Virtual_Pin, Status_LastWatering2);
            ForceBlynkUpdate = false; //Sikre at de ved næste BlynkUpdate opdatres værdier uanst om de er ændret eller ej
            cout << "UpdateAfterSlow 6" << endl;
            cout << "Blynk updates pr second/dag " << BlynkUpdateCounter << " / " << BlynkUpdateCounterDay << endl;

            myDelay(blynkDelay);
            int CurrentHour = round(getHour(nu));
            Rain24h[CurrentHour] = RainHourAccum;
            string mystr =  showDate(nu) + to_string(CurrentHour);
            if (UpdateBlynkOk()) {Blynk.virtualWrite(RainTable_Virtual_Pin,"update",CurrentHour, mystr.c_str(),RainHourAccum);}
            cout << "Opdaterer regnsummer" << endl;
            myDelay(blynkDelay);
            float RainSum3 = 0;
            if (CurrentHour > 2)
            {
                RainSum3 = Rain24h[CurrentHour]+Rain24h[CurrentHour-1] + Rain24h[CurrentHour-2];
            }
            if (CurrentHour == 1)
            {
                RainSum3 = Rain24h[CurrentHour]+Rain24h[CurrentHour-1] + Rain24h[23];
            }
            if (CurrentHour == 0)
            {
                RainSum3 = Rain24h[CurrentHour]+Rain24h[23] + Rain24h[22];
            }
            cout << "Rain3h " << RainSum3 << endl;
            myDelay(blynkDelay);
            if (UpdateBlynkOk()) {Blynk.virtualWrite(Rain3h_Virtual_Pin, RainSum3);}

            float RainSum6 = 0;
            if (CurrentHour >= 5)
            {
                RainSum6 = Rain24h[CurrentHour]+Rain24h[CurrentHour-1] + Rain24h[CurrentHour-2] + + Rain24h[CurrentHour- 3] + Rain24h[CurrentHour-4] + Rain24h[CurrentHour-5] ;
            }
            if (CurrentHour == 4)
            {
                RainSum6 = Rain24h[4] + Rain24h[3] + Rain24h[2] + Rain24h[1] + Rain24h[0] + Rain24h[23] ;
            }
            if (CurrentHour == 3)
            {
                RainSum6 = Rain24h[3] + Rain24h[2] + Rain24h[1] + Rain24h[0] + Rain24h[23] + Rain24h[22] ;
            }
            if (CurrentHour == 2)
            {
                RainSum6 = Rain24h[2] + Rain24h[1] + Rain24h[0] + Rain24h[23] + Rain24h[22] + Rain24h[21];
            }
            if (CurrentHour == 1)
            {
                RainSum6 = Rain24h[1] + Rain24h[0] + Rain24h[23] + Rain24h[22] + Rain24h[21] + Rain24h[20] ;
            }
            cout << "Rain6h " << RainSum6 << endl;
            if (UpdateBlynkOk()) {Blynk.virtualWrite(Rain6h_Virtual_Pin, RainSum6);}
            myDelay(blynkDelay);
            cout << "UpdateAfterSlow 8" << endl;
            cout << "Blynk updates pr second/dag " << BlynkUpdateCounter << " / " << BlynkUpdateCounterDay << endl;


            cout << endl;;
            cout << "********* Slut UpdateBlynkAfterSlow ******" << endl;
            cout << "Blynk updates pr second/dag " << BlynkUpdateCounter << " / " << BlynkUpdateCounterDay << endl;

            cout << endl;

        } //Slut try block


    }
    catch (...)
    {
        cout << "!!!!!  Fejl ved UpdateBlynk" << endl;
        BlynkError = BlynkError +1;
    }

}


ofstream DataFile;
string DataFileName = "/home/pi/Documents/Orangeri/orangeri.csv";

bool AppendDataToFile()
{
    DataFile.open(DataFileName, ios::out | ios::app);

    if (DataFile.is_open())
    {
        DataFile << nu << "," << showDate(nu) <<
                 "," << Weather_temperature <<
                 "," << Weather_humidity << "," << AirFlowValue << "," << RainHourAccum << "," << RainDayAccum <<
                 "," << ControlBoxPressure << "," << ControlBoxTemperature << "," << Greenhouse_temperature <<
                 "," << Greenhouse_humidity << "," << Extra1_temperature << "," << Extra2_temperature <<
                 "," << Stat_TimeWaterValve1 << "," << Stat_TimeWaterValve2 << "," << Stat_TimeHeat <<
                 "," << Stat_TimeFreshWaterToTank << "," << Stat_TimeLight << "," << endl;

        DataFile.close();
    }
    else cout << "Unable to open file";

    return true;
}



/*
Henter værdier gemt i xml fil efter restart af programmet. Derved sikres det at tidsafhængige
handlinger gennemføres korrekt i forhold til tidligere. Fx vanding i rigtigt interval,
beregning af midelværdier osv.
*/

void LoadXMLValues()
{

    Status_LastWatering1 = GetXMLValueDouble("/Orangeri/StatusValues/LastWatering1", Status_LastWatering1);
    //if (Status_LastWatering1 = "missing") {Status_LastWatering1 =  nu;}
    SidsteVandingsLabel1 = showDate(Status_LastWatering1);
    cout << "Status_LastWatering1 " << Status_LastWatering1 << endl;
    cout << "Status_LastWatering1 " << showDate(Status_LastWatering1) << endl;
    Status_LastWatering2 = GetXMLValueDouble("/Orangeri/StatusValues/LastWatering2", Status_LastWatering2);
    //if (Status_LastWatering2 = "missing") {Status_LastWatering2 =  nu;}
    SidsteVandingsLabel2 = showDate(Status_LastWatering2);
    cout << "Status_LastWatering2 " << Status_LastWatering2 << endl;
    cout << "Status_LastWatering2 " << showDate(Status_LastWatering2) << endl;
    Status_LastWatering3 = GetXMLValueDouble("/Orangeri/StatusValues/LastWatering3", Status_LastWatering3);
    //if (Status_LastWatering3 = "missing") {Status_LastWatering3 =  nu;}
    cout << "Status_LastWatering3 " << Status_LastWatering3 << endl;
    cout << "Status_LastWatering3 " << showDate(Status_LastWatering3) << endl;
    Status_LastWatering4 = GetXMLValueDouble("/Orangeri/StatusValues/LastWatering4", Status_LastWatering4);
    //if (Status_LastWatering4 = "missing") {Status_LastWatering4 =  nu;}
    cout << "Status_LastWatering4 " << Status_LastWatering4 << endl;
    cout << "Status_LastWatering4 " << showDate(Status_LastWatering4) << endl;

    CurrentStatus_Window1 = GetXMLValueDouble("/Orangeri/StatusValues/WindowsPct",CurrentStatus_Window1);
    cout << "CurrentStatus_Window1 " << CurrentStatus_Window1 << endl;

    Stat_TimeWaterValve1 = GetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve1",0);
    cout << "Stat_TimeWaterValve1 " << Stat_TimeWaterValve1 << endl;
    Stat_TimeWaterValve2 = GetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve2",0);
    cout << "Stat_TimeWaterValve2 " << Stat_TimeWaterValve2 << endl;
    Stat_TimeFreshWaterToTank = GetXMLValueDouble("/Orangeri/RessourceUse/FreshWaterToTank",0);
    cout << "Stat_TimeFreshWaterToTank " << Stat_TimeFreshWaterToTank << endl;
    Stat_TimeHeat = GetXMLValueDouble("/Orangeri/RessourceUse/TimeHeatOn",0);
    cout << "Stat_Heat " << Stat_TimeHeat << endl;
    Stat_TimeLight = GetXMLValueDouble("/Orangeri/RessourceUse/LightGreenhouse",0);
    cout << "Stat_Light " << Stat_TimeLight << endl;

    RainDayAccum = GetXMLValueDouble( "/Orangeri/StatisticsValues/RainDayAccum", 0);
    cout << "RainDayAccum " << RainDayAccum << endl;
    RainHourAccum = GetXMLValueDouble("/Orangeri/StatisticsValues/RainHourAccum", 0);
    cout << "RainHourAccum " << RainHourAccum << endl;

}

/*
 * Gemmer værdier løbende, så der kan laves genstart med korrekte værdier
*/

void SaveXMLValues()
{

    try
    {
        cout << "SaveXMLValues" << endl;
        bool myResult;
        myResult = myopenXMLFile();
        cout << "Efter openXMLFile: " << myResult << endl;
        SetXMLValueDouble("/Orangeri/StatusValues/LastWatering1", Status_LastWatering1);
        SetXMLValueDouble("/Orangeri/StatusValues/LastWatering2", Status_LastWatering2);
        SetXMLValueDouble("/Orangeri/StatusValues/LastWatering3", Status_LastWatering3);
        SetXMLValueDouble("/Orangeri/StatusValues/LastWatering4", Status_LastWatering4);
        SetXMLValueDouble("/Orangeri/StatusValues/WindowsPct",CurrentStatus_Window1);

        SetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve1",Stat_TimeWaterValve1);
        SetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve2",Stat_TimeWaterValve2);
        SetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve3",Stat_TimeWaterValve3);
        SetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve4",Stat_TimeWaterValve4);
        SetXMLValueDouble("/Orangeri/RessourceUse/FreshWaterToTank",Stat_TimeFreshWaterToTank);
        SetXMLValueDouble("/Orangeri/RessourceUse/TimeHeatOn",Stat_TimeHeat);
        SetXMLValueDouble("/Orangeri/RessourceUse/LightGreenhouse",Stat_TimeLight);

        SetXMLValueDouble("/Orangeri/StatisticsValues/RainDayAccum", RainDayAccum);
        SetXMLValueDouble("/Oranger i/StatisticsValues/RainHourAccum", RainHourAccum);
        myResult = mycloseXMLFile();
        cout << "SaveXMLValues afsluttet: " << myResult << endl;
    }
    catch (...)
    {

    }
}


void ControlGreenhouseVerySlowTimer()
{

    SaveXMLValues();

}

void WriteStatus()
{
    terminal.clear();
    int count;
    char buffer[100];


    char date[20];
    int Month = getMonth(time(NULL));
    int Day = getDay(time(NULL));
    int H = getHour(time(NULL));
    int M = getMinute(time(NULL));
    int S = getSecond(time(NULL));

    count = sprintf(buffer, "Opdateret: %d/%d %d:%d:&d \n",Day, Month, H, M, S );
    terminal.write(buffer);

    count = sprintf(buffer, "Varme tændt: %d \n", HeatOn);
    terminal.write(buffer);

    count = sprintf(buffer, "Udluftning med pumpe/Kunstlys: %d \n", LightOn);
    terminal.write(buffer);

    count = sprintf(buffer, "Vand til palletank: %d \n", FreshWaterToTankOn);
    terminal.write(buffer);

    count = sprintf(buffer, "Vandingsventil 1: %d \n", WaterValve1On);
    terminal.write(buffer);
    count = sprintf(buffer, "Vandingsventil 2: %d \n", WaterValve2On);
    terminal.write(buffer);
    count = sprintf(buffer, "Vandingsventil 3: %d \n", WaterValve3On);
    terminal.write(buffer);
    count = sprintf(buffer, "Vandingsventil 4: %d \n", WaterValve4On);
    terminal.write(buffer);

    //terminal.write("2. linje\n");

    terminal.flush();

}


//-----------------------Funktion der styrer og måler klimaet ---------------------------
// Kaldes af timer med fast mellemrum

void ControlGreenhouseSlowTimer()
{
    string msg1;
    string msg2;
    string msg;
    //get the current time from the clock -- one second resolution
    //nu = time(0);


    WriteStatus();


    if (LedPinOn == 1)
    {
        digitalWrite(LedPin, LOW);
        LedPinOn = 0;
    }
    else
    {
        digitalWrite(LedPin, HIGH);
        LedPinOn = 1;
    }

    cout << endl;
    cout << "********* Nyt Slow loop ******\n";
    cout << endl;

    msg1 = showDate(nu);
    msg2 = showDate(LastRunTime);
    //cout << "Slow time     " << msg1 << endl;
    //cout << "Slow lasttime " << msg2 << endl;

    //cout << "******* Sammenligner time værdi " << LastHourChanged << "   " <<  getHour(nu) << endl;
    //cout << "******* Sammenligner dag værdi " << LastDayChanged << "   " <<  getDay(nu) << endl;


    //Hvis en ny dag initialiseres tællere, summer m.m.
    if (LastDayChanged !=  getDay(nu))
    {
        cout << "Dagsværdier er forskellige \n";
        TempAvgLastDay = TempStats.GetMean(); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding
        cout << "LastDay TempStats.Average " << TempAvgLastDay << endl;
        LastDayChanged = getDay(nu);
        RainDayAccum = 0; //nulstiller regntæller
        RainDayStats.clear();
    }
    int CurrentHour;

    //En ny time startes
    if (LastHourChanged !=  getHour(nu))
    {
        cout << "Timeværdier er forskellige" << endl;
        cout << "Gemmer datafil" << endl;
        CurrentHour = round(getHour(nu));
        Rain24h[CurrentHour] = RainHourAccum;
        string mystr =  showDate(nu) + to_string(CurrentHour);
        if (Blynk.connected())
        {
            if (UpdateBlynkOk()) {Blynk.virtualWrite(RainTable_Virtual_Pin,"update",CurrentHour, mystr.c_str(),RainHourAccum);}
        }
        mystr = "Regn 24h: ";
        for( unsigned int a = 0; a < 24; a++)
        {
            mystr = mystr + ", " + to_string(Rain24h[a]);
        };
        cout << mystr << endl;
        AppendDataToFile();
        cout << "TempStat timeskift ved time " << getHour(nu) << " Gennemsnit " << endl;
        float RainSum = 0;
        cout << "Opdaterer regnsummer" << endl;
        //for( unsigned int a = CurrentHour; a = CurrentHour-3; a=a-1){
        for (int r = CurrentHour - 2; r >= CurrentHour; r++)
        {
            RainSum = RainSum + Rain24h[r];
            cout << "CurrentHour " << CurrentHour << " RainHour " << Rain24h[r] << " RainSum " << RainSum << endl;
        };
        cout << "Rain3h " << RainSum << endl;
        if (Blynk.connected())
        {
            if (UpdateBlynkOk()) {Blynk.virtualWrite(Rain3h_Virtual_Pin, RainSum);}
        }

        RainHourAccum = 0; //nulstiller regntæller
        RainHourStats.clear();
        LastHourChanged = getHour(nu);
        ForceBlynkUpdate = true; //Sikre at de ved næste BlynkUpdate opdatres værdier uanst om de er ændret eller ej
    }


    //cout << "Automatisk styring ";
    //cout << Status_AutomaticControl << "\n";


    if (Status_AutomaticControl = 1)
    {
        //Beregner aktuelt ønsket sætpunkt for temperaturen

        cout << "Automatisk kontrol \n";

        cout << "Setpoint day " << Setpoint_TemperatureDay << "\n";
        cout << "Setpoint nat " << Setpoint_TemperatureNight << "\n";
        cout << "Setpoint komfort " << Setpoint_UdluftningTemperatureComfort<< "\n";
        cout << "Væksthus temperatur " << Greenhouse_temperature << "\n";

        if (getHour(nu) >= DayStartHour && getHour(nu) <= NightStartHour)
        {
            Setpoint_TemperatureNow = Setpoint_TemperatureDay;
        }
        else
        {
            Setpoint_TemperatureNow = Setpoint_TemperatureNight;
        }
    }
    else
    {
        {
            Setpoint_TemperatureNow = Setpoint_TemperatureDay;
        }
    }

    //cout << "Setpunkt " << Setpoint_TemperatureNow << "\n";
    Error_HeatUp = Error_HeatUp + (Setpoint_TemperatureNow-Greenhouse_temperature);

    //Nedsætter fejl sum hvis temperaturen er summet for høj. Dette for at undgå at der komemr for lave temperaturer
    //Det betyder at I justering kun virker i begrænset omfang når tempearaturen er for høj.
    if (Error_HeatUp < -40)
    {
        Error_HeatUp = -50;
    }

    //cout << "Error_HeatUp  " << Error_HeatUp << "\n";

    //bestemmer om varme skal tændes eller slukkes
    if (Greenhouse_temperature < Setpoint_TemperatureNow)
    {
        // AddHeatTime = int((Setpoint_TemperatureNow-Greenhouse_temperature)*P_HeatUp + I_HeatUp/100*Error_HeatUp);
        AddHeatTime = int((Setpoint_TemperatureNow-Greenhouse_temperature)*P_HeatUp);
        StopHeating =  nu  + AddHeatTime;
        //cout << "Beregnet tid til varme " << AddHeatTime << "\n";
    }
    else
    {
        StopHeating = 0;
    }

    //cout << "AutoWindow " << AutoWindow << "\n";

    //Beregner hvilken udluftningstemperatur der benyttes
    if ((getHour(nu) >= ComfortDayStartHour) && (getHour(nu) <= ComfortDayEndHour))
    {
        Setpoint_Udluftning = Setpoint_UdluftningTemperatureComfort;
    }
    else
    {
        Setpoint_Udluftning = Setpoint_UdluftningTemperature;
    }
    cout << "Udluftningstemperatur Comfort " << Setpoint_UdluftningTemperatureComfort << "\n";
    cout << "Udluftningstemperatur Almindelig" << Setpoint_UdluftningTemperature << "\n";

    cout << "Udluftningstemperatur " << Setpoint_Udluftning << "\n";

    //read bmp sensor
    cout << "Read BME860 sensor" << endl;

    /*myBME680Result = GetBME680Data();
    std::cout << "Temperataure: " << myBME680Result.temperature << std:: endl;
    std::cout << "Humidity:     " << myBME680Result.humidity << std:: endl;
    std::cout << "Pressure:     " << myBME680Result.pressure << std:: endl;
    std::cout << "GasResistance:" << myBME680Result.gasResistance << std:: endl;

    ControlBoxTemperature = myBME680Result.temperature;
    cout << "ControlBoxTemperature " << ControlBoxTemperature << endl;
    ControlBoxPressure = myBME680Result.pressure;
    cout << "ControlBoxPressure " << ControlBoxPressure << endl;
    */


    /*AirFlowValue = ExPi_ReadADC(1);
    cout << "ADC 1 " << AirFlowValue << endl;
    AirFlowValue = ExPi_ReadADC(2);
    cout << "ADC 2 " << AirFlowValue << endl;
    AirFlowValue = ExPi_ReadADC(3);
    cout << "ADC 3 " << AirFlowValue << endl;
    AirFlowValue = ExPi_ReadADC(4);
    cout << "ADC 4 " << AirFlowValue << endl;
    AirFlowValue = ExPi_ReadADC(5);
    cout << "ADC 5 " << AirFlowValue << endl;
    AirFlowValue = ExPi_ReadADC(6);
    cout << "ADC 6 " << AirFlowValue << endl;
    AirFlowValue = ExPi_ReadADC(7);
    cout << "ADC 7 " << AirFlowValue << endl;
    AirFlowValue = ExPi_ReadADC(8);
    cout << "ADC 8 " << AirFlowValue << endl;
    */

    AirFlowValue = ExPi_ReadADC(AirFlowPin) * 12.96;
    //AirFlowValue = (ExPi_ReadADC(AirFlowPin));

    cout << "AirFlow " << AirFlowValue << endl;


    GetWaterFlow1();
    GetWaterFlow2();
    GetWaterFlow3();


    //Beregning af automatisk vanding
    //Vander kun når det er dag. Gennemfører vanding når tidsintervallet fra sidste vanding er større det
    //definerede interval.
    //typisk vandes der derfor når det bliver morgen og med tidsinterval afstande

    AddExtraWaterTime = 1;
    if (TempAvgLastDay >= 25)
    {
        AddExtraWaterTime = 1.5;
    }
    if (Greenhouse_temperature >= 27)
    {
        AddExtraWaterTime = 1.5;
    }
    if (Greenhouse_temperature >= 30)
    {
        AddExtraWaterTime = 2.0;
    }

    AddExtraWaterTime = 1;  //Sat ude af funktiion
    cout << "Væksthus gennemsnits temperatur " << TempAvgDay << endl;

    //tm*TimeStamp = localtime(&nu);
    //tm*StartVanding1 = localtime(&nu);
    //StartVanding1->tm_hour = getHour(Status_VandingStartTime1);


    tm*StartVanding2 = localtime(&nu);
    StartVanding2->tm_hour = getHour(Status_VandingStartTime2);


    // cout << "AddExtraWaterTime " << AddExtraWaterTime << "\n";
#if defined(DebugWater)
    cout << endl;
    cout << "***** Vanding opsætning *************" << endl;
    cout << endl;
    cout << "AddExtraWaterTime " << AddExtraWaterTime << "\n";
    cout << "Tid for første vanding " << Status_VandingStartTime1 << "\n";
    cout << "Tid for anden vanding " << Status_VandingStartTime2 << "\n";

    //cout << "StartVanding1 " << StartVanding1 << endl;
    //cout << "StartVanding2 " << StartVanding2 << endl;

    cout << "Sidste vanding kanal 1 "   << showDate(Status_LastWatering1) << "\n";
    cout << "Vandingstid kanal 1 " << Status_VandingsTid1 << " min" << "\n";
    cout << "Sidste vanding kanal 2 "   << showDate(Status_LastWatering2) << "\n";
    cout << "Vandingstid kanal 2 " << Status_VandingsTid2<< " min" << "\n";
    cout << "Sidste vanding kanal 3 "   << showDate(Status_LastWatering3) << "\n";
    cout << "Vandingstid kanal 3 " << Status_VandingsTid3 << " min" << "\n";
    cout << "Sidste vanding kanal 4 "   << showDate(Status_LastWatering4) << "\n";
    cout << "Vandingstid kanal 4 " << Status_VandingsTid4 << " min" << "\n";

    cout << "***** Vanding opsætning*************" << endl;
    cout << endl;
    cout << "***** Vanding Check*************" << endl;
    cout << "Hour " << getHour(nu) << endl;
    cout << "StartTime 1 " << Status_VandingStartTime1 << endl;
    cout << "Time diff 1 " << nu - Status_LastWatering1 << endl;
    cout << "StartTime 2 " << Status_VandingStartTime2 << endl;
    cout << "Time diff 2 " << nu - Status_LastWatering2 << endl;



    cout << "***** Vanding Check*************" << endl;
    cout << "" << endl;

#endif // defined

    /*
    Status_LastStartTime1 sættes når en vanding kan gennemføres pga tid.
    Vanding vil kun ske når Status_VandingsTid1 > 0
    Status_LastStartTime1 sættes ved start af program til 0, så vanding gennemføres når timen paceres,
    derefter sættes Status_LastStartTime1 til aktuel tid. Når dne er sat skal der være
    gået mere end 2 timer før den samme vandingstid kan startes igen. Sikre at der ikke vandes flere gange
    indenfor den samme tid.
    */
    if ((getHour(nu) == Status_VandingStartTime1) && ((nu - Status_LastStartTime1 > 60 * 60 * 2) || Status_LastStartTime1 == 0))
    {
#if defined(DebugWater)
        cout << "VandingsTid 1 nået Kanaler undersøges" << "\n";
#endif // defined
        Status_LastStartTime1 = nu;
        if (Status_VandingsTid1 > 0) //Er sat i Blynk som antal minutter der skal vandes
        {
            Status_WaterValve1 = 1;
            StopWatering1 =  nu  + Status_VandingsTid1*60*AddExtraWaterTime;
            Status_LastWatering1 =  nu ;
#if defined(DebugWater)
            cout << "Starter vanding 1 (første vanding) automatisk ny tid for stop " << StopWatering1 << "\n";
#endif // defined
            SidsteVandingsLabel1 = showDate(nu);
        }

        if (Status_VandingsTid2 > 0) //Er sat i Blynk som antal minutter der skal vandes
        {
            Status_WaterValve2 = 1;
            StopWatering2 =  nu  + Status_VandingsTid2*60*AddExtraWaterTime;
            Status_LastWatering2 =  nu ;
#if defined(DebugWater)
            cout << "Starter vanding 1  (første vanding) automatisk ny tid for stop " << StopWatering2 << "\n";
#endif // defined
            SidsteVandingsLabel2 = showDate(nu);
        }

        if (Status_VandingsTid3 > 0) //Er sat i Blynk som antal minutter der skal vandes
        {
            Status_WaterValve3 = 1;
            StopWatering3 =  nu  + Status_VandingsTid3*60*AddExtraWaterTime;
            Status_LastWatering3 =  nu ;
#if defined(DebugWater)
            cout << "Starter vanding 3 (første vanding)  automatisk ny tid for stop " << StopWatering3 << "\n";
#endif // defined
            SidsteVandingsLabel3 = showDate(nu);
        }

        if (Status_VandingsTid4 > 0) //Er sat i Blynk som antal minutter der skal vandes
        {
            Status_WaterValve4 = 1;
            StopWatering4 =  nu  + Status_VandingsTid4*60*AddExtraWaterTime;
            Status_LastWatering4 =  nu ;
#if defined(DebugWater)
            cout << "Starter vanding 4 (første vanding) automatisk ny tid for stop " << StopWatering4 << "\n";
#endif // defined
            SidsteVandingsLabel4 = showDate(nu);
        }
    }

    if ((getHour(nu) == Status_VandingStartTime2) && ((nu - Status_LastStartTime2 > 60 * 60 * 2) || Status_LastStartTime2 == 0))
    {
#if defined(DebugWater)
        cout << "VandingsTid 2 nået Kanaler undersøges" << "\n";
#endif // defined
        Status_LastStartTime2 = nu;
        if (Status_VandingsTid1 > 0) //Er sat i Blynk som antal minutter der skal vandes
        {
            Status_WaterValve1 = 1;
            StopWatering1 =  nu  + Status_VandingsTid1*60*AddExtraWaterTime;
            Status_LastWatering1 =  nu ;
#if defined(DebugWater)
            cout << "Starter vanding 1 (anden vanding) automatisk ny tid for stop " << StopWatering1 << "\n";
#endif // defined
            SidsteVandingsLabel1 = showDate(nu);
        }

        if (Status_VandingsTid2 > 0) //Er sat i Blynk som antal minutter der skal vandes
        {
            Status_WaterValve2 = 1;
            StopWatering2 =  nu  + Status_VandingsTid2*60*AddExtraWaterTime;
            Status_LastWatering2 =  nu ;
#if defined(DebugWater)
            cout << "Starter vanding 1 (anden vanding) automatisk ny tid for stop " << StopWatering2 << "\n";
#endif // defined
            SidsteVandingsLabel2 = showDate(nu);
        }

        if (Status_VandingsTid3 > 0) //Er sat i Blynk som antal minutter der skal vandes
        {
            Status_WaterValve3 = 1;
            StopWatering3 =  nu  + Status_VandingsTid3*60*AddExtraWaterTime;
            Status_LastWatering3 =  nu ;
#if defined(DebugWater)
            cout << "Starter vanding 3 (anden vanding) automatisk ny tid for stop " << StopWatering3 << "\n";
#endif // defined
            SidsteVandingsLabel3 = showDate(nu);
        }

        if (Status_VandingsTid4 > 0) //Er sat i Blynk som antal minutter der skal vandes
        {
            Status_WaterValve4 = 1;
            StopWatering4 =  nu  + Status_VandingsTid4*60*AddExtraWaterTime;
            Status_LastWatering4 =  nu ;
#if defined(DebugWater)
            cout << "Starter vanding 4 (anden vanding) automatisk ny tid for stop " << StopWatering4 << "\n";
#endif // defined
            SidsteVandingsLabel4 = showDate(nu);
        }
    }





    //Tænder for pumpen hvis en af ventilerne er åbne eller knap for havevanding er benyttet
    Status_WaterPump = 0;
    //jeaa ændret før sommer if (Status_WaterValve1 == 1 or Status_WaterValve2 == 1 or Status_WaterValve3 == 1 or Status_WaterValve4 == 1 or WaterPumpButtonActive > 0 )
    if (Status_WaterValve1 == 1 or Status_WaterValve2 == 1 or Status_WaterValve3 == 1 or Status_WaterValve4 == 1)
    {
        Status_WaterPump = 1;
    }

#if defined(DebugWater)
    cout << "Status_Vanding efter vurdering om vanding skal ske"  << "\n";
    cout << "Status_WaterPump " << Status_WaterPump << "\n";
    cout << "Status_WaterValve1 " << Status_WaterValve1 << "\n";
    cout << "Status_WaterValve2 " << Status_WaterValve2 << "\n";
    cout << "Status_WaterValve3 " << Status_WaterValve3 << "\n";
    cout << "Status_WaterValve4 " << Status_WaterValve4 << "\n";
#endif // defined

    //cout << "*** Vandniveauer ***\n";

    //Læser værdier på flyder i palletank.
    //Flyderen læses med værdier i WaterLevel1 og WaterLevel2
    cout << "Læsning af WaterLevel" << endl;
    WaterReadTop = digitalRead(WaterLevelPin1);
    WaterReadButtom = digitalRead(WaterLevelPin2);
    cout << "Vandniveau 1  Øverste flyder " << Status_WaterLevel1 << " måling " << WaterReadTop << endl;
    cout << "Vandniveau 2  Nederste flyder " << Status_WaterLevel2 << " måling " << WaterReadButtom << endl;

    //Hvis flyderen er løftet op måles høj værdi på A8 eller er A8 = 0
    //cout << "Inden WaterReadButtom == 1" << endl;
    if (WaterReadButtom == 1)
    {
        Status_WaterLevel1 = false;
        Status_FreshWaterToTankValve = 1; // åbner ventil der tilfører vand til vandtank
        WaterLevel1_Msg_Virtual_Pin_msg = "Palletank er snart tom";
        cout << "Palletank er snart tom" << endl;
    }
    //cout << "Inden WaterReadTop == 1" << endl;
    if (WaterReadTop == 1)
    {
        Status_WaterLevel1 = true;
        Status_FreshWaterToTankValve = 0;  //Ikke behov for mere vand til palletank da beholderen er fyldt så flyderen er oppe
        WaterLevel1_Msg_Virtual_Pin_msg = "Palletank er fyldt til flyder niveau";
    }

    //cout << "Inden RainRead" << endl;

    //cout << "Lysniveau " << LightValue << "\n";
    // Rain get start state
    double RainRead = getUnitRain();
    RainDayAccum = RainDayAccum + RainRead;
    RainHourAccum = RainHourAccum + RainRead;

    //cout << "Regn Day  " << RainDayAccum << "\n";
    //cout << "Regn hour " << RainHourAccum << "\n";

    //CPU temperatur læses
    try
    {
        CPUTemperature = getCPUTemperature();
        // Læsning af temperaturer på one-wire
        Weather_temperature = Probe06.getTemp();

        //int rtn = pi_2_dht_read(&Weather_temperature, &Weather_humidity);

        //int rtn = gettempandhumidity(Weather_sht31file, &Weather_temperature, &Weather_humidity);
        //cout << "WeatherRead_1 result " << rtn << endl;

        //inbt rtn = pi_2_dht_read(DHT22, 16, &Weather_humidity, &Weather_temperature);
        //cout << "WeatherRead result " << rtn << endl;
        //cout << "Weather temperature " << Weather_temperature << endl;
        //cout << "Weather humidity " << Weather_humidity << endl;//printtempandhumidity(SHT31file);

        Greenhouse_temperature = Probe02.getTemp();  //Orange
        //Greenhouse_temperature = Probe07.getTemp();
        Extra1_temperature = Probe04.getTemp();
        Extra2_temperature = Probe07.getTemp();
        cout << "Greenhouse temperature " << Greenhouse_temperature << endl;
        cout << "Extra1 temperature " << Extra1_temperature << endl;
        cout << "Extra2 temperature " << Extra2_temperature << endl;

    }
    catch (const std::out_of_range& oor)
    {
        cout << "ReadSensor - Out of Range error: " << oor.what() << endl;
    }

    catch (const std::runtime_error& oor)
    {
        cout << "ReadSensor -  Out of Range error: " << oor.what() << endl;
    }

    catch (...)
    {
        cout << "ReadSensor - Fejl ved ReadSensor " << endl;
    }

    //if (Greenhouse_temperature )
    //Greenhouse_temperature = ControlBoxTemperature;
    cout << "Weather teamperature" << Weather_temperature << endl;
    cout << "Weather humidity " << Weather_humidity << endl;
    cout << "OneWire temperaturer målestation er: ";
    cout << "Greenhouse " << Greenhouse_temperature << endl;
    cout << "Extra1 " <<  Extra1_temperature << endl;
    cout << "Extra2 " <<  Extra2_temperature << endl;


    // Disable the interrupt while calculating flow rate and sending the value to the host

    //DEBUG -----Jeaa slettet af hensyn til fejlfinding på regnsensor
    //detachInterrupt(digitalPinToInterrupt(Water_Flow_Pin));

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    WaterFlowRate = ((1000.0 / (millis() - oldTime)) * WaterFlowPulseCount) / calibrationFactor;

    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    WaterFlowMililitres = (WaterFlowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    WaterTotalMililitres += WaterFlowMililitres;

    unsigned int frac;

    /*// Print the flow rate for this second in litres / minute
    cout << "Flow rate: " << (int(WaterFlowRate));  // Print the integer part of the variable
    cout << "L/min";
    cout << "\t" << "\n";       // Print tab space

    // Print the cumulative total of litres flowed since starting
    cout << "Output Liquid Quantity: ";
    cout << WaterTotalMililitres;
    cout << "mL" << "\n";
    cout << WaterTotalMililitres/1000;
    cout << "L" << "\n";
    cout << "\n";*/

    // Reset the pulse counter so we can start incrementing again
    WaterFlowPulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    //-----Jeaa slettet af hensyn til fejlfinding på regnsensor
    //DEBUG attachInterrupt(digitalPinToInterrupt(Water_Flow_Pin), WaterFlowClick, FALLING);

    //cout << "Temp der laves stat på 1 " << Greenhouse_temperature << "\n";
    //cout << "Temp der laves stat på 2 " << Greenhouse_temperature << "\n";

    //if (isnan(Greenhouse_temperature) == false)  //jeaa
    TempStats.AddData(Greenhouse_temperature);

    //cout << "Væksthus temperatur " << Greenhouse_temperature << "\n";
    //cout << "TempStat efter add " << " Gennemsnit " << TempStats.GetMean() << "\n";

    //RainDayStats.AddData(RainDayAccum);
    //RainHourStats.AddData(RainHourAccum);


    //Sætter relæer efter status værdier der er beregnet eller overstyret i app
    //cout << "Klar til at sætte relæer" << endl;
    //cout << "3 Status_FreshWaterToTankValve " << Status_FreshWaterToTankValve << endl;


    SetHeat(Status_HeatUp);
    SetLight(Status_LightGreenhouse);
    SetFreshWatertoTank(Status_FreshWaterToTankValve);

    TempAvgDay = TempStats.GetMean();

    cout << "Status_FreshWaterToTankValve " << Status_FreshWaterToTankValve << endl;
    cout << "Status_WaterPump " << Status_WaterPump  << endl;
    //cout << "Status_WaterValve1 " << Status_WaterValve1 << endl;
    //cout << "Status_WaterValve2 " << Status_WaterValve2 << endl;

    LastRunTime =  nu;  //Variable der holder styr på hornår der er kørt en styring

    cout << endl;
    cout << "********* Slut Slow loop ******\n";
    cout << endl;

    SetupFinished = TRUE;  //problmer med at interrupt mm starter for tidligt derfor sættes denne til false ved start og true efter første runde her

}

bool CheckIOPortError(void)
{
    string msg;

    if (IOPortReadError > 0 || IOPortSetupError > 0 || IOPortWriteError > 0)
    {
        //ExpanderPiSetup;
    }

    if (IOPortSetupError > 0)
    {
        emailSubject = "Orangeri: IOPortSetupError ";
        msg = showDate(nu) + " IOPortSetupError antal " + std::to_string(IOPortSetupError);
        emailMessage = msg.c_str();
        IOPortSetupError = 0;
    }

    if (IOPortReadError > 0)
    {
        emailSubject = "Orangeri: IOPortReadError ";
        msg = showDate(nu) + " IOPortReadError antal " + std::to_string(IOPortReadError);
        emailMessage = msg.c_str();
        IOPortReadError = 0;
    }

    if (IOPortWriteError > 0)
    {
        emailSubject = "Orangeri: IOPortWriteError ";
        msg = showDate(nu) + " IOPortWriteError antal " + std::to_string(IOPortWriteError);
    }

    if (SetADCError > 0)
    {
        emailSubject = "Orangeri: SetADCError ";
        msg = showDate(nu) + " SetADCError antal " + std::to_string(SetADCError);
        SetADCError = 0;
    }

    if (ReadADCError > 0)
    {
        emailSubject = "Orangeri: ReadADCError ";
        msg = showDate(nu) + " ReadADCError antal " + std::to_string(ReadADCError);
        ReadADCError = 0;
    }

}

//Kontrollerer forbindelsen til Blynk. Genopretter forbindelse hvis den er mistet.
//Kaldes af timer med mellemrum for at opdaterer forbindelsen om nødvendigt hvis den mistes af forskellige grunde

bool CheckBlynk(void)
{

    bool myresult = Blynk.connected();

    if (myresult)
    {
        BlynkResetCount = 0;
        cout << "Check Blynk - Conencted" << endl;
    }

    if (!myresult)
    {
        cout << "Blynk ikke forbundet" << endl;
        myresult = Blynk.connect(15);  // 1000 is a timeout of 3333 milliseconds

        if (myresult)
        {
            cout << "Blynk reconnected pga CheckBlynk" << endl;
            BlynkResetCount = 0;
        }
        else
        {
            BlynkResetCount = BlynkResetCount + 1;
            if (BlynkResetCount > MaxBlynkResetCount)
            {
            }
        }
    }

    cout << "BlynkResetCount " << BlynkResetCount << endl;

    if (BlynkResetCount = 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void FastTimerEvent()
{
    nu = time(NULL);

    ControlGreenhouseFastTimer();
    UpdateBlynkAfterFastTimer();  //Opdater data til Blynk}
    LastFastTimerEvent = nu;
}

void SlowTimerEvent()
{
    nu = time(NULL);
    //CheckBlynk();
    ControlGreenhouseSlowTimer();
    UpdateBlynkAfterSlowTimer();
    LastSlowTimerEvent = nu;

    if (AfterOrangeriSetup== FALSE)
    {
        WaterPumpButtonActive = 0;
        StopVentsButtonActive = 0;
        DispButtonActive = 0;
        Status_HeatUp = 0;
        StopHeating = 0;
        AfterOrangeriSetup= TRUE;
    }

}

void VerySlowTimerEvent()
{
    nu = time(NULL);
    CheckBlynk();
    ControlGreenhouseVerySlowTimer();
}


//---------------------------------Setup rutine --------------------------------





void OrangeriSetup(void)
{

    cout << "FileCount 2.1 "  << getOpenFileCount() << endl;
    AfterOrangeriSetup = FALSE;

    ExpanderPiSetup();

    //slukker for pumpe jhvis den skulle være igang
    Status_WaterPump = 0;
    ExPi_IOPortWrite(Pi_Relay_WaterPump, Status_WaterPump);

    nu = time(NULL);

    //Weather_shtfile = sht31_open(sht31_open(SHT31_INTERFACE_ADDR,SHT31_DEFAULT_ADDR));
    CPUTemperature = getCPUTemperature();

    pinMode(WaterLevelPin1, INPUT);

    //gik godt her til. Skal ændres
    //pinMode(WaterLevelPin2, INPUT);

    enableLog = 0;   //Sættes tilæ 1 når data skal gemmes i log ellers til 0
    emptyLogNow = 0; //Sættes til 1 når log skal tømmes for data

    //Interrupt porte benytter standard porte på GPIO proten og ikke på ExpanderPi
    rain_isp_Setup(RAIN_GAUGE_PIN); //Sætter Pin 15 til at være input for interrupt giver (regnmåler)
    //isp2_Setup(17); //Sætter Pin 17 BAM til at være input for interrupt giver ()

    //Button_WaterPump_Pin

    //pinMode (Button_WaterPump_Pin, INPUT) ;
    waterbutton_isp_Setup(Button_WaterPump_Pin);
    StopVentsButton_isp_Setup(Button_StopVents_Pin);
    DispButton_isp_Setup(Button_Disp_Pin);


    //Opsætning af wand flow målinger
    wiringPiISR (WaterFlow1_PIN, INT_EDGE_FALLING, &WaterFlow1Interrupt);
    wiringPiISR (WaterFlow2_PIN, INT_EDGE_FALLING, &WaterFlow2Interrupt);
    wiringPiISR (WaterFlow3_PIN, INT_EDGE_FALLING, &WaterFlow3Interrupt);

    //InitBME680();  //Init BME680 sensor

    pinMode (LedPin, OUTPUT) ;
    //pinMode (LED_WaterPumpButton_Pin, OUTPUT);
    //pinMode (LED_StopVentsButton_Pin, OUTPUT);
    //pinMode (LED_DispButton_Pin, OUTPUT) ;
    //digitalWrite(LED_WaterPumpButton_Pin, LOW);
    //digitalWrite(LED_StopVentsButton_Pin, LOW);
    //digitalWrite(LED_DispButton_Pin, LOW);
    digitalWrite(LedPin, LOW);

    LastSetupRunTime =  nu;   //Holder sidste gang der blev kørt setup rutine. Altså upload eller wtd reset
    LastRunTime  =  nu;    //Variable der holder styr på hornår der er kørt en styring
    LastFastTimerEvent = nu;
    LastSlowTimerEvent = nu;
    LastVerySlowTimerEvent = nu;
    LastScreenUpdate = nu;

//------------------Relæ output ----------------------------------

    //opsætning af statistik variable
    TempStats.clear(); //explicitly start clean
    RainDayStats.clear(); //explicitly start clean
    RainHourStats.clear(); //explicitly start clean
    RainDayAccum = 0; //nulstiller regntæller
    RainHourAccum = 0; //nulstilelr regntæller

    //jeaa definer porte ind og ud

    //clean table at start
    // Blynk.virtualWrite(Table_Virtual_Pin, "clr");
    // RowIndex = ++RowIndex;
    // Blynk.virtualWrite(Table_Virtual_Pin, "add", RowIndex, "Dato "+ String( nu.day())+'/' + String( nu.month()) +'-' + String( nu.year()), 0);



    //hente gemte værdier for statistik, status m.m.
    //sker først efter setup er gennemført således at de gemte værdier benyttes i stedet for.,
    //createXML(); //når file er ødelagt


    // Blynk.syncAll();
    // Blynk.syncVirtual(V19,V50);

    LoadXMLValues();


    //lukker alle vinduer ved opstart
    //Hvis CloseWindowOnStart = 1 køres denne lukning af vinduer ellers springes det over. Megdfører at vinduerne ikke er
    //placeret korrekt i forhold til % åbne.Derfor kun til debug hvor programmet geninstalleres mange gange.
    if (CloseWindowsOnStart == 1)
    {
        cout << "Close Windows on start" << endl;
        //Blynk.virtualWrite(Window1_Virtual_Pin, 0);
        CurrentStatus_Window1 = 100;
        NewStatus_Window1 = 0;
        CurrentStatus_Window2 = 100;
        NewStatus_Window2 = 0;
        CurrentStatus_Window3 = 100;
        NewStatus_Window3 = 0;
        CurrentStatus_Window4 = 100;
        NewStatus_Window4 = 0;
        ControlWindows;
    }

    if (CloseWindowsOnStart == 0)
    {
        cout << "Vinduer lukkes ikke ved start. Gemt åbning: " << CurrentStatus_Window1 << endl;
        CurrentStatus_Window1 = CurrentStatus_Window1;
        CurrentStatus_Window2 = CurrentStatus_Window1;
        CurrentStatus_Window3 = CurrentStatus_Window1;
        CurrentStatus_Window4 = CurrentStatus_Window1;
        NewStatus_Window1 = CurrentStatus_Window1;
        NewStatus_Window2 = CurrentStatus_Window1;
        NewStatus_Window3 = CurrentStatus_Window1;
        NewStatus_Window4 = CurrentStatus_Window1;
    }

    LastRunTime =  0;  //Sikre at måling køres med dert samme


    // Setup a function to be called every second
//const int FastTimerInterval = 5; //Angiver antal sekunder mellem udførsel af FastTimer funktionen
//const int SlowTimerInterval = 30; //Angiver antal sekunder mellem udførsel af SlowTimer funktionen
//const int VerySlowTimerInterval = 600; //Angiver antal sekunder mellem udførsel af SlowTimer funktionen

//const long FastTimerIntervalLong = 5000L; //Angiver antal sekunder mellem udførsel af FastTimer funktionen
        char msg[30];
        string str_msg = showDate(LastSetupRunTime);
        //cout << "Start Update...FastTimer 0.5" << endl;
        strcpy(msg, str_msg.c_str());
        //cout << "Start Update...FastTimer 0.6" << endl;
        Blynk.virtualWrite(LastSetupRunTimeStamp_Virtual_Pin, msg);
        //cout << "UpdateBlynkAfterFastTimer 1" << endl;

    OrangeriTimer.setInterval(15000L, FastTimerEvent);
    OrangeriTimer.setInterval(60000L, SlowTimerEvent);
    OrangeriTimer.setInterval(120000L, VerySlowTimerEvent);

    cout << "------------- OrangeriSetup Slut --------------" << endl;
    cout << endl;



}

/// Blynk "gp" numbers are BCM numbers, so gp17 is physical pin 11
//#define BLYNK_DEBUG

#include <Orangeri_lib.h>   //Egne rutiner til styring af orangeri

//#include <bmp180.h>         //Læsning af barometertryk på Bosch kort

#include <pi_2_dht_read.h>  //Læsning af DHT sensorer DHT11/DHT22/AM2302
#include <pi_2_mmio.h>      //Benyttes som ovenover til læsning af DHT sensorer

#include <DS18B20.h>        //Læsning af temperatur på one wire forbindelse

//Tidsstyring af processer i programmet. I stedet for en egentlig timer funktion
//const int FastTimerInterval = 5; //Angiver antal sekunder mellem udførsel af FastTimer funktionen
//const int SlowTimerInterval = 30; //Angiver antal sekunder mellem udførsel af SlowTimer funktionen
//const int VerySlowTimerInterval = 600; //Angiver antal sekunder mellem udførsel af SlowTimer funktionen
//const long FastTimerIntervalLong = 5000L; //Angiver antal sekunder mellem udførsel af FastTimer funktionen

void RelayTest()
{

    cout << "Motorstyring aktiv. Dir høj" << endl;
    ExPi_IOPortWrite(Pi_Relay_Motor1_pwm, 1);
    ExPi_IOPortWrite(Pi_Relay_Motor1_dir, 1);
    delay(5000);
    cout << "Ændre retning Dir LAv" << endl;
    ExPi_IOPortWrite(Pi_Relay_Motor1_dir, 0);
    delay(5000);
    ExPi_IOPortWrite(Pi_Relay_Motor1_pwm, 1);
    cout << "Mototsryring stoppet" << endl;

    int r;

    cout <<"Nulstilling" << endl;
    delay(2000);
    r = 1;
    do
    {
        ExPi_IOPortWrite(r, 0);
        r = r+1;
    }
    while (r < 14);

    r = 1;
    do
    {
        if (r > 1)
        {
            ExPi_IOPortWrite(r-1, 0);
        }
        cout << "Relæ nr " << r << endl;
        ExPi_IOPortWrite(r, 1);
        delay(2000);
        r = r+1;
    }
    while (r < 14);

}

void loop()
{
    Blynk.run();
    OrangeriTimer.run();

}


int main(int argc, char* argv[])
{


    std::string sqlError;

    if (!db.IniSqlConnection(sqlError)) {
        std::cerr << "Failed to connect to database: " << sqlError << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Connected to database successfully!" << std::endl;

/*
    crow::SimpleApp app; //define your crow application
    //define your endpoint at the root directory

    CROW_ROUTE(app, "/data")
    ([&db, &dbMutex]() {
        std::lock_guard<std::mutex> lock(dbMutex); // Ensure thread safety
        //std::string query = "SELECT timestamp, temperature, humidity FROM climate_data;";
        //return db.queryDatabase(query);
        return "Hello world";
    });
    app.port(18080).multithreaded().run();
*/

        //createXML();  //Bruges kun til at oprette en tom xml fil struktur
        const char *auth, *serv;
        uint16_t port;
        parse_options(argc, argv, auth, serv, port);
        Blynk.begin(auth, serv, port);
        cout << "Inden OrangeriSetup" << endl;
        OrangeriSetup();

        if (UpdateBlynkOk()) {Blynk.virtualWrite(Terminal_Virtual_Pin, "Efter Orangeri Setup");}

        while(true)
        {
            loop();
        }

        return 0;



}



