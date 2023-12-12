#include "daisysp.h"
#include "../kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <string.h>


#include "../../eurorack/rings/dsp/fx/reverb.h"
#include "../../eurorack/clouds/dsp/frame.h"
#include "../../eurorack/clouds/dsp/fx/reverb.h"

#include "hbpm.h"
#include "hdelay.h"

using namespace kxmx;
using namespace daisy;
using namespace daisysp;

#define CLOUDS_MODE 0
#define RINGS_MODE 1
#define SAMPLE_RATE 48000
#define MENU_ITEMS 6
#define DL_MULT_COUNT 13

CpuLoadMeter cpumeter;

Bluemchen bluemchen;

rings::Reverb rings_reverb;
clouds::Reverb clouds_reverb;

uint16_t rings_reverb_buffer[65536];
uint16_t clouds_reverb_buffer[65536];

clouds::FloatFrame clouds_frame;

const size_t max_delay_time =  SAMPLE_RATE*10;

hbpm bpm;
hdelay<float, max_delay_time> delay;

DelayLine<float, max_delay_time> DSY_SDRAM_BSS delaylineMemL[3];
DelayLine<float, max_delay_time> DSY_SDRAM_BSS delaylineMemR[3];


Parameter knob1;
Parameter knob2;
Parameter cv1;
Parameter cv2;

float _diffusion = 0.75f;
float _rvb_time;
float _rvb_lp_freq = 0.6;

int rvb_mode = CLOUDS_MODE;
std::string rvb_mode_names[2] = {"CLD", "RNG"};

bool incoming_gate = false;

const float dl_mult[DL_MULT_COUNT] =         {0.0625f, 0.125f, 0.25f, 0.50f, 0.75f, 1.f, 2.f, 3.f, 4.f, 8.f, 16.f, 32.f, 64.f};
std::string dl_mult_strings[DL_MULT_COUNT] = { "1/16",  "1/8", "1/4", "1/2", "3/4", "1", "2", "3", "4", "8", "16", "32", "64"};
int dl_mult_setting = 2;

int sel_m_it = 0; // selected menu item row
bool chg_val = false; // changing values? check if scrolling through menu or changing menu values
int menu_item = 0;

float dly_lvl = 0.7f;
float rvb_lvl = 0.7f;

void UpdateOled()
{
    bluemchen.display.Fill(false);

    // draw an arrow on selected menu item
    bluemchen.display.SetCursor(sel_m_it < 4 ? 0 : 32, sel_m_it < 4 ? sel_m_it*8 : (sel_m_it - 4) * 8);
    std::string str = ">";
    char *cstr = &str[0];
    bluemchen.display.WriteString(cstr, Font_6x8, true);

    menu_item = 0;
    bluemchen.display.SetCursor(sel_m_it == menu_item ? 7 : 0, 0);
    str = rvb_mode_names[rvb_mode];
    bluemchen.display.WriteString(cstr, Font_6x8, !(sel_m_it == menu_item && chg_val));

    menu_item = 1;
    bluemchen.display.SetCursor(sel_m_it == menu_item ? 7 : 0, 8);
    str = dl_mult_strings[dl_mult_setting];
    bluemchen.display.WriteString(cstr, Font_6x8, !(sel_m_it == menu_item && chg_val));

    menu_item = 2;
    bluemchen.display.SetCursor(sel_m_it == menu_item ? 7 : 0, 16);
    str = "r: " + std::to_string(static_cast<int>(rvb_lvl*10));
    bluemchen.display.WriteString(cstr, Font_6x8, !(sel_m_it == menu_item && chg_val));

    menu_item = 3;
    bluemchen.display.SetCursor(sel_m_it == menu_item ? 7 : 0, 24);
    str = "d: " + std::to_string(static_cast<int>(dly_lvl*10));
    bluemchen.display.WriteString(cstr, Font_6x8, !(sel_m_it == menu_item && chg_val));

    menu_item = 4;
    bluemchen.display.SetCursor(sel_m_it == menu_item ? 39 : 32, 0);
    str = "f: " + std::to_string(static_cast<int>(_rvb_lp_freq*10));
    bluemchen.display.WriteString(cstr, Font_6x8, !(sel_m_it == menu_item && chg_val));

    menu_item = 5;
    bluemchen.display.SetCursor(sel_m_it == menu_item ? 39 : 32, 8);
    str = delay.GetDelayTypeStr();
    bluemchen.display.WriteString(cstr, Font_6x8, !(sel_m_it == menu_item && chg_val));



    bluemchen.display.SetCursor(46, 16);
    str = std::to_string(static_cast<int>(cpumeter.GetAvgCpuLoad()*100));
    str = str + "%";
    bluemchen.display.WriteString(cstr, Font_6x8, true);

    bluemchen.display.SetCursor(46, 24);
    str = std::to_string(static_cast<int>(bpm.GetBpm()));
    bluemchen.display.WriteString(cstr, Font_6x8, true);



    bluemchen.display.Update();
}

