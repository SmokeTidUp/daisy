#include "daisysp.h"
#include "../kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <string.h>
#include <cmath>

using namespace kxmx;
using namespace daisy;
using namespace daisysp;

Bluemchen bluemchen;


Parameter knob1;
Parameter knob2;
Parameter cv1;
Parameter cv2;

void UpdateOled()
{
    bluemchen.display.Fill(false);

    // Display Encoder test increment value and pressed state
    bluemchen.display.SetCursor(0, 0);
    std::string str = "knob1:";
    char *cstr = &str[0];
    bluemchen.display.WriteString(cstr, Font_6x8, true);

    str = std::to_string(static_cast<int>(knob1.Value()));
    bluemchen.display.SetCursor(33, 0);
    bluemchen.display.WriteString(cstr, Font_6x8, true);
    
    bluemchen.display.Update();
}

void UpdateControls()
{
    bluemchen.ProcessAllControls();

    knob1.Process();
    knob2.Process();

// put the change of the parameters here?
    cv1.Process();
    cv2.Process();

}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    UpdateControls();
    for (size_t i = 0; i < size; i++)
    {

        float sig_l = in[0][i];
        float sig_r = in[1][i];
        float k_val = knob1.Value();

        if(k_val < sig_l)
          out[0][i] = sig_l;
        else if(k_val > sig_l)
          out[0][i] = sig_l + (k_val - sig_l) / pow(1 + ((k_val - sig_l) / (1 - sig_l)), 2);
        else if(k_val > 1)
          out[0][i] = (sig_l + 1) / 2;

        //out[0][i] = sig_l;
        //out[1][i] = sig_r;
    }
}

int main(void)
{
    bluemchen.Init();
    bluemchen.StartAdc();

    knob1.Init(bluemchen.controls[bluemchen.CTRL_1], 0.001f, 0.1f, Parameter::LINEAR);
    knob2.Init(bluemchen.controls[bluemchen.CTRL_2], 0.001f, 0.5f, Parameter::LINEAR);

    cv1.Init(bluemchen.controls[bluemchen.CTRL_3], -5000.0f, 5000.0f, Parameter::LINEAR);
    cv2.Init(bluemchen.controls[bluemchen.CTRL_4], -5000.0f, 5000.0f, Parameter::LINEAR);

    bluemchen.StartAudio(AudioCallback);

    while (1)
    {
        UpdateControls();
        UpdateOled();
    }
}
