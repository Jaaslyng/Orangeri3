#include "ButtonDriver.h"
#include <stdio.h>
#include <spdlog/spdlog.h>

int ButtonDriver::pi = -1;


std::queue<ButtonEvent> ButtonDriver::eventQueue;
std::mutex ButtonDriver::queueMutex;
std::atomic<uint32_t> ButtonDriver::pulseCounters[MAX_PULSES];
static std::map<int,std::atomic<int>> pulseCounters;
static int pulsePinsGlobal[MAX_PULSES];
static int pulsePinCountGlobal = 0;

ButtonDriver::ButtonDriver(int piHandle)
{
    pi = piHandle;
}

void ButtonDriver::addButton(int gpio)
{
    if(buttonCount < MAX_BUTTONS)
        buttonPins[buttonCount++] = gpio;
}

void ButtonDriver::addPulseCounter(int gpio)
{
    if(pulseCount < MAX_PULSES)
    {
        pulsePins[pulseCount] = gpio;
        pulsePinsGlobal[pulseCount] = gpio;
        pulseCounters[pulseCount] = 0;
        pulseCount++;
        pulsePinCountGlobal++;
    }
}

int ButtonDriver::pulseIndexFromGPIO(int gpio)
{
    for(int i=0;i<pulsePinCountGlobal;i++)
    {
        if(pulsePinsGlobal[i] == gpio)
            return i;
    }
    return -1;
}

void ButtonDriver::init()
{
    // init buttons
    for(int i=0;i<buttonCount;i++)
    {
        int gpio = buttonPins[i];

        spdlog::info("Init button GPIO {}", gpio);

        set_mode(pi, gpio, PI_INPUT);
        set_pull_up_down(pi, gpio, PI_PUD_UP);
        set_glitch_filter(pi, gpio, 10000); //Bedst til knapper

        try
        {
            int id = callback(pi, gpio, EITHER_EDGE, ButtonDriver::isr);
            spdlog::info("callback id = {}", id);
        }
        catch(...)
        {
            spdlog::error("Failed to register callback on GPIO {}", gpio);
        }


    }

    // init pulse inputs
    for(int i=0;i<pulseCount;i++)
    {
        int gpio = pulsePins[i];

        spdlog::info("Init pulse GPIO {}", gpio);

        set_mode(pi, gpio, PI_INPUT);
        //set_pull_up_down(pi, gpio, PI_PUD_UP);
        set_pull_up_down(pi, gpio, PI_PUD_OFF);  //med modstand i kredsløb
        //set_glitch_filter(pi, gpio, 2000);   //Bedst til kanpper
        set_noise_filter(pi, gpio, 500, 200000);  //Best til puls måling

        try
        {
            int id = callback(pi, gpio, EITHER_EDGE, ButtonDriver::isr);
            spdlog::info("pulse callback id = {}", id);
        }
        catch(...)
        {
            spdlog::error("Failed to register callback on GPIO {}", gpio);
        }

    }
}

void ButtonDriver::isr(int pi, unsigned gpio, unsigned level, uint32_t tick)
{
    int index = pulseIndexFromGPIO(gpio);

    // pulse input
    if(index >= 0)
    {
        if(level == 0)
            pulseCounters[index]++;

        return;
    }

    // button event
    ButtonEvent ev;
    ev.gpio = gpio;
    ev.level = level;

    std::lock_guard<std::mutex> lock(queueMutex);
    eventQueue.push(ev);
}

bool ButtonDriver::getEvent(ButtonEvent &ev)
{
    std::lock_guard<std::mutex> lock(queueMutex);

    if(eventQueue.empty())
        return false;

    ev = eventQueue.front();
    eventQueue.pop();

    return true;
}

uint32_t ButtonDriver::getAndResetPulseCount(int gpio)
{
    int index = pulseIndexFromGPIO(gpio);

    if(index < 0)
        return 0;

    uint32_t count = pulseCounters[index];
    pulseCounters[index] = 0;

    return count;
}
