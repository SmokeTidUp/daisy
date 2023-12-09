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

Bluemchen bluemchen;

rings::Reverb rings_reverb;
clouds::Reverb clouds_reverb;

uint16_t rings_reverb_buffer[65536];
uint16_t clouds_reverb_buffer[65536];

clouds::FloatFrame clouds_frame;

hbpm bpm;
hdelay delay;

Parameter knob1;
Parameter knob2;
Parameter cv1;
Parameter cv2;

float _diffusion = 0.75f;
float _rvb_time;
float _rvb_lp_freq;

int rvb_mode = CLOUDS_MODE;
std::string rvb_mode_names[2] = {"CLOUDS", "RINGS"};

bool incoming_gate = false;

void UpdateOled()
{
    bluemchen.display.Fill(false);

    bluemchen.display.SetCursor(0, 0);
    std::string str = rvb_mode_names[rvb_mode];
    char *cstr = &str[0];
    bluemchen.display.WriteString(cstr, Font_6x8, true);


    bluemchen.display.SetCursor(46, 24);
    //str = std::to_string(static_cast<int>(bpm.getBpm()));
    bluemchen.display.WriteString(cstr, Font_6x8, true);


    bluemchen.display.Update();
}

void UpdateControls()
{
    bluemchen.ProcessAllControls();

    knob1.Process();
    knob2.Process();

    if(bluemchen.encoder.RisingEdge()) {
        rvb_mode = ++rvb_mode > 1 ? 0 : 1;
    }

    _rvb_lp_freq = knob1.Value();
    _rvb_time = knob2.Value();

    cv1.Process();
    cv2.Process();

    if (cv1.Value() > 500.0f && incoming_gate == false) {
        //bpm.Process();
        incoming_gate = true;
    }

    if (cv1.Value() < 500.0f && incoming_gate == true) {

        incoming_gate = false;
    }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    UpdateControls();
    for (size_t i = 0; i < size; i++)
    {

        float rings_sig_l = in[0][i];
        float rings_sig_r = in[1][i];

        clouds_frame.l = rings_sig_l;
        clouds_frame.r = rings_sig_r;

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

        switch(rvb_mode) {
            case CLOUDS_MODE:
                out[0][i] = clouds_frame.l;
                out[1][i] = clouds_frame.r;
            break;
            case RINGS_MODE:
                out[0][i] = rings_sig_l;
                out[1][i] = rings_sig_r;
            break;

        }
    }
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
    delay.Init(SAMPLE_RATE*4);

    bluemchen.StartAudio(AudioCallback);

    while (1)
    {
        UpdateControls();
        UpdateOled();
    }
}
