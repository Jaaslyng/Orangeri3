/*Noter om udvikling

Styring af lys er ikke i9mplementeret med start og slut tider

*/

//#define DebugWater   //Debug af vandning og friskvand
#define RASPBERRY
#define LINUX
#include <chrono>
#include "Database.h"
#include <string>
//#include <bme680_jeaa.h>
#include <mariadb/conncpp.hpp>
#include <atomic>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"  // For logging to a file
#include "spdlog/sinks/rotating_file_sink.h" // support for rotating file logging
#include <fmt/chrono.h>

#include <ctime>
#include <iomanip>
#include <sstream>
//#include <pigpio.h>   //Pulse counting
#include <pigpiod_if2.h>


#include <csignal>     //Handling Terminal Closure

//auto logger = spdlog::logger("Orangei_logger", spdlog::sinks::file_sink("OrangeriLog.txt", 1));
bool AfterOrangeriSetup = false; //Sættes til Ture når setup er gennemført
bool FirstBlynkUpdate = true;  //Sættes til FALSE efter første update. Bruges til at nulstille tabel m.m.



Database db;

using namespace std;

#include <iostream>
//#include <Orangeri_lib.h>
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




static std::shared_ptr<spdlog::logger> logger;
void setupLogger()
{
// Create a logger with the name "example_logger"
    try
    {
        // create a file rotating logger with 5mb size max and 3 rotated files
        logger = spdlog::rotating_logger_mt("file_logger", "/home/jesper/Documents/Orangeri3/Orangeri/log/OrangeriLogs.txt", 1024 * 1024 * 2, 3);
        // Set the log format: "[timestamp] [log level] message"
        logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

        // Set automatic flushing for INFO and above
        //logger->flush_on(spdlog::level::info);  //kan belaste systemet for voldsomt

        // Brug kun periodic flush
        spdlog::flush_every(std::chrono::seconds(10));

        // Set this logger as the default logger (optional, but useful)
        spdlog::set_default_logger(logger);

        spdlog::set_level(spdlog::level::info);   //- info, warn, error, critical bliver logget
        //- debug og trace bliver ignoreret
        //spdlog::set_level(spdlog::level::debug);//- info, warn, error, critical, debug bliver logget
        //spdlog::set_level(spdlog::level::trace;//- info, warn, error, critical, debug, trace bliver logget

    }
    catch (const spdlog::spdlog_ex &ex)
    {
        // 5️⃣ Handle errors if logging fails
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }

}





//XMLNAme defineres 2 steder. Her og i myXML.cpp
char* XMLName = "/home/orangeri/Documents/Orangeri2/Orangeri/Orangeri.xml";



/* forbinder til pigpiod daemonen*/
int pi;

int BlynkError = 0; //Tæller der holder antal  fejl registreret
bool ForceBlynkUpdate = true; //Hvis den er true opdateres blynk værdier uanset om de e rændret eller ikke er
bool SetupFinished = false; //sættes til true efter at orangesetup er gennemført
int Weather_sht31file; //Filnummer svarende til SHT31
int delayBeforeIsp = 10; //delay der benyttes til at tjekke om det er støj eller en ændring der trigger interrupt
int blynkDelay = 0;  //d0elay der benyttes for at undgå at det uploades for mange værdier til blynk
//BME680Results  myBME680Result; //Struct with data

//holde styr på om udluftning sker efter extra temp sensor eller den almindelige
bool WinterTent = true;
//Nyt ønske om vinduesåbning samt status på den nuværende åbning
double NewStatus_Window1;
double NewStatus_Window2;
double NewStatus_Window3;
double NewStatus_Window4;
double CurrentStatus_Window1;
double CurrentStatus_Window2;
double CurrentStatus_Window3;
double CurrentStatus_Window4;
double TmpDif;
double WinOpn;

time_t StopMotor1 = 0;
time_t StopMotor2 = 0;
time_t StopMotor3 = 0;
time_t StopMotor4 = 0;

bool Motor1_Moving;
bool Motor2_Moving;
bool Motor3_Moving;
bool Motor4_Moving;

time_t WindowsChangedTime; //holder styr på hvornår der sidst blev styret vinduer
bool isFirstConnect = true; //er true indtil initialisering og sync med Blynk er gennemført

//1: lukker vinduer ved upload af nyt program 0: vinduer ændres ikke ved start
//0: læser værdi i xml-fil
bool CloseWindowsOnStart = 1;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

time_t LastRunTime = 0;
time_t NextRunTime = 0;
time_t nu = 0;
time_t LastSetupRunTime= 0;//husker sidste tid der er kørt setup rutine
int LastDayChanged = 0;     //Holder styr på den sidte dag der blev iwindow      nitialiseret pga skift af dag
int LastHourChanged = 0;    //Som ovenfor, men for time værdi.
bool DateInitialized = false; //bruges til at initialiserer diverse variable ved start af ny dag, nulstilling af tællere etc

// Debounce tracking for each button
volatile uint32_t lastInterruptTime[3] = {0};
const uint32_t debounceTime = 200000; // 200ms debounce

time_t StopVentButtonAtTime = 0;  //Tid hvor StopVent afbrydes
bool StopVentButtonActive = 0;  //angiver om StopVentButton er afktiv

time_t WaterPumpButtonAtTime = 0;
bool WaterPumpButtonActive = 0;
int waterPumpButton_Minutes = 30;

//String der løbende benyttes til fejlbeskeder der skal smales til tekst
char ErrStr[200];
//int myBlynkDelay = 0; //Delay der skal sikre at Blynk data ikke overføres for hutigt til server

int TimeForWindowToOpen = 50; //sekunder til at åbne vinduerne helt
int MinSecBetweenMotorMoving = 300; //mindste tid i sekunder mellem at motorerne bevæger sig
int WindowTimerStepInPct = 10; //jeaa ændret beregner hvormange sekunder der rykkes og hvad det svarer til i % åbning/lukning
int WindowOpening_P_Value = 16; //Faktor der ganges med temperatur-fejlen og medfører en ændring i vinduernes åbning på det antal procent

//int Status_HeatUp = 0;
int Status_LightGreenhouse = 0;
int Status_FreshWaterToTankValve = 0;
//int Status_FreshWaterToTankValve_last = 0;

int Status_WaterValve1 = 0;
int Status_WaterValve2 = 0;
int Status_WaterValve3 = 0;
int Status_WaterValve4 = 0;

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

int HeatOn = 0;  //Status på om der opvarmes eller ikke
time_t Stat_TimeHeat;   //Samler sum over opvarmning. Sum gemmes i xml fil og kører indtild er manuelt nulstilles
time_t HeatTimeOn; //holder tid hvor en opvarmning startes, så der kan beregnes tid

int LightOn = 0;        //Status på om der vandes eller ikke vandes
time_t Stat_TimeLight;  //Samler sum over vandingstig. Sum gemmes i xml fil og kører indtild er manuelt nulstilles
time_t LightTimeOn;     //holder tid hvor en vanding startes, så der kan beregnes vandingstid

//int Status_AddWaterValve =  nu ;
//int Status_AddFreshWaterValve = 0; //Holder status på tilstand af ventil, der åbner for vandhanevand til palletank.
int Status_WaterManual1 = 0;   //sættes til 1 når vanding kører manuel ellers 1
int Status_WaterManual2 = 0;   //sættes til 1 når vanding kører manuel ellers 1
int Status_WaterManual3 = 0;   //sættes til 1 når vanding kører manuel ellers 1
int Status_WaterManual4 = 0;   //sættes til 1 når vanding kører manuel ellers 1

int LedPinOn = 0;

double CPUTemperature;
double CPUTemperature_last;

//Temperatur hvor over der begynder at blvie luftet
double Setpoint_UdluftningTemperatureComfort = 20;
double Setpoint_UdluftningTemperature = 20;
double Setpoint_TemperatureDay = 20;
double Setpoint_TemperatureNight = 10;

double Setpoint_Udluftning = Setpoint_UdluftningTemperature; //Holder udluftningstemperaturen der bestemmes efter komfort eller ikke komfort
double Setpoint_MaxTemperature = 30;   //Max tilladt temperatur før luftning
double Setpoint_TemperatureNow = 25;   //Temperatur der styres efter lige nu. Beregnet på baggrund af dag/dat m.m.

double Error_HeatUp = 0; //Variabel der holder accumuleret fejl ved beregning
int LagTime_HeatUp = 0;  //Sekunder fra varmeblaser reelt varmer efter tanding
double P_HeatUp = 30; //P-værdie der ganges med temp forskel for at definerer antal sekunder der skal varmes op for at holde temperature
double I_HeatUp = 0; //I-værdi der benyttes ved at kigge på accumuleret tempearaturfejl
int AddHeatTime;     //holder værdi der benyttes ved beregning af hvor lang tid der skal tilføres varme i PI beregningen

//Justering af vanding efter høj temperatur
//double TempAvgLastDay = 20;
double GreenhouseTempAvgDay = 20;
double GreenhouseTempMinDay = 20;
double GreenhouseTempMaxDay = 20;
//double TempAvgLastHour = 20;
double GreenhouseTempAvgHour = 20;
//double WeatherTempAvgLastDay = 20;
double WeatherTempAvgDay = 20;
double WeatherTempMinDay = 20;
double WeatherTempMaxDay = 20;
//double WeatherTempAvgLastHour = 20;
double WeatherTempAvgHour = 20;

double AddExtraWaterTime_Greenhouse = 1.0; //faktor der ganges med for at tilfører ekstra vand ved høje temperaturer dagen før eller den aktuelle temperatur
double AddExtraWaterTime_Garden = 1.0; //faktor der ganges med for at tilfører ekstra vand ved høje temperaturer dagen før eller den aktuelle temperatur


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


//Setpoint tabel definitions
//Tekster der benyttes i setpoint tabel defineres her

//Setpoints
string WateringControl_SetpointText = "VandingsKontrol";
string WindowControl_SetpointText = "VinduesKontrol";
//CurrentValues

string LastWateringTime1_Parameter = "SidsteVandingsTime1Label";
string LastWateringTime2_Parameter = "SidsteVandingsTime2Label";


string LastWatering1_Parameter = "SidsteVandingLabel1";
string LastWatering2_Parameter = "SidsteVandingLabel2";
string LastWatering3_Parameter = "SidsteVandingLabel3";
string LastWatering4_Parameter = "SidsteVandingLabel4";
string WindowsPct_Parameter = "WindowsPct";
string WaterFlow1_Total_Parameter = "WaterFlow1_Total";
string WaterFlow1_Parameter = "WaterFlow1";
string WaterFlow2_Total_Parameter = "WaterFlow2_Total";
string WaterFlow2_Parameter = "WaterFlow2";
string WaterFlow3_Total_Parameter = "WaterFlow3_Total";
string WaterFlow3_Parameter = "WaterFlow3";
string RainAccumDay_Parameter = "RainAccumDay";
string RainAccumHour_Parameter = "RainHourAccum";
string RainSum3h_Parameter = "Rain3h";
string RainSum6h_Parameter = "Rain6h";
string RainSum24h_Parameter = "Rain24h";

//(HistoricValues
string GreenhouseTempAvgDay_Text = "GreenhouseTemperatureDaily";
string GreenhouseTempMinDay_Text = "GreenhouseMinimumTemperatureDaily";
string GreenhouseTempMaxDay_Text = "GreenhouseMaximumTemperatureDaily";
string WeatherTempAvgDay_Text = "WeatherTemperatureDaily";
string WeatherTempMinDay_Text = "WeatherMinimumTemperatureDaily";
string WeatherTempMaxDay_Text = "WeatherMaximumTemperatureDaily";
string RainSumDay_Text = "RainSumDaily";
string TempAvgHour_Text = "GreenhouseTemperatureHour";
string WeatherTempAvgHour_Text = "WeatherTemperatureHour";
string RainSumHour_Text = "RainSumHour";
string WaterFlow1Day_Text = "WaterFlow1Daily";
string WaterFlow2Day_Text = "WaterFlow2Daily";
string WaterFlow3Day_Text = "WaterFlow3Daily";


string HeatTimeOn_Text = "HeatOnTime";
string FreshWaterToTankTime_Text = "FreshWaterTime";
string WaterValve1Time_Text = "WaterValve1Time";
string WaterValve2Time_Text = "WaterValve2Time";
string WaterValve3Time_Text = "WaterValve3Time";
string WaterValve4Time_Text = "WaterValve4Time";


//digitale input


//digitale output
//#define LED_WaterPumpButton_Pin 13
//#define LED_StopVentsButton_Pin 19
//#define LED_DispButton_Pin 26

// GPIO Pins
const int GPIO_Button_WaterPump = 12, GPIO_LED_WaterPump = 15;
const int GPIO_Button_StopVent = 20, GPIO_LED_StopVent = 23;
const int GPIO_Button_Disp = 21, GPIO_LED_Disp = 18;

// Global state
std::atomic<bool> procedureWaterPump_Active(false);
std::atomic<bool> procedureStopVent_Active(false);
std::atomic<bool> procedureDisp_Active(false);
std::atomic<bool> running(true);


#define RAIN_GAUGE_PIN 17
#define Water_Flow_Pin 27  //Benyttes denne???????
#define WaterLevelPin1 24  //Vandniveau måling for fyldt palletank   høj: fyldt lav: ikke fyldt
#define WaterLevelPin2 11 //Vandniveau måling for tom palletank    høj: tom   lav: ikke tom
#define WaterFlow1_PIN 26
#define WaterFlow2_PIN 19
#define WaterFlow3_PIN 16
//#define LED_WaterPump_PIN 9
//#define LED_StopVents_PIN 10
//#define LED_Disp_PIN 11

#define LedPin 22       //LED der viser at det kører
//Expander Pi porte
#define Pi_Relay_Heat 1
#define Pi_Relay_WaterPump 2
#define Pi_Relay_LightGreenhouse 3
#define Pi_Relay_FreshWater 4  //Vandhanevand til tank#define Relay_LightPlants 5   //er ikkef orbundet elektrisk

