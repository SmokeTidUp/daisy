#include "src/per/tim.h"

class hbpm
{
public:
	hbpm() {};
	~hbpm() {};

	void Process() { calc_bpm(); };
	void Init() {
		daisy::TimerHandle::Config config = daisy::TimerHandle::Config();
		config.periph = daisy::TimerHandle::Config::Peripheral::TIM_2;
		config.dir = daisy::TimerHandle::Config::CounterDir::UP;
		//config.enable_irq = 0;

		timekeeper.Init(config); 
		timekeeper.Start();
	}


	float getBpm() { return _bpm; }

	
private:
	float _bpm = 0.0f, old_bpm = 0.0f;
	bool bpm_changed = true;

	static const int BPM_SMOOTHER_COUNT = 3;
	float old_bpms[BPM_SMOOTHER_COUNT];
	int bpm_smoother_counter = 0;

	uint32_t old_time = 0;
	uint32_t new_time = 0;
	float time_interval = 0;

	daisy::TimerHandle timekeeper;

	void calc_bpm() {

	  bpm_changed = false;

	  float average = 0;
	  
	  new_time = timekeeper.GetTick();
	  time_interval = (float)(new_time - old_time)/480000; // 4800000 is the CPU clock
	  old_time = new_time;

	  old_bpms[bpm_smoother_counter++] = 60000 / (time_interval * 8);
	  if(bpm_smoother_counter == BPM_SMOOTHER_COUNT) bpm_smoother_counter = 0;

	  for (int i = 0; i < BPM_SMOOTHER_COUNT; i++)
	  {
	    average += old_bpms[i];
	  }

	  _bpm = round(average / BPM_SMOOTHER_COUNT);

	  if (_bpm != old_bpm)
	  {
	    old_bpm = _bpm;
	    bpm_changed = true;
	  }


	}


};




