/*
 * 
███████╗ ██████╗     ██╗                 ██████╗ 
██╔════╝██╔═══ ██╗   ██║                ██╔════╝ 
███████╗██║    ██║   ██║        █████╗  ███████╗ 
╚════██║██║    ██║   ██║        ╚════╝  ██╔══██╗
███████║╚██████╔╝    ███████╗           ╚██████╔
╚══════╝ ╚═════╝     ╚══════╝            ╚═════╝

                                      POLYPHONIC SYNTHESIZER

                       
                            A BlogHoskins Monstrosity @ 2020
                           https://bloghoskins.blogspot.com/
                                                 ANSI Shadow

v0.2 ADSR now working (Maybe change durations).
v0.1 Mono synth. Wav switch works.
*/

#include <MIDI.h>
#include <MozziGuts.h>
#include <Oscil.h>                            // oscillator template
#include <tables/saw2048_int8.h>              // saw table for oscillator
#include <tables/square_no_alias_2048_int8.h> // square table for oscillator
#include <mozzi_midi.h>
#include <ADSR.h>
#include <AutoMap.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define WAVE_SWITCH 2 // switch for switching waveforms

// Set up Attack Envelope
#define atkPot A2                     // select the input pin for the potentiometer
AutoMap atkPotMap(0, 1023, 20, 3000); // remap the atk pot to 3 seconds

// Set up Decay Envelope
#define dcyPot A3                     // select the input pin for the potentiometer
AutoMap dcyPotMap(0, 1023, 20, 5000); // remap the dky pot to 5 seconds

// Set up Sustain Envelope
#define susPot A5                     // select the input pin for the potentiometer
AutoMap susPotMap(0, 1023, 20, 5000); // remap the sus pot to 5 seconds

// Set up Release Envelope
#define relPot A4                     // select the input pin for the potentiometer
AutoMap relPotMap(0, 1023, 20, 3000); // remap the rel pot to 3 seconds

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 128 // powers of 2 please

// audio sinewave oscillator
Oscil<SAW2048_NUM_CELLS, AUDIO_RATE> oscil1;              //Saw Wav
Oscil<SQUARE_NO_ALIAS_2048_NUM_CELLS, AUDIO_RATE> oscil2; //Sqr wave

// envelope generator
ADSR<CONTROL_RATE, AUDIO_RATE> envelope;

#define LED 13 // Internal LED lights up if MIDI is being received

void HandleNoteOn(byte channel, byte note, byte velocity)
{
    if (velocity == 0)
    {
        HandleNoteOff(channel, note, velocity);
        return;
    }
    oscil1.setFreq(mtof(float(note)));
    envelope.noteOn();
    digitalWrite(LED, HIGH);
}

void HandleNoteOff(byte channel, byte note, byte velocity)
{
    envelope.noteOff();
    digitalWrite(LED, LOW);
}

void setup()
{
    pinMode(LED, OUTPUT);                 // built-in arduino led lights up with midi sent data
    MIDI.setHandleNoteOn(HandleNoteOn);   // Put only the name of the function
    MIDI.setHandleNoteOff(HandleNoteOff); // Put only the name of the function
    MIDI.begin(MIDI_CHANNEL_OMNI);
    envelope.setADLevels(255, 64); // A bit of attack / decay while testing
    startMozzi(CONTROL_RATE);
}

void updateControl()
{
    MIDI.read();

    int atkVal = mozziAnalogRead(atkPot); // read the value from the Attack pot
    atkVal = atkPotMap(atkVal);

    int dcyVal = mozziAnalogRead(dcyPot); // read the value from the Decay pot
    dcyVal = dcyPotMap(dcyVal);

    int susVal = mozziAnalogRead(susPot); // read the value from the Sustain pot
    susVal = susPotMap(susVal);

    int relVal = mozziAnalogRead(relPot); // read the value from the Release pot
    relVal = relPotMap(relVal);

    envelope.setTimes(atkVal, dcyVal, susVal, relVal); // 10000 is so the note will sustain 10 seconds unless a noteOff comes
    envelope.update();

    pinMode(2, INPUT_PULLUP);       // Pin two is connected to the middle of a switch, high switch goes to 5v, low to ground
    int sensorVal = digitalRead(2); // read the switch position value into a  variable
    if (sensorVal == HIGH)          // If switch is set to high, run this portion of code
    {
        oscil1.setTable(SAW2048_DATA);
    }
    else // If switch not set to high, run this portion of code instead
    {
        oscil1.setTable(SQUARE_NO_ALIAS_2048_DATA);
    }
}

int updateAudio()
{
    return (int)(envelope.next() * oscil1.next()) >> 8;
}

void loop()
{
    audioHook(); // required here
}