#define Pi_Relay_WaterValve1 5 //COM 7 på relækort
#define Pi_Relay_WaterValve2 6 //COM 8 på relækort
#define Pi_Relay_WaterValve3 7 //COM 5 på relækort
#define Pi_Relay_WaterValve4 8 //COM 8 på relækort

//#define Pi_Relay_LED_WaterPump 9
//#define Pi_Relay_LED_StopVents 10
//#define Pi_Relay_LED_Disp 11

#define Pi_Relay_Motor1_dir 12
#define Pi_Relay_Motor1_pwm 13


//Liste over brug af analoge input
#define LightPin 2
#define AirFlowPin 3
#define SoilMoistureSensorPin1 3
#define SoilMoistureSensorPin2 4

//Variable der holder sætpunkter
int DayStartHour = 6;
int NightStartHour = 22;
int ComfortDayStartHour = 16;
int ComfortDayEndHour = 22;
int AirPumpTemperature = 18;
int WateringControl = 1;           //Hoælder værdi der læses løbende fra Setpoint tabel
int Last_WateringControl = 1;      //Gemmer sidste valg indtil nyt tages eller det nulstilles efter udført funktion

int Status_VandingStartTime1 = 9;  //Tid for start af første vanding
int Status_VandingStartTime2 = 20; //Tid fors tart af anden vanding
int Status_AutomaticControl = 1;
bool AutoWindow = true;
int Status_VandingsTidValve1 = 5;
int Status_VandingsTidValve2 = 5;
int Status_VandingsTidValve3 = 5;
int Status_VandingsTidValve4 = 5;
int VinduesKontrol = 5;
int VinduesStyring = 1;
int Aarstid = 1;
int StopVentsButton_minutes;
int TvangslukkedeVinduer_Minutter = 30;
int LogUse = 0;                //0: ingen log, 1: log 2: tøm log
int Vinduer_1TrinMere = 0;     //1 åbner vinduer 1 trin mere
int Status_LuftPumpe = 0;  //Holer aktuel status for luftpumpe, der bruges i vinterteletet.
//-------------------OrangeriLog----------------------------------


// Function to handle signals and ensure cleanup
void handleSignal(int signum)
{
    //spdlog::warn("Signal received: {}", signum); //kan være et problem med logger rotation
    running = false;
}


void myDelay(int delay)
{
    int test;
    test = 0;
    int timestamp = time(NULL);
    //cout << "myDelay Start " << timestamp << endl;

    while (time(NULL) - timestamp < delay)
    {
        test++;
    }

}

double RoundToDecimal(double Value, int decimals)
{
    int Factor = pow(10,decimals);
    Value = Value * Factor;
    Value = round(Value);
    return Value/Factor;
}

std::string showDate(time_t time)
{
    std::tm tm = *std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%y-%m-%d %H:%M"); // Format as "YYYY-MM-DD HH:MM:SS"
    return oss.str();
}


//***********************************************************************
//Tæller der sikre at der ikke kommer for mange data til Blynk pr sekund
//***********************************************************************
int BlynkUpdateCounter = 0; //Counter der holder styr på blynk opdateringer, så man undgår for mange data på en gang
int BlynkUpdateCounterDay = 0; //Counter der holder styr på blynk opdateringer, så man undgår for mange data på en gang
int UpdateWeekDay;
int BlynkUpdateTime = 0;
const int MaxUpdatePerSecond = 20;


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
            spdlog::error("ADC Expander Pi kan ikke åbnes");
        }
        ADCValue = ABE_expi.adc_read_voltage(ADCChn, 0);
        ABE_expi.dac_close();
    }


    catch (const std::out_of_range& oor)
    {
        spdlog::error("ReadADC - Out of Range error: {} ", oor.what());
        ReadADCError++;
    }

    catch (const std::runtime_error& oor)
    {
        spdlog::error("ReadADC - Out of Range error: {} ", oor.what());
        ReadADCError++;
    }

    catch (...)
    {
        spdlog::error(std::string("ReadADC - Out of Range error: "));
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
        spdlog::error(std::string("SetADC - Out of Range error: ") + oor.what());
        SetADCError++;
    }

    catch (const std::runtime_error& oor)
    {
        cout << "SetADC -  Out of Range error: " << oor.what() << endl;
        spdlog::error(std::string("SetADC - Out of Range error: ") + oor.what());
        SetADCError++;
    }

    catch (...)
    {
        cout << "SetADC - Fejl ved ReadADC " << endl;
        spdlog::error(std::string("SetADC - Fejl ved ReadADC: "));
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
        spdlog::error(std::string("IOPortSetup error: ") + e.what());
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
        spdlog::error(std::string("IOPortWrite: ") + e.what());
        IOPortWriteError++;
    }

    catch(...)
    {
        cout << "***** IOPortWrite Uspecificeret " << endl;
        spdlog::error(std::string("IOPortWrite - Unspecific: "));
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
        spdlog::error(std::string("IOPortRead: ") + e.what());
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


volatile unsigned long lastRainInterruptTime = 0;
volatile int rainPulseCount = 0; //tæller på flow1

//volatile int WaterFlow1EventCounter = 0;
double WaterFlow1;   //Holder flow i en periode
double WaterTotalFlow1 = 0; //Holder flow siden program start

//volatile int WaterFlow2EventCounter = 0;
double WaterFlow2;   //Holder flow i en periode
double WaterTotalFlow2 = 0; //Holder flow siden program start

//volatile int WaterFlow3EventCounter = 0;
double WaterFlow3;   //Holder flow i en periode
double WaterTotalFlow3 = 0; //Holder flow siden program start


volatile int pulseCountFlow1 = 0;
volatile int pulseCountFlow2 = 0;
volatile int pulseCountFlow3 = 0;



volatile unsigned long pulseCount = 0;
std::chrono::steady_clock::time_point lastPulseTime;
const int DEBOUNCE_MS = 100; // Adjust as needed

void countRainPulse(int pi,unsigned gpio, unsigned level, uint32_t tick)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPulseTime).count();

    if (level == 1 && elapsed > DEBOUNCE_MS)
    {
        rainPulseCount++;
        lastPulseTime = now;
    }

}
// Callback function for pulse counting
void pulseCallbackFlow1(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if (level == 1)   // Only count rising edges
    {
        pulseCountFlow1++;
    }
}

// Callback function for pulse counting
void pulseCallbackFlow2(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if (level == 1)   // Only count rising edges
    {
        pulseCountFlow2++;
    }
}
// Callback function for pulse counting
void pulseCallbackFlow3(int pi, unsigned gpio, unsigned  level, uint32_t tick)
{
    if (level == 1)   // Only count rising edges
    {
        pulseCountFlow3++;
    }
}


void GetWaterFlow1()
{
    // Calibration constant for YF-B5 flow sensor (1 pulse per 0.5 mL)
    const double volumePerPulse = 1.4; // mL

    WaterFlow1  = (pulseCountFlow1 * volumePerPulse) / 1000.0; // Convert to liters
    WaterTotalFlow1 = WaterTotalFlow1 + WaterFlow1; //Beregner nyt total forbrug
    if (WaterFlow1 > 0)
    {
        spdlog::info("WaterFlow1: {} total: {} pulse: {}", WaterFlow1, WaterTotalFlow1, pulseCountFlow1);
    }
    pulseCountFlow1 = 0; //Nulstller tæller når flow er læst
}

double GetRain()
{
    double RAIN_FACTOR = 0.2794;
    double rain = (rainPulseCount * RAIN_FACTOR); // Convert to liters
    //double rain = (pulseCount * RAIN_FACTOR); // Convert to liters
    if (rain > 0)
    {
        spdlog::info("Rain: {} Pulse: {}", rain, rainPulseCount);
    }
    rainPulseCount = 0; //Nulstller tæller når flow er læst
    return rain; //Beregner nyt total forbrug
}

void GetWaterFlow2()
{
    // Calibration constant for YF-B5 flow sensor (1 pulse per 0.5 mL)
    const double volumePerPulse = 2.2; // mL
    WaterFlow2  = (pulseCountFlow2 * volumePerPulse) / 1000.0; // Convert to liters
    WaterTotalFlow2 = WaterTotalFlow2 + WaterFlow2; //Beregner nyt total forbrug
    if (WaterFlow2 > 0)
    {
        spdlog::info("WaterFlow2: {} total: {} pulse: {}", WaterFlow2, WaterTotalFlow2, pulseCountFlow2);
    }
    pulseCountFlow2 = 0; //Nulstller tæller når flow er læst

}

void GetWaterFlow3()
{
    // Calibration constant for YF-B5 flow sensor (1 pulse per 0.5 mL)
    const double volumePerPulse = 2.2; // mL
    WaterFlow3  = (pulseCountFlow3 * volumePerPulse) / 1000.0; // Convert to liters
    WaterTotalFlow3 = WaterTotalFlow3 + WaterFlow3; //Beregner nyt total forbrug
    if (WaterFlow3 > 0)
    {
        spdlog::info("WaterFlow3: {} total: {} pulse: {}", WaterFlow3, WaterTotalFlow3, pulseCountFlow3);
    }

    pulseCountFlow3 = 0; //Nulstller tæller når flow er læst

}

