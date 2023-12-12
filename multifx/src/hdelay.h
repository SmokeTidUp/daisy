using namespace daisysp;
using namespace daisy;

#define D_STR 0
#define D_PONG 1
#define DELAY_TYPE_COUNT 2

template<typename T, int max_size>
class hdelay
{
public:
	hdelay() {};
	~hdelay() {};

	void Init(float _delay_lenght, DelayLine<T, max_size> * _dl_l, DelayLine<T, max_size> * _dl_r) {
    for (int i = 0; i < 3; ++i) {
      _dl_l[i].Init();
      delayLineL[i] = &_dl_l[i];
    }

    for (int i = 0; i < 3; ++i) {
      _dl_r[i].Init();
      delayLineR[i] = &_dl_r[i];
    }

		for (int i = 0; i < 3; i++) {
	    delayLineL[i]->SetDelay(_delay_lenght);
		}

	  for (int i = 0; i < 3; i++) {
	    delayLineR[i]->SetDelay(_delay_lenght);
	  }
	};

	void Process(float * _sig_l, float * _sig_r) {


    float dl_l = delayLineL[0]->Read();
    float dl_r = delayLineR[0]->ReadWithPhase(delay_type == D_PONG ? delay_samples: 0);

    switch (delay_type) {
      case D_STR:

        for (int i = 0; i < 3; i++) {
          delayLineL[i]->SetDelay(delay_samples);
        }

        for (int i = 0; i < 3; i++) {
          delayLineR[i]->SetDelay(delay_samples);
        }

        delayLineL[0]->Write( (*_sig_l) + dl_l * feedback);
        delayLineR[0]->Write( (*_sig_r) + dl_r * feedback);

      break;
      case D_PONG:

        for (int i = 0; i < 3; i++) {
          delayLineL[i]->SetDelay(delay_samples*2);
        }

        for (int i = 0; i < 3; i++) {
          delayLineR[i]->SetDelay(delay_samples*2);
        }

        delayLineL[0]->Write( (*_sig_l) + dl_r * feedback);
        delayLineR[0]->Write( (*_sig_l) + dl_l * feedback);

      break;
    }

    *_sig_l = dl_l;
    *_sig_r = dl_r;

  };

  void SetDelay(float _samples) {

    delay_samples = _samples;

  };

  void SetFeedback(float _f) {
    feedback = _f;
  }

  void NextDelayType(signed int incr) {
    if(delay_type + incr < DELAY_TYPE_COUNT && 
       delay_type + incr >= 0){
      delay_type += incr;
    } else if (delay_type+incr >= DELAY_TYPE_COUNT) delay_type = 0; // round robin the menu items
      else if (delay_type+incr < 0) delay_type = DELAY_TYPE_COUNT-1;
  }

  void SetDelayType(int _delay_type){
    delay_type = _delay_type;
  }

  std::string GetDelayTypeStr () {
    switch (delay_type) {
      case D_STR:
        return "STR";
        break;
      case D_PONG:
        return "PONG";
      break;
    }
  }

private:
	static const int DL_MULT_COUNT = 13;

	DelayLine<T, max_size> *delayLineL[3];

  DelayLine<T, max_size>  *delayLineR[3];

  float feedback = .75f;

  int delay_type = D_STR;
  float delay_samples;

};