using namespace daisysp;
using namespace daisy;

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
    float dl_r = delayLineR[0]->Read();

    delayLineL[0]->Write( (*_sig_l) + dl_l * feedback);
    delayLineR[0]->Write( (*_sig_r) + dl_r * feedback);

    *_sig_l = dl_l;
    *_sig_r = dl_r;

  };

  void SetDelay(float _samples) {
    for (int i = 0; i < 3; i++) {
      delayLineL[i]->SetDelay(_samples);
    }

    for (int i = 0; i < 3; i++) {
      delayLineR[i]->SetDelay(_samples);
    }
  };

  void SetFeedback(float _f) {
    feedback = _f;
  }

private:
	static const int DL_MULT_COUNT = 13;

	DelayLine<T, max_size> *delayLineL[3];

  DelayLine<T, max_size>  *delayLineR[3];

  float feedback = .75f;

};