double getCPUTemperature()
{
    string val;
    string preparedTemp;
    double temperature;
    ifstream temperatureFile ("/sys/class/thermal/thermal_zone0/temp");

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


int Status_WaterPump = 0;
int CurretStatus_WaterPump = 0; //Holder styr på den aktuelt valgte status for vandpumpen

void SetWaterPump(int Status_WaterPump)
{
    string errmsg;
    if (Status_WaterPump !=	CurretStatus_WaterPump)
    {

        spdlog::info("SetWaterPump  Status_WaterPump: {} CurrentStatus_WaterPump: {}", Status_WaterPump, CurretStatus_WaterPump);
        CurretStatus_WaterPump = Status_WaterPump;
        if (Status_WaterPump == 1)
        {
            gpio_write(pi, GPIO_LED_WaterPump, 1);
            ExPi_IOPortWrite(Pi_Relay_WaterPump, 1);
            db.Update_CurrentValue<string>("WaterPump","On", errmsg);
            spdlog::info("WaterPump On");
        }
        else
        {
            gpio_write(pi, GPIO_LED_WaterPump, 0);
            ExPi_IOPortWrite(Pi_Relay_WaterPump, 0);
            spdlog::info("WaterPump Off");
            db.Update_CurrentValue<string>("WaterPump","Off",errmsg);
        }
    }

}

//--------------------Argent Rain sensor -------------------------

//Rain variable
double RainDayAccum = 0.0;   //Rain accumulator since start of sample
double RainHourAccum = 0.0;   //Rain accumulator since start of sample

//#define RAIN_FACTOR 0.2794  //Argent raingauge 0.2794 mm se https://www.argentdata.com/catalog/product_info.php?products_id=168&osCsid=4gBQZzEt1wYYnSUJ7P9qN3
double Rain24h [24] = { };  //holder sidste 24 timeres regn

int lastButtonState; //gememr sidste læsnings værdi
time_t lastDebounceTime;
int debounceDelay;
int buttonState;


//==================Interrupt rutiner slut =========================



void ControlMotor(int MotorNumber)
{
    int pwm;
    int dir;
    int diff;
    bool moving;
    double currentstatus;
    double newstatus;
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

    if (currentstatus != newstatus)
    {
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

    string errmsg;
    if (nu >= stopmotor && moving == 1)
    {
        if (MotorNumber == 1)
        {
            spdlog::info("Motor stoppes new/current -- Ny status {} Aktuel status {}", newstatus,currentstatus);
        }

        ExPi_IOPortWrite(Pi_Relay_Motor1_pwm, 0);
        moving = 0;
        currentstatus = newstatus;
        VinduesKontrol = 5; //Sætter vindues menu til 5 (=ingen ændring)
        db.Update_Setpoint<int>(WindowControl_SetpointText,VinduesKontrol,errmsg);
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



void buttonISR(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    if (level == PI_LOW)
    {
        int buttonIndex = -1;
        int ledPin = -1;
        std::atomic<bool>* procedureState = nullptr;
        spdlog::debug("buttonISR gpio: {}  level: {} ",gpio,level);
        // Determine which button was pressed
        switch(gpio)
        {
        case GPIO_Button_WaterPump:
            buttonIndex = 0;
            ledPin = GPIO_LED_WaterPump;
            procedureState = &procedureWaterPump_Active;
            spdlog::debug("GPIO_Button_WaterPump");
            break;
        case GPIO_Button_StopVent:
            buttonIndex = 1;
            ledPin = GPIO_LED_StopVent;
            procedureState = &procedureStopVent_Active;
            spdlog::debug("GPIO_Button_StopVentPump");
            break;
        case GPIO_Button_Disp:
            buttonIndex = 2;
            ledPin = GPIO_LED_Disp;
            procedureState = &procedureDisp_Active;
            spdlog::debug("GPIO_Button_Disp");
            break;
        default:
            return;
        }

        // Debounce check
        if ((tick - lastInterruptTime[buttonIndex]) < debounceTime) return;
        lastInterruptTime[buttonIndex] = tick;

        // Toggle state
        bool newState = !(*procedureState);
        *procedureState = newState;
        //gpio_write(ledPin, newState ? 1 : 0);

        spdlog::info("Button NewState {} ledpin {} ", newState, ledPin);
        string errmsg;
        if (newState)
        {
            spdlog::info("newState: Started");
            switch(gpio)
            {
            case GPIO_Button_WaterPump:

                gpio_write(pi, GPIO_LED_WaterPump,1);
                ExPi_IOPortWrite(Pi_Relay_WaterPump, 1);
                db.Update_CurrentValue<string>("WaterPump","On",errmsg);
                WaterPumpButtonAtTime = time(NULL)+ waterPumpButton_Minutes *60;
                spdlog::info("WaterPump On until {:%H:%M:%S}", fmt::localtime(WaterPumpButtonAtTime));
                WaterPumpButtonActive = 1; //Angiver at StopVentButten er aktiv
                break;

            case GPIO_Button_StopVent:
                gpio_write(pi, GPIO_LED_StopVent, 1);
                CurrentStatus_Window1 = 100;  //Vinduer tvinges helt ned
                NewStatus_Window1 = 0;
                CurrentStatus_Window2 = 100;
                NewStatus_Window2 = 0;
                CurrentStatus_Window3 = 100;
                NewStatus_Window3 = 0;
                CurrentStatus_Window4 = 100;
                NewStatus_Window4 = 0;
                ControlWindows;
                StopVentButtonAtTime = time(NULL)+ StopVentsButton_minutes*60; //Deaktiverer automatiske vinduere indtil tid
                StopVentButtonActive = 1; //Angiver at StopVentButten er aktiv
                spdlog::info("Stop vinduer knap aktiv indtil {:%H:%M:%S}", fmt::localtime(StopVentButtonAtTime));
                break;
            case GPIO_Button_Disp:
                gpio_write(pi, GPIO_LED_Disp,1);
                spdlog::info("Disp button knap: on");
                break;
            }
        }
        else
        {
            spdlog::info("newState: Stopped");

            switch(gpio)
            {
            case GPIO_Button_WaterPump:
                gpio_write(pi, GPIO_LED_WaterPump,0);
                ExPi_IOPortWrite(Pi_Relay_WaterPump, 0);
                WaterPumpButtonActive = 0;
                db.Update_CurrentValue<string>("WaterPump","Off", errmsg);
                spdlog::info("WaterPump Off");
                break;

            case GPIO_Button_StopVent:
                gpio_write(pi, GPIO_LED_StopVent,0);
                StopVentButtonActive = 0;  //Aktiverer automatiske vinduere
                spdlog::info("Stop vinduer knap: Off");
                break;

            case GPIO_Button_Disp:
                gpio_write(pi, GPIO_LED_Disp,0);
                spdlog::info("Disp button knap off");
                //ExPi_IOPortWrite(Pi_Relay_WaterPump, 0);
                break;
            }


        }
    }
}

void OrangeriSetup(void);
void ControlGreenhouseVerySlowTimer();
void ControlGreenhouseSlowTimer();
void ControlGreenhouseFastTimer();
void WriteInfoToLog();

#include <MyStat.h>
#include <ctime>
#include <string>
//#include <bmp180.h>         //Læsning af barometertryk på Bosch kort
#include <DS18B20.h>        //Læsning af temperatur på one wire forbindelse
#include <fstream>
bool Use_WDT = false; //Angiver om Watchdog skal være aktiv i hele programmet eller om det kun benyttes ved BlynkConnect.

//Statistik variable
MyStat GreenhouseDayTempStats;
MyStat WeatherDayTempStats;
MyStat GreenhouseHourTempStats;
MyStat WeatherHourTempStats;
MyStat RainDayStats;
MyStat RainHourStats;
MyStat AirFlowStats;

//VAriable der sættes for at kunne sende en email
char* emailAdress = "jesper@aaslyng.dk";
char* emailSubject = "";
const char* emailMessage;

//Parametre der holder gemte værdier at blynk uploadeded værdier
//double ControlBoxTemperature_last = 0;
//double ControlBoxPressure_last = 0;
//double ControlBoxHumidity_last = 0;
//double Setpoint_TemperatureNow_last = 0;
double AirFlowValue;

double SoilMoistureValue1;
double SoilMoistureValue2;
double SoilMoistureValue3;
time_t StopHeating = 0;

double Greenhouse_temperature;
double Temperature4Vent;
double Greenhouse_humidity;
double Extra1_temperature;
double Extra2_temperature;
double Weather_temperature;
double Weather_humidity;

//--------------------
std::string str ("Test string");

time_t Status_LastWateringValve1;
time_t Status_LastWateringValve2;
time_t Status_LastWateringValve3;
time_t Status_LastWateringValve4;

time_t Status_LastStartTime1 = 0;
time_t Status_LastStartTime2 = 0;

time_t Status_StopWateringValve1 = 0;
time_t Status_StopWateringValve2 = 0;
time_t Status_StopWateringValve3 = 0;
time_t Status_StopWateringValve4 = 0;

std::string WaterLevel1_Msg = "Ikke bestemt";


//kontrollerede adresser
DS18B20 Probe02("28-0316473b91ff");  //Drivhus på lang ledning (orange)
DS18B20 Probe04("28-0316556970ff");  //sort kort ledning
DS18B20 Probe06("28-030a97940080");  //grøn Vejrstation


//skal kontrolleres igen
DS18B20 Probe01("28-0316555af8ff");  //Gul, ændret til Vejrstation
//DS18B20 Probe06("28-041650bc06ff");  //grøn
DS18B20 Probe05("28-06BC5016040B");  //Klar
DS18B20 Probe07("28-0316622393ff");  //Rød+Grøn
DS18B20 Probe03("28-0316622393ff");  //Virker muligvis ikke længere, gammel vejrstation sensor der gav problemer



//--------------------DHT Temperature Sensor -------------------------
//DHT 22  (AM2302), AM2321
//DHT 21 (AM2301)
//DHT11   // DHT 11

#include <pi_2_dht_read.h>  //Læsning af DHT sensorer DHT11/DHT22/AM2302
#include <pi_2_mmio.h>      //Benyttes som ovenover til læsning af DHT sensorer

int dht_res;
double dht_humidity;
double dht_temperature;

/*
DHT dht_Greenhouse(DHT_Greenhouse_PIN, DHT22);
DHT dht_Extra1(DHT_Extra1_PIN, DHT22);
DHT dht_Extra2(DHT_Extra2_PIN, DHT22);
*/



double Probe01_Temp, Probe02_Temp, Probe03_Temp, Probe04_Temp, Probe05_Temp, Probe06_Temp, Probe07_Temp;

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

void doWatering(int MenuValg)
{
    if (MenuValg == Last_WateringControl)
    {
        return;
    }

    spdlog::info("doWatering Start med parameter {}", MenuValg);
    time_t nu = time(NULL);
    Last_WateringControl = WateringControl;

    switch (MenuValg)
    {
    case 1:
    {
        // Ingen vanding
        break;
    }
    case 2:
    {
        // 5 min ekstra vand
        spdlog::info("5 min vand til kanal 1");
        cout << "5 min vand til kanal 1" << endl;
        Status_WaterValve1 = 1; // åbner ventil 1
        Status_LastWateringValve1 =  nu ;
        //SidsteVandingsLabel1 = "'" + showDate(nu) + "'";
        Status_StopWateringValve1 = nu + 300;
        Status_WaterManual1 = 0; //Ikke manuel vanding efter tryk på knap

        break;
    }

    case 3:
    {
        // 5 min ekstra vand
        cout << "5 min vand til kanal 2" << endl;
        spdlog::info("5 min vand til kanal 2");

        Status_WaterValve2 = 1; // åbner ventil 1
        Status_LastWateringValve2 =  nu ;
        //SidsteVandingsLabel2 = "'" + showDate(nu) + "'";
        Status_StopWateringValve2 = nu + 300;
        Status_WaterManual2 = 0; //Ikke manuel vanding
        break;
    }


    case 4:
    {
        // 5 min ekstra vand
        spdlog::info("5 min vand til kanal 3");

        cout << "5 min vand til kanal 3" << endl;
        Status_WaterValve3 = 1; // åbner ventil 1
        Status_LastWateringValve3 =  nu ;
        //SidsteVandingsLabel3 = "'" + showDate(nu) + "'";
        Status_StopWateringValve3 = nu + 300;
        Status_WaterManual3 = 0; //Ikke manuel vanding
        break;
    }


    case 5:
    {
        // 5 min ekstra vand
        spdlog::info("5 min vand til kanal 4");

        cout << "5 min vand til kanal 4" << endl;
        Status_WaterValve4 = 1; // åbner ventil 1
        Status_LastWateringValve4 =  nu ;
        //SidsteVandingsLabel4 = "'" + showDate(nu) + "'";
        Status_StopWateringValve4 = nu + 300;
        Status_WaterManual4 = 0; //Ikke manuel vanding
        break;
    }

    case 6:
    {
        // 5 min ekstra vand
        spdlog::info("5 min vand til all kanaler");

        cout << "5 min vand til palletank" << endl;

        Status_WaterValve1 = 1; // åbner ventil 1
        Status_LastWateringValve1 =  nu ;
        //SidsteVandingsLabel1 = "'" + showDate(nu) + "'";
        Status_StopWateringValve1= nu + 300;

        Status_WaterManual2 = 0; //Ikke manuel vanding
        Status_WaterValve2 = 1; // åbner ventil 1
        Status_LastWateringValve2 =  nu ;
        //SidsteVandingsLabel2= "'" + showDate(nu) + "'";
        Status_StopWateringValve2 = nu + 300;

        Status_WaterManual3 = 0; //Ikke manuel vanding
        Status_WaterValve3 = 1; // åbner ventil 1
        Status_LastWateringValve3 =  nu ;
        //SidsteVandingsLabel3 = "'" + showDate(nu) + "'";
        Status_StopWateringValve3 = nu + 300;
        Status_WaterManual3 = 0; //Ikke manuel vanding

        Status_WaterValve4 = 1; // åbner ventil 1
        Status_LastWateringValve4 =  nu ;
        //SidsteVandingsLabel4 = "'" + showDate(nu) + "'";
        Status_StopWateringValve4 = nu + 300;
        Status_WaterManual4 = 0; //Ikke manuel vanding

        break;
    }

    case 7:
    {
        // Vanding startes og sluttes manuelt
        spdlog::info("5 min vand til palletank");

        string msg;
        Status_FreshWaterToTankValve = 1; // åbner ventil 3 der tilfører vand til vandtank
        cout << "Vandhanevand tændes i 5 minutter" << endl;
        break;
    }


    }

}


//-------------------Water flow ----------------------------------
// The hall-effect flow sensor outputs approximately 4.5 pulses per second per litre/minute of flow.
/*double calibrationFactor = 7.5;
volatile int WaterFlowPulseCount;
double WaterFlowRate;
unsigned int WaterFlowMililitres;
unsigned long WaterTotalMililitres;
unsigned long oldTime;
*/


//--------------------BMP Sensor for Lufttryk-------------------------------
//SFE_BMP180 pressure; // You will need to create an SFE_BMP180 object, here called "pressure":%
//#define ALTITUDE 45.9 // Altitude aflæst på vores adresse på www.altitude-maps.com
double ControlBoxTemperature;  //Temperatur målt i kontrolboks med Tryksensor
double ControlBoxPressure;     //Tryk målt i kontrolboks med Tryksensor
double ControlBoxHumidity;

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
//-------------------Rutiner der kaldes når værdier i Blynk widgets ændres, altså på de mobile enheder.-------------

void ManageWindows(int Code)
{
    WindowsChangedTime = 0;  //Nulstiller sidst brugte justerings tid
    switch (Code)
    {
    case 1:
    {
        // Item 1 Luk vinduer helt
        spdlog::info("Vindues kontrol - Vinduer lukkes helt");
        NewStatus_Window1 = 0;
        NewStatus_Window2 = 0;
        NewStatus_Window3 = 0;
        NewStatus_Window4 = 0;
        break;
    }
    case 2:
    {
        // Item 2 åben vinduer helt
        spdlog::info("Vindues kontrol - Vinduer åbnes helt");
        NewStatus_Window1 = 100;
        NewStatus_Window2 = 100;
        NewStatus_Window3 = 100;
        NewStatus_Window4 = 100;
        break;
    }
    case 3:
    {
        // Item 3 åben vinduer et trin mere
        spdlog::info("Vindues kontrol - Vinduer åbnes et trin mere. Ny % åbning er {:.1f}", NewStatus_Window1);
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
        spdlog::info("Vindues kontrol - Vinduer lukkes et trin mere. Ny % åbning er {:.1f}", NewStatus_Window1);

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

void OpenWindowsOneStepMore(int code)
{
    if (code == 1 && Motor1_Moving == 0)
    {
        NewStatus_Window1 = NewStatus_Window1 + WindowTimerStepInPct;
        if (NewStatus_Window1 >= 100)
        {
            NewStatus_Window1 = 0;
        }
        cout <<("********** Windues åbning ændret ved tryk på knap *************** ");
        cout << NewStatus_Window1 << "\n";
        Vinduer_1TrinMere = 0;
    }
}


void SetFreshWatertoTank(bool FreshWaterToTank)
{
    //spdlog::info("SetFreshWatertoTank {}",FreshWaterToTank);
    //ExPi_IOPortWrite(Pi_Relay_FreshWater, FreshWaterToTank);
    string errmsg ;
    if (FreshWaterToTankOn != FreshWaterToTank)    //status på vanding ændres
    {
        spdlog::info("Vand palletank ændring: FreshWaterToTankOn {} FreshWaterToTank {}", FreshWaterToTankOn, FreshWaterToTank);

        if (FreshWaterToTank == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeFreshWaterToTank = Stat_TimeFreshWaterToTank + nu - FreshWaterToTankTimeOn;
            FreshWaterToTankOn = 0;
            ExPi_IOPortWrite(Pi_Relay_FreshWater, 0);
            db.Update_CurrentValue<string>("WaterToContainer","Off", errmsg);
            spdlog::info("Vandniveau lavt - Vand tilførelse afbrydes");
        }

        if (FreshWaterToTank == 1)    //der er blevet tændt for vandet
        {
            FreshWaterToTankTimeOn = time(0);
            FreshWaterToTankOn = 1;
            db.Update_CurrentValue<string>("WaterToContainer","On",errmsg);
            spdlog::info("Vandniveau lavt - Vand tilføres til palletank");
            ExPi_IOPortWrite(Pi_Relay_FreshWater, 1);
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
            spdlog::info("Vanding 1 - Slukkes");
        }

        if (WaterValve1 == 1)    //der er blevet tændt for vandet
        {
            WaterValve1TimeOn = time(0);
            WaterValve1On = 1;
            spdlog::info("Vanding 1 - Startes");
        }
    }
    //Sætter relæ
    ExPi_IOPortWrite(Pi_Relay_WaterValve1, WaterValve1);
}


void SetWaterValve2(bool WaterValve2)
{

    if (WaterValve2On != WaterValve2)    //status på vanding ændres
    {
        if (WaterValve2 == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeWaterValve2 = Stat_TimeWaterValve2 + nu - WaterValve2TimeOn;
            WaterValve2On = 0;
            spdlog::info("Vanding 2 - Slukkes");
        }

        if (WaterValve2 == 1)    //der er blevet tændt for vandet
        {
            WaterValve2TimeOn = time(0);
            WaterValve2On = 1;
            spdlog::info("Vanding 2 - Startes");
        }
    }

    //Sætter relæ
    ExPi_IOPortWrite(Pi_Relay_WaterValve2, WaterValve2);

}


void SetWaterValve3(bool WaterValve3)
{


    if (WaterValve3On != WaterValve3)    //status på vanding ændres
    {
        if (WaterValve3 == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeWaterValve3 = Stat_TimeWaterValve3 + nu - WaterValve3TimeOn;
            WaterValve3On = 0;
            spdlog::info("Vanding 3 - Slukkes");

        }

        if (WaterValve3 == 1)    //der er blevet tændt for vandet
        {
            WaterValve3TimeOn = time(0);
            WaterValve3On = 1;
            spdlog::info("Vanding 3 - Startes");

        }
    }
    ExPi_IOPortWrite(Pi_Relay_WaterValve3, WaterValve3);
}

void SetWaterValve4(bool WaterValve4)
{

    if (WaterValve4On != WaterValve4)    //status på vanding ændres
    {
        if (WaterValve4 == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeWaterValve4 = Stat_TimeWaterValve4 + nu - WaterValve4TimeOn;
            WaterValve4On = 0;
            spdlog::info("Vanding 4 - Slukkes");

        }

        if (WaterValve4 == 1)    //der er blevet tændt for vandet
        {
            WaterValve4TimeOn = time(0);
            WaterValve4On = 1;
            spdlog::info("Vanding 4 - Startes");

        }
    }
    ExPi_IOPortWrite(Pi_Relay_WaterValve4, WaterValve4);
}


void SetHeat(bool HeatRelay)
{
    spdlog::debug("SetHeat 1 {}", HeatRelay);
    if (AfterOrangeriSetup == 0)
    {
        HeatOn = 0;
        return;
    }
    spdlog::debug("SetHeat 2 {} ", HeatRelay);
    ExPi_IOPortWrite(Pi_Relay_Heat, HeatRelay);

    if (HeatOn != HeatRelay)    //status på vanding ændres
    {
        spdlog::info("HeatOn status andret");
        if (HeatRelay == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeHeat = Stat_TimeHeat + nu - HeatTimeOn;
            HeatOn = 0;
            spdlog::info("Der slukkes for varmen");
        }

        if (HeatRelay == 1)    //der er blevet tændt for varmen
        {
            HeatTimeOn = time(0);
            HeatOn = 1;
            spdlog::info("Det tændes for varmen");
        }
    }
}


//Ved vintertilstand med telt kan der ikke tændes lys, da stik benyttes til Luftpumpe
void SetLight(bool LightRelay)
{

    if (AfterOrangeriSetup == 0)
    {
        return;
    }

    if (WinterTent == 0)
    {
        return;
    }

    if (LightRelay == 1)
    {
        ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, 0);
    }
    else
    {
        ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, 1);
    }


    //cout << "SetLight " << LightRelay << " " << LightOn << endl;
    if (LightOn != LightRelay)    //status på vanding ændres
    {
        if (LightRelay == 0)  //der er blevet slukket for vandet
        {
            Stat_TimeLight = Stat_TimeLight + nu - LightTimeOn;
            LightOn = 0;
            spdlog::info("Der slukkes for lyset ");

        }

        if (LightRelay == 1)    //der er blevet tændt for vandet
        {
            LightTimeOn = time(0);
            LightOn = 1;
            spdlog::info("Der tændes for lyset");
        }
    }
}



//Opdater data til Blynk systemet
void UpdateBlynkAfterFastTimer()
{
    char msg[30];
    string str_msg;

    string errmsg;
    {
        try
        {
            str_msg = showDate(LastRunTime);
            cout <<  "FastTimer 0.2" << str_msg << endl;
            strcpy(msg, str_msg.c_str());
            cout << "Start Update...FastTimer 0.3 " << msg << endl;
            db.Update_CurrentValue<string>("LastRunTime",msg, errmsg);


#if defined(DebugWater)
            cout << "Tidspunkt nu: " << nu << endl;
            cout << "Vanding 1   stop: "  << Status_StopWateringVentil1 << endl;
            cout << "Vanding 2   stop: "  << Status_StopWateringVentil2 << endl;
            cout << "Vanding 3   stop: "  << Status_StopWateringVentil3 << endl;
            cout << "Vanding 4   stop: "  << Status_StopWateringVentil4 << endl;
#endif // defined

            if ( nu  >= Status_StopWateringValve1  & Last_WateringControl == 2)
            {

                WateringControl = 1; //Sætter menu til "Ingen vanding"

                db.Update_Setpoint<int>(WateringControl_SetpointText, WateringControl,errmsg);
                db.Update_CurrentValue<string>("Vanding1Header"," ", errmsg); //Sletter label der angiver at der vandes
            }

            if ( nu  >= Status_StopWateringValve2 & Last_WateringControl == 3)
            {
                WateringControl = 1; //Sætter menu til "Ingen vanding"
                db.Update_Setpoint<int>(WateringControl_SetpointText, WateringControl,errmsg);
                db.Update_CurrentValue<string>("Vanding2Header"," ", errmsg);
            }

            if ( nu  >= Status_StopWateringValve3 & Last_WateringControl == 4)
            {
                WateringControl = 1; //Sætter menu til "Ingen vanding"
                db.Update_Setpoint<int>(WateringControl_SetpointText, WateringControl,errmsg);
                db.Update_CurrentValue<string>("Vanding3Header"," ", errmsg);

            }

            if ( nu  >= Status_StopWateringValve4 & Last_WateringControl == 5)
            {
                WateringControl = 1; //Sætter menu til "Ingen vanding"
                db.Update_Setpoint<int>(WateringControl_SetpointText, WateringControl,errmsg);
                db.Update_CurrentValue<string>("Vanding4Header"," ", errmsg);
            }

            if ( nu  >= Status_StopWateringValve1  & Last_WateringControl == 6)
            {
                WateringControl  = 1; //Sætter menu til "Ingen vanding"
                db.Update_Setpoint<int>(WateringControl_SetpointText, WateringControl,errmsg);
                db.Update_CurrentValue<string>("Vanding1Header"," ", errmsg); //Sletter label der angiver at der vandes
            }

            cout << "Motor1_Moving " << Motor1_Moving << endl;
            if (Motor1_Moving == 0)
            {
                //(WindowPct_Virtual_Pin, NewStatus_Window1);
                cout << "NewStatus_Window1 " << NewStatus_Window1 << endl;
            }
            else
            {
                db.Update_CurrentValue<double>("WindowPct", -1, errmsg);
            }


            FirstBlynkUpdate = 0;

        }

        catch (...)
        {

            spdlog::error(std::string("Fejl under update af fasttimer"));
            BlynkError = BlynkError +1;
        }
    }


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
        gpio_write(pi,LedPin, 0);
        LedPinOn = 0;

    }
    else
    {
        gpio_write(pi, LedPin, 1);


        LedPinOn = 1;
    }

    //spdlog::info("Orangeri temperatur: {:.1f} Status_LuftPumpe: {} Status_Light {}", Greenhouse_temperature, Status_LuftPumpe, Status_LightGreenhouse);
    //Lang sensor bruges til temp-måling i telt
    if (WinterTent == 1)
    {
        //Pumpe tændes når temperaturen overstiger 18C
        if (Greenhouse_temperature > AirPumpTemperature & Status_LuftPumpe == 0)
        {
            ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, 1);
            Status_LuftPumpe = 1;
            spdlog::info("Luftpumpe tændes over {} grader i telt {:.1f},  Status_LuftPumpe {}", AirPumpTemperature, Greenhouse_temperature, Status_LuftPumpe);
        };

        if (Greenhouse_temperature < AirPumpTemperature & Status_LuftPumpe == 1)
        {
            ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, 0);
            Status_LuftPumpe = 0;
            spdlog::info("Luftpumpe slukkes med under {} grader i telt {:.1f},  Status_LuftPumpe {}", AirPumpTemperature, Greenhouse_temperature, Status_LuftPumpe);
        }

    }

    //msg1 = showDate(WaterPumpButtonActive);
    //cout << "WaterPumpButtonActive indtil " << msg1 << endl;

    /*    WaterPumpButtonState =  gpioRead(Button_WaterPump_Pin);
        StopVentsButtonState =  gpioRead(Button_StopVents_Pin);
        DispButtonState =  gpioRead(Button_Disp_Pin);
    */
#if defined(DebugWater)
    cout << "WaterPumpButtonState: " << WaterPumpButtonState << " waterbutton_PumpOn_minutes: " << waterbutton_PumpOn_minutes << endl;
#endif // defined
    /*    cout << "StopVentsButtonState: " << StopVentsButtonState << " StopVentsButton_minutes: " << StopVentsButton_minutes << endl;
        cout << "DispButtonState: " << DispButtonState << endl;
    */
    //cout << "WaterPumpButtonActive " << WaterPumpButtonActive << " " << nu	<< endl;
    //Hvis Pumpe skal stoppes da tid efter tryk er udløbet,
    //Hvis den er 0 er den stoppet tidligere
    /*   if (WaterPumpButtonActive < nu && WaterPumpButtonActive > 0)
       {
           cout << "Pumpe stoppes tid er udløbet " << endl;
           //Hvis WaterPumpButtonActive ikke er igang slukkes for vandet medmindre
           //der vandes automatisk med en af de 4 Valveer
           if (Status_WaterValve1 == 0 || Status_WaterValve2 == 0 || Status_WaterValve3 == 0 || Status_WaterValve4 == 0)
           {
    F
               WaterPumpButtonActive  = 0;
               myControlWaterPumpButton(WaterPumpButtonActive);
           }
       }
    */

    if (nu > StopVentButtonAtTime && StopVentButtonAtTime > 0)
    {
        StopVentButtonAtTime = 0;
        StopVentButtonActive = 0;  //Afbryder stop af vinduer
        spdlog::info("StopVentButtonAtTime: Tid overskredet");
        gpio_write(pi, GPIO_LED_StopVent, 0);
    }

    if (nu > WaterPumpButtonAtTime && WaterPumpButtonAtTime > 0 )
    {
        WaterPumpButtonAtTime = 0;
        WaterPumpButtonActive = 0;
        gpio_write(pi, GPIO_LED_WaterPump, 0);
        spdlog::info("WaterPumpButtonAtTime: Tid overskredet");
        Status_WaterPump = 0;
        //SetWaterPump(Status_WaterPump);
    }


    /*  if (DispButtonActive < nu)
      {
          //cout << "Disp funktion afbrydes " << endl;
          DispButtonActive  = 0;
          ControlDispButton(DispButtonActive);
      }

    */


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
    {gpio_write(LED_Stoped_Vents_Pin, HIGH);}
      else
    {gpio_write(LED_Stoped_Vents_Pin, LOW);}
    */

    // cout << "Vinduer:" << endl;
    // cout << "AutoWindow "  << AutoWindow << endl;

    //spdlog::info("AutoWindow: {}  StopVeentButtenActive {}", AutoWindow, StopVentsButtonActive);
    //Styrer hvordan vinduer skal åbnes og lukkes hvis automatik er sat
    //til og knap for StopVents ikke er aktiv

//    if (AutoWindow == 1 and StopVentsButtonActive == 0)
    spdlog::debug("AutoWindow {} StopVentsButtonActive {} ", AutoWindow, StopVentButtonActive);
    if (AutoWindow == 1 & StopVentButtonActive == 0)  //Automatiske vinduer og ikke stop med knap
    {
        //cout << endl;
        //cout << "Motor1_Moving " << Motor1_Moving << "\n";
        //cout << "LastRunTime " << showDate(LastRunTime) << endl;
        //cout << "WindowsChangedTime " << showDate(WindowsChangedTime) << endl;
        //cout << "MinSecBetweenMotorMoving " << MinSecBetweenMotorMoving << endl;

        spdlog::info("VinterTent {} Greenhouse_temperature {} Extra1_temperature {}", WinterTent, Greenhouse_temperature, Extra1_temperature);
        //Det bestemmes om der åbnes viduer efter om der telt i orangeriet
        Temperature4Vent = Greenhouse_temperature;   //Som standard benyttes den almindelige orangeritemperatur til vinduesstyring
        if (WinterTent == 1)
        {
            Temperature4Vent = Extra1_temperature;
        }

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

                spdlog::debug("TemperatureDiff {} ", TmpDif);
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



    //Vinduer holdes lukket når AutoWindow er = 0
    if (AutoWindow == 0)
    {
        NewStatus_Window1 = 0;
        NewStatus_Window2 = 0;
        NewStatus_Window3 = 0;
        NewStatus_Window4 = 0;
    }

    //Lukker for varme
    if ( nu  >= StopHeating && HeatOn == 1)
    {
        //Status_HeatUp = 0;
        SetHeat(0);
        spdlog::info("Varme stoppes da tid for opvarmning er overskredet");
    };

    if (nu < StopHeating && HeatOn == 0)
    {
        //Status_HeatUp = 1;
        SetHeat(1);
        spdlog::info("Varme startes da nu er < StopHeating");
    }


    string errmsg;
    if ( nu  >= Status_StopWateringValve1 && Status_WaterValve1 == 1)
    {
        Status_WaterValve1 = 0;
        string t = showDate(Status_StopWateringValve1);
        db.Update_CurrentValue<string>(LastWatering1_Parameter,t, errmsg);
        spdlog::info("Vanding kanal 1 stoppes {}", t );

    }

    if ( nu  >= Status_StopWateringValve2 && Status_WaterValve2 == 1)
    {
        Status_WaterValve2 = 0;
        string t = showDate(Status_StopWateringValve2);
        spdlog::info("Vanding kanal 2 stoppes {}", t );
        db.Update_CurrentValue<string>(LastWatering2_Parameter,t, errmsg);
    }

    if ( nu  >= Status_StopWateringValve3 && Status_WaterValve3 == 1)
    {
        Status_WaterValve3 = 0;
        string t = showDate(Status_StopWateringValve3);
        spdlog::info("Vanding kanal 3 stoppes {}", t );
        db.Update_CurrentValue<string>(LastWatering3_Parameter,t, errmsg);
    }

    if ( nu  >= Status_StopWateringValve4  && Status_WaterValve4 == 1)
    {
        Status_WaterValve4 = 0;
        string t = showDate(Status_StopWateringValve4);
        spdlog::info("Vanding kanal 4 stoppes {}", t );
        db.Update_CurrentValue<string>(LastWatering4_Parameter,t, errmsg);
    }
    //Styrer vinduer efter behov defineret tidligere
    ControlWindows();

    //spdlog::info("Inden SetWaterValves 1: {} 2: {} :3 {} 4: {} WaterPump: {}", Status_WaterValve1, Status_WaterValve2, Status_WaterValve3, Status_WaterValve4, Status_WaterPump);
    SetWaterValve1(Status_WaterValve1);
    SetWaterValve2(Status_WaterValve2);
    SetWaterValve3(Status_WaterValve3);
    SetWaterValve4(Status_WaterValve4);
    SetWaterPump(Status_WaterPump);

    SetFreshWatertoTank(Status_FreshWaterToTankValve);

    if (!WinterTent)
    {
        SetLight(Status_LightGreenhouse);
    }

    LastRunTime =  nu;  //Variable der holder styr på hornår der er kørt en styring


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
        string errmsg;
        str_msg = showDate(nu);
        strcpy(msg, str_msg.c_str());
        db.Update_CurrentValue<double>("Weather_temperature",Weather_temperature, errmsg);

        double WeatherTempMinDay = WeatherDayTempStats.GetMin();
        double WeatherTempMaxDay = WeatherDayTempStats.GetMax();
        double GreenhouseTempMin = GreenhouseDayTempStats.GetMin();
        double GreenhouseTempMax = GreenhouseDayTempStats.GetMax();

        db.Update_CurrentValue<double>("WeatherTemp",Weather_temperature, errmsg);
        db.Update_CurrentValue<double>("WeatherAvgTemp",WeatherTempAvgDay,errmsg);
        db.Update_CurrentValue<double>("WeatherMinTemp",WeatherTempMinDay, errmsg);
        db.Update_CurrentValue<double>("WeatherMaxTemp",WeatherTempMaxDay, errmsg);

        db.Update_CurrentValue<double>("RainHourAccum",RainHourAccum, errmsg);
        db.Update_CurrentValue<double>("RainAccumDay",RainDayAccum, errmsg);
        db.Update_CurrentValue<double>("Extra1_temperature",Extra1_temperature,errmsg);
        db.Update_CurrentValue<double>("Extra2_temperature",Extra2_temperature,errmsg);
        db.Update_CurrentValue<double>("ControlBoxTemperature",ControlBoxTemperature,errmsg);
        db.Update_CurrentValue<double>("ControlBoxPressure",ControlBoxPressure,errmsg);
        db.Update_CurrentValue<double>("Setpoint_TemperatureNow",Setpoint_TemperatureNow,errmsg);

        db.Update_CurrentValue<double>("WaterFlow1", WaterFlow1,errmsg);
        db.Update_CurrentValue<double>("WaterFlow1_Total", WaterTotalFlow1,errmsg);

        GreenhouseHourTempStats.AddData(Greenhouse_temperature);
        GreenhouseDayTempStats.AddData(Greenhouse_temperature);
        WeatherDayTempStats.AddData(Weather_temperature);
        WeatherHourTempStats.AddData(Weather_temperature);
        AirFlowStats.AddData(AirFlowValue);

        cout << "UpdateAfterSlow 2" << endl;

        double AirFlowMin = AirFlowStats.GetMin();
        double AirFlowMax = AirFlowStats.GetMax();
        db.Update_CurrentValue<double>("AirFlowValue",AirFlowValue, errmsg);
        db.Update_CurrentValue<double>("AirFlowMin",AirFlowMin, errmsg);
        db.Update_CurrentValue<double>("AirFlowMax",AirFlowMax, errmsg);
        db.Update_CurrentValue<double>("TempAvgDay",GreenhouseTempAvgDay, errmsg);
        cout << "UpdateAfterSlow 2.2" << endl;
        myDelay(blynkDelay);
        //Blynk.virtualWrite(Greenhouse_temperature_Virtural_Pin, Greenhouse_temperature);
        //cout << "Greenhouse temperature to Blynk "  << Greenhouse_temperature << "  " << Greenhouse_temperature_last << endl;
        db.Update_CurrentValue<double>("Greenhouse_temperature",   Greenhouse_temperature, errmsg);
        db.Update_CurrentValue<double>("Greenhouse temperature, min", GreenhouseTempMin, errmsg);
        db.Update_CurrentValue<double>("Greenhouse temperature, max", GreenhouseTempMax, errmsg);

        db.Update_CurrentValue<double>("Greenhouse_humidity",   Greenhouse_humidity, errmsg);
        db.Update_CurrentValue<double>("CPUTemperature",CPUTemperature, errmsg);
        db.Update_CurrentValue<double>(WindowsPct_Parameter, CurrentStatus_Window1, errmsg);

        db.Update_CurrentValue<string>("WaterLevel1",WaterLevel1_Msg, errmsg);



        if (Status_LastWateringValve1 >= Status_StopWateringValve1 )
        {
            //spdlog::info("SidsteVandingsLabel1, {}", SidsteVandingsLabel1);
            //spdlog::info("SidsteVandingsLabel1_Last, {}", SidsteVandingsLabel1_last);
            string d = showDate(Status_LastWateringValve1);
            db.Update_CurrentValue<string>("SidsteVandingLabel1", d, errmsg);
            //SidsteVandingsLabel1_last = SidsteVandingsLabel1;
        }


        db.Update_CurrentValue<double>("LastWateringTimeLength_Greenhouse",Status_VandingsTidValve1*60*AddExtraWaterTime_Greenhouse, errmsg);
        db.Update_CurrentValue<double>("LastWateringTimeLength_Garden",Status_VandingsTidValve1*60*AddExtraWaterTime_Garden, errmsg);

        //Giver status på om Valve der åbner for vandhane vand til palletank er åben
        if (Status_FreshWaterToTankValve == 0)
        {
            db.Update_CurrentValue<string>("AddWaterValve","Vandhane lukket", errmsg);
        }
        else
        {
            db.Update_CurrentValue<string>("AddWaterValve","Vandhane åben", errmsg);
        }

        //Status_FreshWaterToTankValve_last = Status_FreshWaterToTankValve;

        ForceBlynkUpdate = false; //Sikre at de ved næste BlynkUpdate opdatres værdier uanst om de er ændret eller ej

        myDelay(blynkDelay);
        int CurrentHour = round(getHour(nu));
        Rain24h[CurrentHour] = RainHourAccum;
        string mystr =  showDate(nu) + to_string(CurrentHour);
        //if (UpdateBlynkOk()) {Blynk.virtualWrite(RainTable_Virtual_Pin,"update",CurrentHour, mystr.c_str(),RainHourAccum);}
        cout << "Opdaterer regnsummer" << endl;
        myDelay(blynkDelay);
        double RainSum3 = 0;
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


        db.Update_CurrentValue<double>(RainSum3h_Parameter, RainSum3, errmsg);

        double RainSum6 = 0;
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
        db.Update_CurrentValue<double>(RainSum6h_Parameter, RainSum6, errmsg);

        double RainSum24 = 0;
        for (int i=0; i<24; i++)
        {
            RainSum24 = RainSum24 + Rain24h[i];
        }
        db.Update_CurrentValue<double>(RainSum24h_Parameter, RainSum24, errmsg);

        cout << endl;;
        cout << "********* Slut UpdateBlynkAfterSlow ******" << endl;
        cout << endl;


    }
    catch (...)
    {
        cout << "!!!!!  Fejl ved UpdateBlynk" << endl;
        spdlog::error(std::string("Fejl ved update after slow"));
        BlynkError = BlynkError +1;
    }

}


ofstream DataFile;
string DataFileName = "/home/pi/Documents/Orangeri/orangeri.csv";

bool AppendDataToFile()
{
    try
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
    catch (...)
    {
        spdlog::error(std::string("Fejl ved AppendDataToFile"));
    }


}


void ReadSetpoints()
{

    string sqlError = "";

    spdlog::info("ReadSetpoints");

    string Parameter = "UdluftningsTemperatur_Aften";
    db.Get_Setpoint<double>(Parameter, Setpoint_UdluftningTemperatureComfort, 20, sqlError);

    Parameter = "UdluftningsTemperatur";
    db.Get_Setpoint<double>(Parameter, Setpoint_UdluftningTemperature, 20, sqlError);

    Parameter = "OpvarmningsTemperatur_Dag";
    db.Get_Setpoint<double>(Parameter, Setpoint_TemperatureDay, 20, sqlError);

    Parameter = "OpvarmningsTemperatur_Nat";
    db.Get_Setpoint<double>(Parameter, Setpoint_TemperatureNight, 20, sqlError);

    Parameter = "Temperatur_Styring";
    db.Get_Setpoint<int>(Parameter, Status_AutomaticControl, 1, sqlError);

    Parameter = WateringControl_SetpointText;
    db.Get_Setpoint<int>(Parameter, WateringControl, 1, sqlError);

    Parameter = "Vanding1_Start";
    db.Get_Setpoint<int>(Parameter, Status_VandingStartTime1, 9, sqlError);

    Parameter = "Vanding2_Start";
    db.Get_Setpoint<int>(Parameter, Status_VandingStartTime2, 20, sqlError);

    Parameter = "VandingsTid_Ventil1";
    db.Get_Setpoint<int>(Parameter, Status_VandingsTidValve1, 20, sqlError);

    Parameter = "VandingsTid_Ventil2";
    db.Get_Setpoint<int>(Parameter, Status_VandingsTidValve2, 20, sqlError);

    Parameter = "VandingsTid_Ventil3";
    db.Get_Setpoint<int>(Parameter, Status_VandingsTidValve3, 20, sqlError);

    Parameter = "VandingsTid_Ventil4";
    db.Get_Setpoint<int>(Parameter, Status_VandingsTidValve4, 20, sqlError);

    Parameter = "VinduesKontrol";
    db.Get_Setpoint<int>(Parameter, VinduesKontrol, 5, sqlError);

    if (VinduesKontrol =! 5)
    {
        spdlog::info("Vindues kontrol aktiveret med kode {}", VinduesKontrol);
        ManageWindows(VinduesKontrol);
    }


    Parameter = "VinduesStyring";
    db.Get_Setpoint<int>(Parameter, VinduesStyring, 20, sqlError);
    if (VinduesStyring == 0)
    {
        AutoWindow = 1;
    }
    else
    {
        AutoWindow = 0;
    }

    Parameter = "AarsTid";
    db.Get_Setpoint<int>(Parameter, Aarstid, 20, sqlError);
    if (Aarstid == 1)
    {
        WinterTent = false;
        spdlog::info("Aarstid sætpunkt = 1 -> Sommer temperatursensor i Orangeri");
    }
    else
    {
        WinterTent = true;
        spdlog::info("Aarstid sætpunkt <> 1 -> Vinter temperaturmåling i telt");
    }

    Parameter = "DagStart";
    db.Get_Setpoint<int>(Parameter, DayStartHour, 8, sqlError);

    Parameter = "NatStart";
    db.Get_Setpoint<int>(Parameter, NightStartHour, 20, sqlError);

    Parameter = "KomfortPeriodeSlut";
    db.Get_Setpoint<int>(Parameter, ComfortDayEndHour, 20, sqlError);

    Parameter = "KomfortPeriodeStart";
    db.Get_Setpoint<int>(Parameter, ComfortDayStartHour, 16, sqlError);

    Parameter = "TvangslukkedeVinduer_Minutter";
    db.Get_Setpoint<int>(Parameter, StopVentsButton_minutes, 20, sqlError);

    Parameter = "VinterHeatingPValue";
    db.Get_Setpoint<double>(Parameter, P_HeatUp, 20, sqlError);

    Parameter = "VinterHeatingLagTime";
    db.Get_Setpoint<int>(Parameter, LagTime_HeatUp, 20, sqlError);


    Parameter = "StartLuftPumpeTemperatur";
    db.Get_Setpoint<int>(Parameter, AirPumpTemperature, 18, sqlError);

    //spdlog::info("ReadSetpoints gennemført");

}



time_t convertToTimeT(const std::string& datetime)
{
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%y-%m-%d %H:%M");
    if (ss.fail())
    {
        throw std::runtime_error("Failed to parse datetime string.");
    }

    // Adjust 2-digit year to 4-digit manually (assume 2000–2099 range)
    if (tm.tm_year < 100)
    {
        tm.tm_year += 100;  // Because tm_year is years since 1900
    }

    tm.tm_sec = 0;
    tm.tm_hour = tm.tm_hour-1;  //????
    return std::mktime(&tm);
}


/*
Henter værdier gemt i xml fil efter restart af programmet. Derved sikres det at tidsafhængige
handlinger gennemføres korrekt i forold til tidligere. Fx vanding i rigtigt interval,
beregning af midelværdier osv.
*/

void LoadDBValues()
{
    try
    {
        spdlog::info("LoadDBValues Start");

        std::string sqlError;
        string readValue;
        double readDouble;
        string updateTime;
        bool result;


        Status_LastStartTime1 = nu;
        result = db.Get_CurrentValue<string>(LastWateringTime1_Parameter, readValue, updateTime, sqlError);
        spdlog::info("SidsteVandingsTime1: Result {} Value: {}", result, readValue);
        if (result && readValue.length() > 2)
        {
            Status_LastStartTime1 = convertToTimeT(readValue);
            std::string formatted_time = showDate(convertToTimeT(readValue));
            spdlog::info("SidsteVandingsTime1: {}", formatted_time);
        }

        Status_LastStartTime2 = nu;
        result = db.Get_CurrentValue<string>(LastWateringTime2_Parameter, readValue, updateTime, sqlError);
        spdlog::info("SidsteVandingsTime2: Result {} Value: {}", result, readValue);
        if (result && readValue.length() > 2)
        {
            Status_LastStartTime2 = convertToTimeT(readValue);
            std::string formatted_time = showDate(convertToTimeT(readValue));
            spdlog::info("SidsteVandingsTime2: {}", formatted_time);
        }

        result = db.Get_CurrentValue<string>(LastWatering1_Parameter, readValue, updateTime, sqlError);
        spdlog::info("Read SidsteVandingsLabel1: Result {} Value: {}", result, readValue);
        if (result && readValue.length() > 2)
        {
            Status_LastWateringValve1 = convertToTimeT(readValue);
            Status_LastStartTime1 = Status_LastWateringValve1;
            std::string formatted_time = showDate(Status_LastWateringValve1);
            spdlog::info("LastWatering1: {}", formatted_time);
        }

        result = db.Get_CurrentValue<string>(LastWatering2_Parameter, readValue, updateTime, sqlError);
        if (updateTime.length() > 2)
        {
            Status_LastWateringValve2 = convertToTimeT(readValue);
            Status_LastStartTime2 = Status_LastWateringValve2;
        }

        result = db.Get_CurrentValue<string>(LastWatering3_Parameter, readValue, updateTime, sqlError);
        if (updateTime.length() > 2)
        {
            Status_LastWateringValve3 = convertToTimeT(readValue);
        }

        result = db.Get_CurrentValue<string>(LastWatering4_Parameter, readValue, updateTime, sqlError);
        if (updateTime.length() > 2)
        {
            Status_LastWateringValve4 = convertToTimeT(readValue);
        }

        std::string formatted_time = showDate(Status_LastWateringValve1);
        spdlog::info("LastWatering1: {}", formatted_time);
        formatted_time = showDate(Status_LastWateringValve2);
        spdlog::info("LastWatering2: {}", formatted_time);
        formatted_time = showDate(Status_LastWateringValve3);
        spdlog::info("LastWatering3: {}", formatted_time);
        formatted_time = showDate(Status_LastWateringValve4);

        spdlog::info("LastWatering4: {}", formatted_time);



        result = db.Get_CurrentValue<double>(WindowsPct_Parameter, readDouble, updateTime, sqlError);
        CurrentStatus_Window1 = readDouble;
        spdlog::info("Vinduesåbning  {:.1f}",CurrentStatus_Window1);

        result = db.Get_CurrentValue<double>(WaterFlow1_Total_Parameter, readDouble, updateTime, sqlError);
        WaterTotalFlow1 = readDouble;
        spdlog::info("WaterTotalFlow1 {}", WaterTotalFlow1);

        result = db.Get_CurrentValue<double>(WaterFlow2_Total_Parameter, readDouble, updateTime, sqlError);
        WaterTotalFlow2 = readDouble;
        spdlog::info("WaterTotalFlow2 {}", WaterTotalFlow2);

        result = db.Get_CurrentValue<double>(WaterFlow3_Total_Parameter, readDouble, updateTime, sqlError);
        WaterTotalFlow3 = readDouble;
        spdlog::info("WaterTotalFlow3 {}", WaterTotalFlow3);

        result = db.Get_CurrentValue<double>(RainAccumDay_Parameter, readDouble, updateTime, sqlError);
        RainDayAccum = readDouble;
        spdlog::info("RainAccumDay {}", RainDayAccum);

        result = db.Get_CurrentValue<double>(RainAccumHour_Parameter, readDouble, updateTime, sqlError);
        RainHourAccum = readDouble;
        spdlog::info("RainHourAccum {}", RainHourAccum);

        Stat_TimeWaterValve1 = GetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve1",0);
        spdlog::info("Stat_TimeWaterValve1 {}",Stat_TimeWaterValve1);
        Stat_TimeWaterValve2 = GetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve2",0);
        spdlog::info("Stat_TimeWaterValve2 {}",Stat_TimeWaterValve2);
        Stat_TimeFreshWaterToTank = GetXMLValueDouble("/Orangeri/RessourceUse/FreshWaterToTank",0);
        spdlog::info("Stat_TimeFreshWaterToTank {}", Stat_TimeFreshWaterToTank);
        Stat_TimeHeat = GetXMLValueDouble("/Orangeri/RessourceUse/TimeHeatOn",0);
        spdlog::info("Stat_Heat {} ", Stat_TimeHeat);
        Stat_TimeLight = GetXMLValueDouble("/Orangeri/RessourceUse/LightGreenhouse",0);
        spdlog::info("Stat_Light {}",Stat_TimeLight);


        spdlog::info("LoadDbValues Slut");
    }
    catch (...)
    {
        spdlog::error(std::string("Fejl ved LoadDbValues"));
    }
}

/*
 * Gemmer værdier løbende, så der kan laves genstart med korrekte værdier
*/

void SaveDBValues()
{

    try
    {
        cout << "SaveDBValues" << endl;
        bool myResult;
        //myResult = myopenXMLFile();
        //cout << "Efter openXMLFile: " << myResult << endl;


        /*Update_CurrentValue(const std::string& parameter,
                                   const T& value,
                                   std::string& sqlError)*/
        string err;
        db.Update_CurrentValue<string>(LastWatering1_Parameter, showDate(Status_LastWateringValve1), err);
        db.Update_CurrentValue<string>(LastWatering2_Parameter, showDate(Status_LastWateringValve2), err);
        db.Update_CurrentValue<string>(LastWatering3_Parameter, showDate(Status_LastWateringValve3), err);
        db.Update_CurrentValue<string>(LastWatering4_Parameter, showDate(Status_LastWateringValve4), err);

        db.Update_CurrentValue<string>(LastWateringTime1_Parameter, showDate(Status_LastStartTime1), err);
        db.Update_CurrentValue<string>(LastWateringTime2_Parameter, showDate(Status_LastStartTime2), err);
        /*db.Update_CurrentValue<string>(RainDayAccum_Parameter, RainDayAccum);
        db.Update_CurrentValue<string>(RainHourAccum_Parameter, RainHourAccum);
        */
        /*SetXMLValueDouble("/Orangeri/StatusValues/LastWatering1", Status_LastWateringValve1);
        SetXMLValueDouble("/Orangeri/StatusValues/LastWatering2", Status_LastWateringValve2);
        SetXMLValueDouble("/Orangeri/StatusValues/LastWatering3", Status_LastWateringValve3);
        SetXMLValueDouble("/Orangeri/StatusValues/LastWatering4", Status_LastWateringValve4);
        SetXMLValueDouble("/Orangeri/StatusValues/WindowsPct",CurrentStatus_Window1);
        */
        /*
        SetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve1",Stat_TimeWaterValve1);
        SetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve2",Stat_TimeWaterValve2);
        SetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve3",Stat_TimeWaterValve3);
        SetXMLValueDouble("/Orangeri/RessourceUse/TimeWaterValve4",Stat_TimeWaterValve4);
        SetXMLValueDouble("/Orangeri/RessourceUse/FreshWaterToTank",Stat_TimeFreshWaterToTank);
        SetXMLValueDouble("/Orangeri/RessourceUse/TimeHeatOn",Stat_TimeHeat);
        SetXMLValueDouble("/Orangeri/RessourceUse/LightGreenhouse",Stat_TimeLight);
        */
        //SetXMLValueDouble("/Orangeri/StatisticsValues/RainDayAccum", RainDayAccum);
        //SetXMLValueDouble("/Oranger i/StatisticsValues/RainHourAccum", RainHourAccum);
        //myResult = mycloseXMLFile();
        //cout << "SaveXMLValues afsluttet: " << myResult << endl;
    }
    catch (...)
    {
        spdlog::error(std::string("Fejl ved SaveXML"));
    }
}

void ControlGreenhouseVerySlowTimer()
{
    ReadSetpoints();
    SaveDBValues();

}

//Log af diverse værdier med mellemrum
//Kaldes ved skift af time.
void WriteInfoToLog()
{
    spdlog::info("WriteInfoToLog");

    if (WinterTent == 1)
    {
        cout << "System i vintermode med lang temperatursensor" << endl;
        spdlog::info("Aarstid sætpunkt = 1 -> Vinter med telt og temperatursensor i vinterteltet");
    }
    else
    {
        cout << "System i sommermode";
        spdlog::info("Aarstid sætpunkt <> 1 -> Sommer med temperaturmåling i selve drivhuset");
    }

    spdlog::info("WaterTotalFlow1: {}", WaterTotalFlow1);
    spdlog::info("WaterTotalFlow2: {}", WaterTotalFlow2);
    spdlog::info("WaterTotalFlow3: {}", WaterTotalFlow3);


}

void WriteStatus()
{

    /*
        int count;
        char buffer[100];

        spdlog::info("Relæ status");
        count = sprintf(buffer, "Varme tændt: {}", HeatOn);
        spdlog::info(buffer);

        count = sprintf(buffer, "Udluftning med pumpe/Kunstlys: {}", LightOn);
        spdlog::info(buffer);

        count = sprintf(buffer, "Vand til palletank: {}", FreshWaterToTankOn);
        spdlog::info(buffer);

        count = sprintf(buffer, "VandingsValve 1:  {}", WaterValve1On);
        spdlog::info(buffer);
        count = sprintf(buffer, "VandingsValve 2:  {}", WaterValve2On);
        spdlog::info(buffer);
        count = sprintf(buffer, "VandingsValve 3:  {}", WaterValve3On);
        spdlog::info(buffer);
        count = sprintf(buffer, "VandingsValve 4:  {}", WaterValve4On);
        spdlog::info(buffer);
    */

}

//Starter vanding ved at sætte diverse parametre
void Vanding(int WaterTimerNumber)
{
    spdlog::info("I Vanding procdure. TimerNummer {}", WaterTimerNumber);

    //Gememr tid for start af timer 1 eller 2
    if (WaterTimerNumber == 1)
    {
        Status_LastStartTime1 = nu;
    }
    else
    {
        Status_LastStartTime2 = nu;
    }

    if (Status_VandingsTidValve1 > 0) //Hvis antal minutter er > 0 og vanding ikke er i gang
    {
        spdlog::info("Status_VandingsTidValve1 {}", Status_VandingsTidValve1);
        Status_WaterValve1 = 1;  //Valve 1 skal åbnes
        Status_StopWateringValve1 =  nu + (Status_VandingsTidValve1*60*AddExtraWaterTime_Greenhouse);  //Beregning af tidspunkt der stoppes
        Status_LastWateringValve1 =  nu;  //Sidste vandingstidspunkt gemmes
    }

    if (Status_VandingsTidValve2 > 0) //Er sat i Blynk som antal minutter der skal vandes
    {
        spdlog::info("Status_VandingsTidValve2 {}", Status_VandingsTidValve2);
        Status_WaterValve2 = 1;
        Status_StopWateringValve2 =  nu  + Status_VandingsTidValve2*60*AddExtraWaterTime_Greenhouse;
        Status_LastWateringValve2 =  nu ;
    }

    if (Status_VandingsTidValve3 > 0)
    {
        spdlog::info("Status_VandingsTidValve3 {}", Status_VandingsTidValve3);
        Status_WaterValve3 = 1;
        Status_StopWateringValve3 =  nu  + Status_VandingsTidValve3*60*AddExtraWaterTime_Garden;
        Status_LastWateringValve3 =  nu ;
    }

    if (Status_VandingsTidValve4 > 0)
    {
        spdlog::info("Status_VandingsTidValve4 {}", Status_VandingsTidValve4);
        Status_WaterValve4 = 1;
        Status_StopWateringValve4 =  nu  + Status_VandingsTidValve4*60*AddExtraWaterTime_Garden;
        Status_LastWateringValve4 =  nu ;
    }
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
        gpio_write(pi, LedPin, 0);
        LedPinOn = 0;
    }
    else
    {
        gpio_write(pi, LedPin, 1);
        LedPinOn = 1;
    }

    cout << endl;
    cout << "********* Nyt Slow loop ******\n";
    cout << endl;


    OpenWindowsOneStepMore(Vinduer_1TrinMere);

    msg1 = showDate(nu);
    msg2 = showDate(LastRunTime);

    string errmsg;
    //Hvis en ny dag initialiseres tællere, summer m.m.
    if (LastDayChanged !=  getDay(nu))
    {

        spdlog::info(std::string("Ny dag initialiseres"));

        cout << "Dagsværdier er forskellige \n";
        GreenhouseTempAvgDay = RoundToDecimal(GreenhouseDayTempStats.GetMean(),1); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding
        GreenhouseTempMinDay = RoundToDecimal(GreenhouseDayTempStats.GetMin(),1); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding
        GreenhouseTempMaxDay = RoundToDecimal(GreenhouseDayTempStats.GetMax(),1); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding

        RainDayAccum = RoundToDecimal(RainDayStats.GetSum(),1); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding
        WeatherTempAvgDay = RoundToDecimal(WeatherDayTempStats.GetMean(),1); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding
        WeatherTempMinDay = RoundToDecimal(WeatherDayTempStats.GetMin(),1); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding
        WeatherTempMaxDay = RoundToDecimal(WeatherDayTempStats.GetMax(),1); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding

        GreenhouseDayTempStats.clear();
        WeatherDayTempStats.clear();
        RainDayStats.clear();
        AirFlowStats.clear();
        db.Update_HistoryValue<double>(GreenhouseTempAvgDay_Text, GreenhouseTempAvgDay, errmsg);
        db.Update_HistoryValue<double>(GreenhouseTempMinDay_Text, GreenhouseTempMinDay, errmsg);
        db.Update_HistoryValue<double>(GreenhouseTempMaxDay_Text, GreenhouseTempMaxDay, errmsg);

        db.Update_HistoryValue<double>(WeatherTempAvgDay_Text, WeatherTempAvgDay, errmsg);
        db.Update_HistoryValue<double>(WeatherTempMinDay_Text, WeatherTempMinDay, errmsg);
        db.Update_HistoryValue<double>(WeatherTempMaxDay_Text, WeatherTempMaxDay, errmsg);

        db.Update_HistoryValue<double>(RainSumDay_Text, RainDayAccum, errmsg);
        db.Update_HistoryValue<double>(WaterFlow1Day_Text, WaterTotalFlow1, errmsg);
        db.Update_HistoryValue<double>(WaterFlow2Day_Text, WaterTotalFlow2, errmsg);
        db.Update_HistoryValue<double>(WaterFlow3Day_Text, WaterTotalFlow3, errmsg);

        db.Update_HistoryValue<double>(HeatTimeOn_Text, Stat_TimeHeat, errmsg);
        db.Update_HistoryValue<double>(FreshWaterToTankTime_Text, Stat_TimeFreshWaterToTank, errmsg);
        db.Update_HistoryValue<double>(WaterValve1Time_Text, Stat_TimeWaterValve1, errmsg);
        db.Update_HistoryValue<double>(WaterValve2Time_Text, Stat_TimeWaterValve2, errmsg);
        db.Update_HistoryValue<double>(WaterValve3Time_Text, Stat_TimeWaterValve3, errmsg);
        db.Update_HistoryValue<double>(WaterValve4Time_Text, Stat_TimeWaterValve4, errmsg);



        LastDayChanged = getDay(nu);  //Bruges til holde styr på om daglige data skal nulstilles
        RainDayAccum = 0; //nulstiller regntæller
        WaterTotalFlow1 = 0; //Nulstille vandforbrug pr dag
        WaterTotalFlow2 = 0;
        WaterTotalFlow3 = 0;
        Stat_TimeHeat = 0;
        Stat_TimeWaterValve1 = 0;
        Stat_TimeWaterValve2 = 0;
        Stat_TimeWaterValve3 = 0;
        Stat_TimeWaterValve4 = 0;
        Stat_TimeFreshWaterToTank = 0;
    }
    int CurrentHour;

    //En ny time startes
    if (LastHourChanged !=  getHour(nu))
    {
        spdlog::info(std::string("Nye time initialiseres"));

        WriteInfoToLog(); //Skriver diverse opsætnings værdier til log

        try
        {
            CurrentHour = round(getHour(nu));
            Rain24h[CurrentHour] = RainHourAccum;
            string mystr =  showDate(nu) + to_string(CurrentHour);

            GreenhouseTempAvgHour = RoundToDecimal(GreenhouseHourTempStats.GetMean(),1); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding
            WeatherTempAvgHour = RoundToDecimal(WeatherHourTempStats.GetMean(),1); //holder værdi for gennemsnit forrige dag. Benyttes som udgangspunkt for at vurderer behov for ekstra vanding
            RainHourAccum = RoundToDecimal(RainHourStats.GetSum(),1);

            db.Update_HistoryValue<double>(TempAvgHour_Text, GreenhouseTempAvgHour, errmsg);
            db.Update_HistoryValue<double>(WeatherTempAvgHour_Text, WeatherTempAvgHour, errmsg);
            db.Update_HistoryValue<double>(RainSumHour_Text, RainHourAccum, errmsg);

            WeatherHourTempStats.clear();
            WeatherHourTempStats.clear();
            GreenhouseHourTempStats.clear();
            RainHourStats.clear();

            mystr = "Regn 24h: ";
            for( unsigned int a = 0; a < 24; a++)
            {
                mystr = mystr + ", " + to_string(Rain24h[a]);
            };
            cout << mystr << endl;
            AppendDataToFile();
            cout << "TempStat timeskift ved time " << getHour(nu) << " Gennemsnit " << endl;
            double RainSum = 0;
            cout << "Opdaterer regnsummer" << endl;
            //for( unsigned int a = CurrentHour; a = CurrentHour-3; a=a-1){
            for (int r = CurrentHour - 2; r >= CurrentHour; r++)
            {
                RainSum = RainSum + Rain24h[r];
                cout << "CurrentHour " << CurrentHour << " RainHour " << Rain24h[r] << " RainSum " << RainSum << endl;
            };
            cout << "Rain3h " << RainSum << endl;
            db.Update_CurrentValue<double>("Rain3h_Virtual_Pin", RainSum, errmsg);


            RainHourAccum = 0; //nulstiller regntæller

            LastHourChanged = getHour(nu);

        }

        catch (...)
        {
            cout << "Fejl ved diverse routiner ved timeskift " << endl;
            spdlog::error("Fejl ved diverse routiner ved timeskift");

        }

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
        Setpoint_TemperatureNow = 10;
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

    Setpoint_TemperatureNow = 8;


    Error_HeatUp = Error_HeatUp + (Setpoint_TemperatureNow-Greenhouse_temperature);

    //Nedsætter fejl sum hvis temperaturen er summet for høj. Dette for at undgå at der komemr for lave temperaturer
    //Det betyder at I justering kun virker i begrænset omfang når tempearaturen er for høj.
    if (Error_HeatUp < -40)
    {
        Error_HeatUp = -50;
    }

    //cout << "Error_HeatUp  " << Error_HeatUp << "\n";
    spdlog::info("Greenhouse_temperature {} Setpoint_TemperatureNow {}", Greenhouse_temperature,Setpoint_TemperatureNow);

    //bestemmer om varme skal tændes eller slukkes
    if (Greenhouse_temperature < Setpoint_TemperatureNow)
    {
        // AddHeatTime = int((Setpoint_TemperatureNow-Greenhouse_temperature)*P_HeatUp + I_HeatUp/100*Error_HeatUp);
        AddHeatTime = int(LagTime_HeatUp + (Setpoint_TemperatureNow-Greenhouse_temperature)*P_HeatUp);
        spdlog::info("Tid for opvarmning bestemmes. AddHeatTime: {} ", AddHeatTime);
        StopHeating =  nu  + AddHeatTime;
        spdlog::info("Temperatur mindre end sætpunkt. Varme tændes  Temperatur  {}  setpoint_now {} Stop tidspunkt {:%H:%M:%S}",
                     Greenhouse_temperature, Setpoint_TemperatureNow, fmt::localtime(StopHeating));
    };
    if (Greenhouse_temperature >= Setpoint_TemperatureNow &&  HeatOn == 1)
    {
        StopHeating = 0;
        spdlog::info("Temperatur større end sætpunkt. Varme slukkes  Temperatur  {}  setpoint_now {} ",Greenhouse_temperature, Setpoint_TemperatureNow);
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

    //myBME680Result = GetBME680Data();
    /*std::cout << "Temperataure: " << myBME680Result.temperature << std:: endl;
    std::cout << "Humidity:     " << myBME680Result.humidity << std:: endl;
    std::cout << "Pressure:     " << myBME680Result.pressure << std:: endl;
    std::cout << "GasResistance:" << myBME680Result.gasResistance << std:: endl;

    ControlBoxTemperature = myBME680Result.temperature;
    cout << "ControlBoxTemperature " << ControlBoxTemperature << endl;
    ControlBoxPressure = myBME680Result.pressure;
    cout << "ControlBoxPressure " << ControlBoxPressure << endl;
    */

    string err;
    int i = 1;
    do
    {
        double AdcTest = ExPi_ReadADC(i);
        db.Update_CurrentValue<string>("ADC" + to_string(i), to_string(AdcTest), err);
        i++;
    }
    while (i < 9);

    AirFlowValue = ExPi_ReadADC(AirFlowPin) * 12.96;

    //spdlog::info("AirFlowValue {}",AirFlowValue);

    //Beregning af automatisk vanding
    //Ekstra tid til vanding ved høje temperaturer
    //Skal baseres på gennemsnit sidste dag, når den virker
    double factor = (Greenhouse_temperature - 26)*0.1;
    if (factor > 0 )
    {
        AddExtraWaterTime_Greenhouse = factor;
    }
    else
    {
        AddExtraWaterTime_Greenhouse = 1;
    }

    factor = ( Weather_temperature - 26)*0.1;
    if (factor > 0 )
    {
        AddExtraWaterTime_Garden = factor;
    }
    else
    {
        AddExtraWaterTime_Garden = 1;
    }



    cout << "Væksthus gennemsnits temperatur " << GreenhouseTempAvgDay << endl;

    tm*StartVanding2 = localtime(&nu);
    StartVanding2->tm_hour = getHour(Status_VandingStartTime2);

    /*
    Status_LastStartTime1 sættes når en vanding kan gennemføres pga tid.
    Vanding vil kun ske når Status_VandingsTidValve1 > 0
    Status_LastStartTime1 indlæses ved start af program fra database. Vanding gennemføres når timen paceres,
    derefter sættes Status_LastStartTime1 til aktuel tid. Når den er sat skal der være
    gået mere end 2 timer før den samme vandingstid kan startes igen. Sikre at der ikke vandes flere gange
    indenfor den samme time.
    */

    if (getHour(nu) == Status_VandingStartTime1 || getHour(nu) == Status_VandingStartTime2)
    {
        //spdlog::info("Vanding 1 bestemmes Hour: {} NU = {}, LastStartTime1 = {}, Difference = {} ", getHour(nu), nu , Status_LastStartTime1, nu - Status_LastStartTime1);
        //spdlog::info("Vanding 2 bestemmes Hour: {} NU = {}, LastStartTime2 = {}, Difference = {} ", getHour(nu), nu , Status_LastStartTime2, nu - Status_LastStartTime2);
    }

    if ((getHour(nu) == Status_VandingStartTime1) && ((nu - Status_LastStartTime1 > 60 * 60 * 2)))
    {
        spdlog::info("Vanding 1.1 bestemmes NU = {}, LastStartTime1 = {}, Difference = {} ", nu, Status_LastStartTime1, nu - Status_LastStartTime1);
        Vanding(1); //Timer1
    }

    if ((getHour(nu) == Status_VandingStartTime2) && ((nu - Status_LastStartTime2 > 60 * 60 * 2) ))
    {
        spdlog::info("Vanding 1.2 bestemmes NU = {}, LastStartTime2 = {}, Difference = {} ", nu, Status_LastStartTime2, nu - Status_LastStartTime2);
        Vanding(2);  //Timer2
    }

    //Tænder for pumpen hvis en af ventilerne er åbne eller knap for havevanding er benyttet

    Status_WaterPump = 0;

    if (Status_WaterValve1 == 1 or Status_WaterValve2 == 1 or Status_WaterValve3 == 1 or Status_WaterValve4 == 1)
    {
        spdlog::info("Pumpe tændes for status_WaterValve 1: {}  2: {}   3: {}  4: {}", Status_WaterValve1, Status_WaterValve2, Status_WaterValve3, Status_WaterValve4);
        Status_WaterPump = 1;
    }

    //Læser værdier på flyder i palletank.
    //Flyderen læses med værdier i WaterLevel1 og WaterLevel2
    cout << "Læsning af WaterLevel" << endl;
    WaterReadTop = gpio_read(pi, WaterLevelPin1);
    WaterReadButtom = gpio_read(pi, WaterLevelPin2);
    spdlog::info("Niveausensorer læses: WaterReadTop {}  WaterReadButtom {}", WaterReadTop, WaterReadButtom);

    GetWaterFlow1();
    GetWaterFlow2();
    GetWaterFlow3();

    //Hvis flyderen er løftet op måles høj værdi på A8 eller er A8 = 0

    if (WaterReadButtom == 1)
    {
        Status_WaterLevel1 = false;
        Status_FreshWaterToTankValve = 1; // åbner ventil der tilfører vand til vandtank
        WaterLevel1_Msg = "Palletank er snart tom";

        spdlog::info("Palletank er snart tom");
    }
    //cout << "Inden WaterReadTop == 1" << endl;
    if (WaterReadTop == 1)
    {
        Status_WaterLevel1 = true;
        Status_FreshWaterToTankValve = 0;  //Ikke behov for mere vand til palletank da beholderen er fyldt så flyderen er oppe
        WaterLevel1_Msg = "Palletank er fyldt til flyder niveau";
    }

    //cout << "Inden RainRead" << endl;

    //cout << "Lysniveau " << LightValue << "\n";

    // Rain get start state
    double RainRead =  GetRain();
    RainDayStats.AddData(RainRead);
    RainHourStats.AddData(RainRead);

    RainDayAccum = RainDayAccum + RainRead;
    RainHourAccum = RainHourAccum + RainRead;

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

        Greenhouse_temperature = Probe02.getTemp();  //Orange
        //Greenhouse_temperature = Probe07.getTemp();
        Extra1_temperature = Probe04.getTemp();
        Extra2_temperature = Probe07.getTemp();
        spdlog::info("Temperaturmålinger Ekstra1  {}  Ekstra2 {}", Extra1_temperature,Extra2_temperature);


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



    unsigned int frac;


    //if (isnan(Greenhouse_temperature) == 0)  //jeaa
    //GreenhouseDayTempStats.AddData(Greenhouse_temperature);
    //WeatherDayTempStats.AddData(Weather_temperature);

    //cout << "Væksthus temperatur " << Greenhouse_temperature << "\n";
    //cout << "TempStat efter add " << " Gennemsnit " << GreenhouseDayTempStats.GetMean() << "\n";



    //Sætter relæer efter status værdier der er beregnet eller overstyret i app
    //cout << "Klar til at sætte relæer" << endl;
    //cout << "3 Status_FreshWaterToTankValve " << Status_FreshWaterToTankValve << endl;


    //SetHeat(Status_HeatUp);
    if (!WinterTent)
    {
        SetLight(Status_LightGreenhouse);
    }
    spdlog::info("Status_FreshWaterToTankValve {}", Status_FreshWaterToTankValve);
    SetFreshWatertoTank(Status_FreshWaterToTankValve);

    GreenhouseTempAvgDay = GreenhouseDayTempStats.GetMean();
    WeatherTempAvgDay = WeatherDayTempStats.GetMean();


    //cout << "Status_FreshWaterToTankValve " << Status_FreshWaterToTankValve << endl;
    //cout << "Status_WaterPump " << Status_WaterPump  << endl;

    LastRunTime =  nu;  //Variable der holder styr på hornår der er kørt en styring

    cout << endl;
    cout << "********* Slut Slow loop ******\n";
    cout << endl;

    SetupFinished = 1;  //problmer med at interrupt mm starter for tidligt derfor sættes denne til false ved start og true efter første runde her

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


void WriteStatusInfo()
{
    //spdlog::info("GreenhouseTempAvgDay {:.1f}", TempAvgDay);
    //spdlog::info("WeatherAvgDay {:.1f}", WeatherTempAvgDay);
    if (AddExtraWaterTime_Greenhouse > 1)
    {
        spdlog::info("Øget vandstid pga af temperatur i væksthuset. Tempertur nu: {:.1f}, Ekstra tid: {:.1f}", Greenhouse_temperature, AddExtraWaterTime_Greenhouse);
    }
    if (AddExtraWaterTime_Garden > 1)
    {
        spdlog::info("Øget vandstid pga af temperatur i haven. Tempertur nu: {:.1f}, Ekstra tid: {:.1f}", Weather_temperature, AddExtraWaterTime_Garden);
    }

    spdlog::info("Temperature4Vent {} Setpoint_Udluftning {} ",Temperature4Vent, Setpoint_Udluftning);

    spdlog::info("Status_VandingStartTime1: {}, Status_VandingStartTime2: {}", Status_VandingStartTime1, Status_VandingStartTime2);

}

//Kontrollerer forbindelsen til Blynk. Genopretter forbindelse hvis den er mistet.
//Kaldes af timer med mellemrum for at opdaterer forbindelsen om nødvendigt hvis den mistes af forskellige grunde

void FastTimerEvent()
{
    nu = time(NULL);
    //spdlog::info("FastTimerEvent {:%H:%M:%S}", fmt::localtime(nu));
    ControlGreenhouseFastTimer();
    UpdateBlynkAfterFastTimer();  //Opdater data til Blynk}
    //LastFastTimerEvent = nu;
}

void SlowTimerEvent()
{
    nu = time(NULL);
    spdlog::info("SlowTimerEvent {:%H:%M:%S}", fmt::localtime(nu));

    //CheckBlynk();
    ControlGreenhouseSlowTimer();
    UpdateBlynkAfterSlowTimer();
    //LastSlowTimerEvent = nu;

    if (AfterOrangeriSetup== 0)
    {
        //WaterPumpButtonActive = 0;
        // StopVentsButtonAc = 0;
        //
        //DispButtonActive = 0;
        HeatOn = 0;
        StopHeating = 0;
        AfterOrangeriSetup= 1;
    }

}


void VerySlowTimerEvent()
{
    nu = time(NULL);
    spdlog::info("VerySlowTimerEvent {:%H:%M:%S}", fmt::localtime(nu));
    ControlGreenhouseVerySlowTimer();

}


//---------------------------------Setup rutine --------------------------------





void OrangeriSetup(void)
{

    //wiringPiSetupGpio();


    AfterOrangeriSetup = 0;
    ExpanderPiSetup();

    //slukker for pumpe hvis den skulle være igang
    Status_WaterPump = 0;
    ExPi_IOPortWrite(Pi_Relay_WaterPump, Status_WaterPump);

    //Slukker luftpumpe/kunstlys ved opstart
    Status_LuftPumpe = 0;
    ExPi_IOPortWrite(Pi_Relay_LightGreenhouse, Status_LuftPumpe);

    nu = time(NULL);
    LastDayChanged = getDay(nu);

    //Weather_shtfile = sht31_open(sht31_open(SHT31_INTERFACE_ADDR,SHT31_DEFAULT_ADDR));
    CPUTemperature = getCPUTemperature();

    set_mode(pi, WaterLevelPin1, PI_INPUT);

    set_mode(pi, WaterFlow1_PIN, PI_INPUT);
    set_pull_up_down(pi, WaterFlow1_PIN, PI_PUD_DOWN); // Enable pull-down resistor

    set_mode(pi, WaterFlow2_PIN, PI_INPUT);
    set_pull_up_down(pi, WaterFlow2_PIN, PI_PUD_DOWN); // Enable pull-down resistor

    set_mode(pi, WaterFlow3_PIN, PI_INPUT);
    set_pull_up_down(pi, WaterFlow3_PIN, PI_PUD_DOWN); // Enable pull-down resistor

    set_mode(pi, RAIN_GAUGE_PIN, PI_INPUT);
    set_pull_up_down(pi, RAIN_GAUGE_PIN, PI_PUD_DOWN); // Enable pull-up resistor
    //gpioGlitchFilter(pi, RAIN_GAUGE_PIN, 300000);



    //Interrupt porte benytter standard porte på GPIO proten og ikke på ExpanderPi


    //ExPi_IOPortWrite(Pi_Relay_LED_WaterPump, 0);
    //ExPi_IOPortWrite(Pi_Relay_LED_StopVents, 0);
    //ExPi_IOPortWrite(Pi_Relay_LED_Disp, 0);


    //Opsætning af wand flow målinger

//jeaa 0503

    /*  gpioSetISRFunc(WaterFlow1_PIN, RISING_EDGE, 0, pulseCallbackFlow1);
      gpioSetISRFunc(WaterFlow2_PIN, RISING_EDGE, 0, pulseCallbackFlow2);
      gpioSetISRFunc(WaterFlow3_PIN, RISING_EDGE, 0, pulseCallbackFlow3);
      gpioSetISRFunc(RAIN_GAUGE_PIN, RISING_EDGE, 0, countRainPulse);

    */


    // Attach the callback to count pulses
    /*  gpioSetAlertFunc(WaterFlow1_PIN, pulseCallbackFlow1);
        gpioSetAlertFunc(WaterFlow2_PIN, pulseCallbackFlow2);
        gpioSetAlertFunc(WaterFlow3_PIN, pulseCallbackFlow3);
        gpioSetAlertFunc(RAIN_GAUGE_PIN, countRainPulse);
    */

    callback(pi, WaterFlow1_PIN, EITHER_EDGE, pulseCallbackFlow1);
    callback(pi, WaterFlow2_PIN, EITHER_EDGE, pulseCallbackFlow2);
    callback(pi, WaterFlow3_PIN, EITHER_EDGE, pulseCallbackFlow3);
    callback(pi, RAIN_GAUGE_PIN, EITHER_EDGE, countRainPulse);


    //InitBME680();  //Init BME680 sensor

    set_mode(pi, LedPin, PI_OUTPUT);
    set_mode (pi, GPIO_LED_WaterPump, PI_OUTPUT);
    set_mode (pi, GPIO_LED_StopVent, PI_OUTPUT);
    set_mode (pi, GPIO_LED_Disp, PI_OUTPUT) ;
    gpio_write(pi, GPIO_LED_WaterPump, 0);
    gpio_write(pi, GPIO_LED_StopVent, 0);
    gpio_write(pi, GPIO_LED_Disp, 0);

    gpio_write(pi, LedPin, 0);

    LastSetupRunTime =  nu;   //Holder sidste gang der blev kørt setup rutine. Altså upload eller wtd reset
    LastRunTime  =  nu;    //Variable der holder styr på hornår der er kørt en styring


//------------------Relæ output ----------------------------------

    //opsætning af statistik variable
    GreenhouseDayTempStats.clear(); //explicitly start clean
    WeatherDayTempStats.clear(); //explicitly start clean
    RainDayStats.clear(); //explicitly start clean
    RainHourStats.clear(); //explicitly start clean
    RainDayAccum = 0; //nulstiller regntæller
    RainHourAccum = 0; //nulstilelr regntæller

    //hente gemte værdier for statistik, status m.m.
    LoadDBValues();

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
    string err;
    db.Update_CurrentValue<string>("LastSetupRunTime", msg, err);
    //cout << "UpdateBlynkAfterFastTimer 1" << endl;
    spdlog::info("Program startet {}", msg);


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


int main(int argc, char* argv[])
{
    running = true;

    // Setup logger FIRST
    setupLogger();
    spdlog::info("Running... Press Ctrl+C to exit");

    // Catch signals (Ctrl+C, terminal close, etc.)
    signal(SIGINT, handleSignal);  // Handle Ctrl+C
    signal(SIGHUP, handleSignal);  // Handle terminal close
    signal(SIGTERM, handleSignal); // Handle kill command


    /* forbinder til pigpiod daemonen*/
    pi = pigpio_start(NULL, NULL);
    if (pi < 0)
    {
        spdlog::error("Failed to connect to pigpiod daemon");
        return EXIT_FAILURE;  // eller håndter fejlen på anden måde
    }
    spdlog::info("Connected to pigpiod");


    std::string sqlError;

    if (!db.IniSqlConnection(sqlError))
    {
        spdlog::error("Failed to connect to database: {}",sqlError);
        return EXIT_FAILURE;
    }

    spdlog::info("Connected to database successfully!");

    OrangeriSetup();
    ReadSetpoints();

    /*
    // Define the intervals in milliseconds
    const int intervalWriteStatusTimer = 540000; // 150 second
    const int intervalVerySlowTimer = 150000; // 150 second
    const int intervalSlowTimer = 30000; // 30  seconds
    const int intervalFastTimer = 15000; // 2 seconds

    // Initialize the last execution times
    auto lastExecutionWriteStatusTimer = std::chrono::steady_clock::now();
    auto lastExecutionVerySlowTimer = std::chrono::steady_clock::now();
    auto lastExecutionSlowTimer = std::chrono::steady_clock::now();
    auto lastExecutionFastTimer = std::chrono::steady_clock::now();
    */
    // Timer intervals
    const auto intervalWriteStatusTimer = std::chrono::milliseconds(540000);
    const auto intervalVerySlowTimer   = std::chrono::milliseconds(150000);
    const auto intervalSlowTimer       = std::chrono::milliseconds(30000);
    const auto intervalFastTimer       = std::chrono::milliseconds(15000);

    // Initialize timers
    auto lastExecutionWriteStatusTimer = std::chrono::steady_clock::now();
    auto lastExecutionVerySlowTimer    = std::chrono::steady_clock::now();
    auto lastExecutionSlowTimer        = std::chrono::steady_clock::now();
    auto lastExecutionFastTimer        = std::chrono::steady_clock::now();

    // Configure GPIOs
    int buttons[] = {GPIO_Button_WaterPump, GPIO_Button_StopVent, GPIO_Button_Disp};

    for(int i = 0; i < 3; i++)
    {
        set_mode(pi, buttons[i], PI_INPUT);
        set_pull_up_down(pi, buttons[i], PI_PUD_UP);
        // debounce
        set_glitch_filter(pi, buttons[i], 5000);
        try
        {
            callback(pi, buttons[i], EITHER_EDGE, buttonISR);
        }
        catch(...)
        {
            spdlog::error("Failed to register callback on GPIO {}", buttons[i]);
        }

        spdlog::info("GPIO configured");
        spdlog::info("Entering main loop");

        /*
        while(running.load())
        {
            auto now = std::chrono::steady_clock::now();

            // Check if it's time to execute function A
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastExecutionVerySlowTimer).count() >= intervalVerySlowTimer)
            {
                VerySlowTimerEvent();
                lastExecutionVerySlowTimer = now; // Update the last execution time
                //spdlog::info("VerySlowTimer Event");
            }

            // Check if it's time to execute function B
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastExecutionSlowTimer).count() >= intervalSlowTimer)
            {
                SlowTimerEvent();
                lastExecutionSlowTimer = now; // Update the last execution time
            }

            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastExecutionFastTimer).count() >= intervalFastTimer)
            {
                FastTimerEvent();
                lastExecutionFastTimer = now; // Update the last execution time
            }


            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastExecutionWriteStatusTimer).count() >= intervalWriteStatusTimer)
            {
                WriteStatusInfo();
                lastExecutionWriteStatusTimer = now; // Update the last execution time
            }

            // Sleep for a short duration to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        */
        while (running.load())
        {
            auto now = std::chrono::steady_clock::now();

            if (now - lastExecutionVerySlowTimer >= intervalVerySlowTimer)
            {
                VerySlowTimerEvent();
                lastExecutionVerySlowTimer = now;
            }

            if (now - lastExecutionSlowTimer >= intervalSlowTimer)
            {
                SlowTimerEvent();
                lastExecutionSlowTimer = now;
            }

            if (now - lastExecutionFastTimer >= intervalFastTimer)
            {
                FastTimerEvent();
                lastExecutionFastTimer = now;
            }

            if (now - lastExecutionWriteStatusTimer >= intervalWriteStatusTimer)
            {
                WriteStatusInfo();
                lastExecutionWriteStatusTimer = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        spdlog::info("Shutdown requested");

        //gpioTerminate(); // Always clean up
        spdlog::default_logger()->flush();  // Force immediate flush

        // Cleanup pigpio
        if (pi >=0)
        {
            pigpio_stop(pi);
        }
        spdlog::shutdown();

        return 0;
    }



}
