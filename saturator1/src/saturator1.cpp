#include "daisysp.h"
#include "../kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <string.h>
#include <cmath>

#define MODES 2

using namespace kxmx;
using namespace daisy;
using namespace daisysp;

Bluemchen bluemchen;
Overdrive drive;
Limiter lim;


Parameter knob1;
Parameter knob2;
Parameter cv1;
Parameter cv2;

int current_mode = 0;
std::vector<std::string> mode_names{"SATURATE", "OVERDRIVE"};

float sat_value = 0, drive_value = 0;

float saturator1(float in) {
  float _sat_value = 1-sat_value;
  if(_sat_value < in)
    return in;
  else if(_sat_value > in)
    return in + (_sat_value - in) / pow(1 + ((_sat_value - in) / (1 - in)), 2);
  else if(_sat_value > 1)
    return (in + 1) / 2;
}

void UpdateOled()
{
  bluemchen.display.Fill(false);

  bluemchen.display.SetCursor(0, 0);
  std::string str = mode_names[current_mode];
  char *cstr = &str[0];
  bluemchen.display.WriteString(cstr, Font_6x8, true);

  // show knob value
  bluemchen.display.SetCursor(0, 8);
  str = "knob1:";
  bluemchen.display.WriteString(cstr, Font_6x8, true);

  str = std::to_string(static_cast<int>(sat_value*1000));
  bluemchen.display.SetCursor(33, 8);
  bluemchen.display.WriteString(cstr, Font_6x8, true);

  bluemchen.display.SetCursor(0, 16);
  str = "knob2:";
  bluemchen.display.WriteString(cstr, Font_6x8, true);

  str = std::to_string(static_cast<int>(drive_value*1000));
  bluemchen.display.SetCursor(33, 16);
  bluemchen.display.WriteString(cstr, Font_6x8, true);
  
  bluemchen.display.Update();
}

void UpdateControls()
{
  bluemchen.ProcessAllControls();

  knob1.Process();
  knob2.Process();
  if(abs(knob1.Value() - sat_value) >= 0.003f)
    sat_value = knob1.Value();
  if(abs(knob2.Value() - drive_value) >= 0.003f){
    drive_value = knob2.Value();
    drive.SetDrive(drive_value);
  }

  if (bluemchen.encoder.RisingEdge())
  {
    current_mode == MODES - 1 ? current_mode = 0 : current_mode++;
  }

  cv1.Process();
  cv2.Process();

}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
  UpdateControls();

  for (size_t i = 0; i < size; i++)
  {

    float sig_l = in[0][i];
    float saturated_output;

    switch (current_mode) {
      case 0:
        saturated_output = saturator1(sig_l) * 3;
      break;
      case 1:
        saturated_output = drive.Process(sig_l) * (drive_value * 0.5);
      break;
    }

    out[0][i] = saturated_output;
    //out[1][i] = sig_r;
  }
}

int main(void)
{
  bluemchen.Init();
  bluemchen.StartAdc();

  knob1.Init(bluemchen.controls[bluemchen.CTRL_1], 0.001f, 1.0f, Parameter::LINEAR);
  knob2.Init(bluemchen.controls[bluemchen.CTRL_2], 0.001f, 1.0f, Parameter::LINEAR);

  cv1.Init(bluemchen.controls[bluemchen.CTRL_3], -5000.0f, 5000.0f, Parameter::LINEAR);
  cv2.Init(bluemchen.controls[bluemchen.CTRL_4], -5000.0f, 5000.0f, Parameter::LINEAR);

  bluemchen.StartAudio(AudioCallback);

  while (1)
  {
    UpdateControls();
    UpdateOled();
  }
}
