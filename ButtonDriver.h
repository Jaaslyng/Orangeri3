#pragma once

#include <pigpiod_if2.h>
#include <queue>
#include <mutex>
#include <atomic>
#include <map>



constexpr int MAX_BUTTONS = 10;
constexpr int MAX_PULSES  = 10;

struct ButtonEvent
{
    int gpio;
    int level;
};

class ButtonDriver
{
public:
    ButtonDriver(int piHandle);

    void addButton(int gpio);
    void addPulseCounter(int gpio);

    void init();

    bool getEvent(ButtonEvent &ev);

    uint32_t getAndResetPulseCount(int gpio);

private:
   // static std::atomic<uint32_t> pulseCounters[MAX_PULSES];
    static void isr(int pi, unsigned gpio, unsigned level, uint32_t tick);

    static int pi;

    static std::queue<ButtonEvent> eventQueue;
    static std::mutex queueMutex;

    //static std::map<int,std::atomic<int>> pulseCounters;
    static std::atomic<uint32_t> pulseCounters[MAX_PULSES];
    static int pulseIndexFromGPIO(int gpio);
    int buttonPins[10];
    int buttonCount = 0;

    int pulsePins[10];
    int pulseCount = 0;
};
