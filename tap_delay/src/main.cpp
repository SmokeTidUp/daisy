#include "daisysp.h"
#include "../kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <string.h>
#include "src/per/tim.h"
//#include <cmath>

#define BPM_SMOOTHER_COUNT 3
#define DL_MULT_COUNT 16
#define SAMPLERATE 48000

using namespace kxmx;
using namespace daisy;
using namespace daisysp;

Bluemchen bluemchen;

// TODO rewrite into structure, make 3 of them and let them interpolate on change of multiplier
DelayLine<float, SAMPLERATE*4> DSY_SDRAM_BSS delaylineMem[3];
DelayLine<float, SAMPLERATE*4> *delayline[3];

ReverbSc reverb;

Parameter knob1;
Parameter knob2;
Parameter cv1;
Parameter cv2;

float knob1_smooth;
float knob2_smooth;

float bpm = 0.0f;
float old_bpm = bpm;
bool bpm_changed = false;
float old_bpms[BPM_SMOOTHER_COUNT];
int bpm_smoother_counter = 0;
bool incoming_gate = false;
uint32_t old_time = 0;
uint32_t new_time = 0;
float time_interval = 0;
TimerHandle timekeeper;

bool first_gate = false;
int gate_counter = 0;

float samplerate;

const float dl_mult[DL_MULT_COUNT] =         { 0.125f,     0.25f,   0.50f,   0.75f,    1.f,    2.f,    4.f,   8.f,  16.f,  64.f,128.f, 256.f, 512.f, 1024.f, 2048.f, 4096.f};
std::string dl_mult_strings[DL_MULT_COUNT] = {"1/2048", "1/1024", "1/256", "1/128", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2",  "1",   "2",   "4",    "8",   "16",   "32"};
int dl_mult_setting = 8;
float dl_delay;
bool change_dl_time = false;


void UpdateOled()
{
  bluemchen.display.Fill(false);

  bluemchen.display.SetCursor(0, 0);
  std::string str = std::to_string(static_cast<int>(bpm));
  char *cstr = &str[0];
  bluemchen.display.WriteString(cstr, Font_6x8, bpm_changed);

  bluemchen.display.SetCursor(0, 8);
  str = dl_mult_strings[dl_mult_setting];
  bluemchen.display.WriteString(cstr, Font_6x8, true);

  bluemchen.display.SetCursor(0, 16);
  str = std::to_string(static_cast<int>(dl_delay));
  bluemchen.display.WriteString(cstr, Font_6x8, true);

  bluemchen.display.SetCursor(0, 24);
  str = std::to_string(static_cast<int>(samplerate));
  bluemchen.display.WriteString(cstr, Font_6x8, true);

  bluemchen.display.Update();
}

void bpm_smoother() {

  bpm_changed = false;

  float average = 0;

  old_bpms[bpm_smoother_counter++] = 60000 / (time_interval * 8);
  if(bpm_smoother_counter == BPM_SMOOTHER_COUNT) bpm_smoother_counter = 0;

  for (int i = 0; i < BPM_SMOOTHER_COUNT; i++)
  {
    average += old_bpms[i];
  }

  bpm = round(average / BPM_SMOOTHER_COUNT);

  if (bpm != old_bpm)
  {
    old_bpm = bpm;
    bpm_changed = true;
  }

}

void UpdateControls()
{
  bluemchen.ProcessAllControls();

  knob1.Process();
  knob2.Process();

  cv1.Process();
  cv2.Process();

  if (cv1.Value() > 500.0f && incoming_gate == false) {

    incoming_gate = true;

    gate_counter++;
    if(gate_counter > 4) gate_counter = 1;

    //attempt at phase alignment
    // should be called second gate but who cares
    if(!first_gate) {
      first_gate = true;

      if(gate_counter == 1)
        for (int i = 0; i < 3; ++i){
          delaylineMem[i].Reset();
        }
    }

    new_time = timekeeper.GetTick();
    time_interval = (float)(new_time - old_time)/480000;
    old_time = new_time;

    bpm_smoother();


    if(bpm_changed || change_dl_time){

      dl_delay = ((samplerate*60*dl_mult[dl_mult_setting]) / bpm);

      // for (int i = 0; i < 3; i++)
      // {
      //   delayline[i]->SetDelay(dl_delay);
      // }
      delayline[0]->SetDelay(dl_delay);
      delayline[1]->SetDelay(dl_delay*2);
      delayline[2]->SetDelay(dl_delay*3);

      change_dl_time = false;
    }
  } 

  if (cv1.Value() < 500.0f && incoming_gate == true) {

    incoming_gate = false;
  }

  if(dl_mult_setting + bluemchen.encoder.Increment() < DL_MULT_COUNT
     && dl_mult_setting + bluemchen.encoder.Increment() >= 0){

    dl_mult_setting += bluemchen.encoder.Increment();
    change_dl_time = true;
  }

  float knob1_tmp = knob1.Value();
  if(abs(knob1_smooth - knob1_tmp) > 0.01f){
    knob1_smooth = knob1_tmp;
    reverb.SetFeedback(knob1_smooth);
  }
  float knob2_tmp = knob2.Value();
  if(abs(knob2_smooth - knob2_tmp) > 0.01f){
    knob2_smooth = knob2_tmp;
    reverb.SetLpFreq(knob2_smooth*10000); // max freq 10khz
  }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
  UpdateControls();

  for (size_t i = 0; i < size; i++)
  {
    float rev_signal;

    
    delayline[0]->Write(in[0][i]);
    delayline[1]->Write(in[0][i]);
    delayline[2]->Write(in[0][i]);

    // for (int j = 1; j < 3; j++)
    // {
    //   delayline[j]->Write(delayline[j-1]->Read());
    // }


    out[0][i] = (delayline[0]->Read()) * (1-delayline[1]->Read()) +
                (delayline[1]->Read()) * (1-delayline[2]->Read()) +
                (delayline[2]->Read()) * (1-in[0][i]);

    //reverb.Process(in[0][i]*0.5f, 0, &rev_signal, 0);


    out[1][i] = rev_signal;


    //out[1][i] = sig_r;
  }

}

int main(void)
{
  bluemchen.Init();
  bluemchen.StartAdc();
  
  TimerHandle::Config config = TimerHandle::Config();
  config.periph = TimerHandle::Config::Peripheral::TIM_2;
  config.dir = TimerHandle::Config::CounterDir::UP;
  //config.enable_irq = 0;

  timekeeper.Init(config); 
  timekeeper.Start();

  samplerate = bluemchen.AudioSampleRate();

  knob1.Init(bluemchen.controls[bluemchen.CTRL_1], 0.001f, 1.0f, Parameter::LINEAR);
  knob2.Init(bluemchen.controls[bluemchen.CTRL_2], 0.001f, 1.0f, Parameter::LINEAR);

  cv1.Init(bluemchen.controls[bluemchen.CTRL_3], -5000.0f, 5000.0f, Parameter::LINEAR);
  cv2.Init(bluemchen.controls[bluemchen.CTRL_4], -5000.0f, 5000.0f, Parameter::LINEAR);

  for (int i = 0; i < 3; ++i)
  {
    delaylineMem[i].Init();
    delayline[i] = &delaylineMem[i];
  }

  for (int i = 0; i < BPM_SMOOTHER_COUNT; i++)
  {
    old_bpms[i] = 0.f;
  }

  reverb.Init(48000);
  reverb.SetFeedback(0.85f);
  reverb.SetLpFreq(8000.f);

  bluemchen.StartAudio(AudioCallback);

  while (1)
  {
    UpdateControls();
    UpdateOled();
  }
}