void UpdateControls()
{
    bluemchen.ProcessAllControls();

    knob1.Process();
    knob2.Process();

    if (bluemchen.encoder.RisingEdge()) {
        chg_val = !chg_val;
    }

    int incr = bluemchen.encoder.Increment();
    float incr_dec = 0.1f * incr;

    if(chg_val) {

      switch(sel_m_it) {
        case 0: // reverb mode
          if(incr != 0)
            rvb_mode = ++rvb_mode > 1 ? 0 : 1;
        break;
        case 1: // delay mult/div
          if(dl_mult_setting + incr < DL_MULT_COUNT && 
             dl_mult_setting + incr >= 0){

            dl_mult_setting += incr;
          }
        break;
        case 2: // reverb level
          if (rvb_lvl + incr_dec < 1.0f && 
              rvb_lvl + incr_dec >= 0) 
            rvb_lvl += incr_dec;
        break;
        case 3: // delay level
          if (dly_lvl + incr_dec < 1.0f && 
              dly_lvl + incr_dec >= 0)
            dly_lvl += incr_dec;
        break;
        case 4: // reverb filter
          if (_rvb_lp_freq + incr_dec < 1.0f && 
              _rvb_lp_freq + incr_dec >= 0)
            _rvb_lp_freq += incr_dec;
        break;
        case 5: // delay type
          delay.NextDelayType(incr);
        break;
      }

    } else {

      if(sel_m_it + incr < MENU_ITEMS && 
         sel_m_it + incr >= 0) {

        sel_m_it += incr;

      } else if (sel_m_it+incr >= MENU_ITEMS) sel_m_it = 0; // round robin the menu items
        else if (sel_m_it+incr < 0) sel_m_it = MENU_ITEMS-1;



    }

    _rvb_time = knob1.Value();
    //_rvb_lp_freq = knob2.Value();

    delay.SetFeedback(knob2.Value());


    cv1.Process();
    cv2.Process();

    if (cv1.Value() > 500.0f && incoming_gate == false) {
        bpm.Process();

        if(bpm.BpmChanged())
          delay.SetDelay((SAMPLE_RATE*60*dl_mult[dl_mult_setting]) / bpm.GetBpm());

        incoming_gate = true;
    }

    if (cv1.Value() < 500.0f && incoming_gate == true) {

        incoming_gate = false;
    }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    cpumeter.OnBlockStart();

    UpdateControls();
    for (size_t i = 0; i < size; i++)
    {

        float sig_l = in[0][i];
        float sig_r = in[1][i];

        // left input signal will go to both reverbs
        float rings_sig_l = sig_l;
        float rings_sig_r = sig_l;

        clouds_frame.l = sig_l;
        clouds_frame.r = sig_l;


        //right input signal goes to delay

        float dly_sig_l = sig_r;
        float dly_sig_r = sig_r;


        rings_reverb.set_amount(1);
        rings_reverb.set_time(_rvb_time);// 0.5f + (0.49f * patch_position));
        rings_reverb.set_input_gain(0.8);
        rings_reverb.set_lp(_rvb_lp_freq);// : 0.6f);

        clouds_reverb.set_amount(1);
        clouds_reverb.set_time(_rvb_time);// 0.5f + (0.49f * patch_position));
        clouds_reverb.set_input_gain(0.8);
        clouds_reverb.set_lp(_rvb_lp_freq);// : 0.6f);

        rings_reverb.Process(&rings_sig_l, &rings_sig_r, 1);

        clouds_reverb.Process(&clouds_frame, 1);

        delay.Process(&dly_sig_l, &dly_sig_r);

        switch(rvb_mode) {
            case CLOUDS_MODE:
                out[0][i] = (clouds_frame.l*rvb_lvl) + (dly_sig_l*dly_lvl);
                out[1][i] = (clouds_frame.r*rvb_lvl) + (dly_sig_r*dly_lvl);
            break;
            case RINGS_MODE:
                out[0][i] = (rings_sig_l*rvb_lvl) + (dly_sig_l*dly_lvl);
                out[1][i] = (rings_sig_r*rvb_lvl) + (dly_sig_r*dly_lvl);
            break;

        }
    }

  cpumeter.OnBlockEnd();
}

int main(void)
{
    bluemchen.Init();
    bluemchen.StartAdc();

    knob1.Init(bluemchen.controls[bluemchen.CTRL_1], 0.00001f, 1.0f, Parameter::LINEAR);
    knob2.Init(bluemchen.controls[bluemchen.CTRL_2], 0.001f, 1.0f, Parameter::LINEAR);

    cv1.Init(bluemchen.controls[bluemchen.CTRL_3], -5000.0f, 5000.0f, Parameter::LINEAR);
    cv2.Init(bluemchen.controls[bluemchen.CTRL_4], -5000.0f, 5000.0f, Parameter::LINEAR);

    rings_reverb.Init(rings_reverb_buffer);
    rings_reverb.set_diffusion(_diffusion);

    clouds_reverb.Init(clouds_reverb_buffer);
    clouds_reverb.set_diffusion(_diffusion);

    bpm.Init();
    delay.Init(max_delay_time, delaylineMemL, delaylineMemR);

    cpumeter.Init(SAMPLE_RATE, 48);

    bluemchen.StartAudio(AudioCallback);

    while (1)
    {
        UpdateControls();
        UpdateOled();
    }
}
