using namespace daisysp;
using namespace daisy;

class hdelay
{
public:
	hdelay() {};
	~hdelay() {};

	void Init(float _delay_lenght) {
    for (int i = 0; i < 3; ++i) {
      delaylineMemL[i].Init();
      delaylineL[i] = &delaylineMemL[i];
    }

    for (int i = 0; i < 3; ++i) {
      delaylineMemR[i].Init();
      delaylineR[i] = &delaylineMemR[i];
    }

		for (int i = 0; i < 3; i++) {
	    delaylineL[i]->SetDelay(_delay_lenght);
		}

	  for (int i = 0; i < 3; i++) {
	    delaylineR[i]->SetDelay(_delay_lenght);
	  }
	};

	void Process() {

  };

  void SetDelay(float _samples) {
    for (int i = 0; i < 3; i++) {
      delaylineL[i]->SetDelay(_samples);
    }

    for (int i = 0; i < 3; i++) {
      delaylineR[i]->SetDelay(_samples);
    }
  };

private:
	static const int DL_MULT_COUNT = 13;
	daisysp::DelayLine<float, 1> delaylineMemL[3];
	daisysp::DelayLine<float, 1> delaylineMemR[3];
	daisysp::DelayLine<float, 1> *delaylineL[3];
	daisysp::DelayLine<float, 1> *delaylineR[3];

	const float dl_mult[DL_MULT_COUNT] =         {0.0625f, 0.125f, 0.25f, 0.50f, 0.75f, 1.f, 2.f, 3.f, 4.f, 8.f, 16.f, 32.f, 64.f};
	std::string dl_mult_strings[DL_MULT_COUNT] = { "1/16",  "1/8", "1/4", "1/2", "3/4", "1", "2", "3", "4", "8", "16", "32", "64"};

};