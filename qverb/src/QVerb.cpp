#include "daisysp.h"
#include "../../kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <string.h>

using namespace kxmx;
using namespace daisy;
using namespace daisysp;

Bluemchen bluemchen;

static ReverbSc   verb;
static DcBlock    blk[2];
Parameter         lpParam;
static float      drylevel, send;

Parameter knob1;
Parameter knob2;

/* Store for CV and Knob values*/
int cv_values[2] = {0,0};

float wetValue = 0.94f;
int wetCalc[2] {0,0};

float filterValue = 10000.0f;
int filterCalc[2] {0,0};

float feedbackValue = 0.5f;
int feedbackCalc[2] {0,0};

/* value for the menu that is showing on screen */
int currentMenu = 0;

/* variable for CV setting menu */
int cv_link[3] = {0,1, 2};
std::string cv_link_string[3] {"wet", "filter", "feedback" };
std::string cv_link_item_string[2] {"Pot 1", "Pot 2"};

/* Value of the encoder */
int enc_val = 0;

void Menu_Main() {

    bluemchen.display.SetCursor(0, 0);
    char *cstr = &str[0];
    str = "damp: ";
    bluemchen.display.WriteString(cstr, Font_6x8, true);

    str = std::to_string(static_cast<int>(filterValue));
    bluemchen.display.SetCursor(42, 8);
    bluemchen.display.WriteString(cstr, Font_6x8, true);

    str = "feedb:";
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(cstr, Font_6x8, true);

    str = std::to_string(static_cast<int>(feedbackValue * 100));
    bluemchen.display.SetCursor(42, 16);
    bluemchen.display.WriteString(cstr, Font_6x8, true);
}

void Menu_SetCV() {
    int i = currentMenu -1;
    bluemchen.display.SetCursor(0, 0);
    std::string str = cv_link_item_string[i];
    char *cstr = &str[0];
    bluemchen.display.WriteString(cstr, Font_6x8, true);

    bluemchen.display.SetCursor(0, 8);
    str = cv_link_string[cv_link[i]];
    bluemchen.display.WriteString(cstr, Font_6x8, true);
}

void UpdateOled()
{
    bluemchen.display.Fill(false);

    if (currentMenu == 0) {
        Menu_Main();
    } else {
        Menu_SetCV();
    }    

    bluemchen.display.Update();
}

void setCurrentMenu (){
    // Set current menu
    if(bluemchen.encoder.RisingEdge()) {
        currentMenu++;
    }
    if(currentMenu >= 3) {
        currentMenu = 0;
    }
}

void setCVlinkmenu(){    
    int i = currentMenu -1;
    cv_link[i] += bluemchen.encoder.Increment();
    if(cv_link[i] >= 2 || cv_link[i] <= 0) {
        cv_link[i] = 0;
    }    
}

void calculateValues() {
    // reset values
    for(int j = 0; j < 2; j++) {
        dryCalc[j] = 0;
        wetCalc[j] = 0;
        filterCalc[j] = 0;
        feedbackCalc[j] = 0;
    }    

    for(int i = 0; i < 4; i++) {
        switch (cv_link[i])
        {
        case 1:
            /* Filter */
            filterCalc[0] += cv_values[i];
            filterCalc[1]++;
            break;

        case 2:
            /* wet */
            feedbackCalc[0] += cv_values[i];
            feedbackCalc[1]++;
            break;
        
        default:
            break;
        }        
    }

    wetValue = 1.0f;
    filterValue = 10000.0f + ( (filterCalc[0] / filterCalc[1]) * 2.0f );
    feedbackValue = 0.5f + ( (feedbackCalc[0] / feedbackCalc[1]) / 1000.0f);
}

void processCVandKnobs(){
    
    cv_values[0] = knob1.Process();
    cv_values[1] = knob2.Process();  

    calculateValues();
}

void UpdateControls()
{
    bluemchen.ProcessAllControls();

    processCVandKnobs();

    setCurrentMenu();
    if (currentMenu !=0) { setCVlinkmenu(); }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    float dryL, dryR, wetL, wetR, sendL, sendR;
   
    bluemchen.ProcessAnalogControls();
    dryValue =0;
    for(size_t i = 0; i < size; i++)
    {
        // read some controls
        drylevel = 0.0f;
        send     = wetValue;
        verb.SetFeedback(feedbackValue);
        verb.SetLpFreq(filterValue);

        // Read Inputs (only stereo in are used)
        dryL = in[0][i];
        dryR = in[1][i];

        // Send Signal to Reverb
        sendL = dryL * send;
        sendR = dryR * send;
        verb.Process(sendL, sendR, &wetL, &wetR);

        // Dc Block
        wetL = blk[0].Process(wetL);
        wetR = blk[1].Process(wetR);

        // Out 1 and 2 are Mixed
        out[0][i] = wetL;
        out[1][i] = wetR;
    }
    UpdateControls();
}

int main(void)
{
    float samplerate;
    bluemchen.Init();
    samplerate = bluemchen.AudioSampleRate();

    verb.Init(samplerate);
    verb.SetFeedback(feedbackValue);
    verb.SetLpFreq(filterValue);

    knob1.Init(bluemchen.controls[bluemchen.CTRL_1], 0.0f, 5000.0f, Parameter::LINEAR);
    knob2.Init(bluemchen.controls[bluemchen.CTRL_2], 0.0f, 5000.0f, Parameter::LINEAR);

    blk[0].Init(samplerate);
    blk[1].Init(samplerate);

    lpParam.Init(bluemchen.controls[3], 20, 20000, Parameter::LOGARITHMIC);

    bluemchen.StartAdc();
    bluemchen.StartAudio(AudioCallback);

    while (1)
    {
        UpdateControls();
        UpdateOled();
    }
}
