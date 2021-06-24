// SoundSynth.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
/*
* Author: Saransh Jindal
* Following a tutorial(https://github.com/OneLoneCoder/synth) provided by OneLoneCoder for fun.
*/

#include <iostream>
using namespace std;

#include"olcNoiseMakerAdv.h"


namespace synth{
	//utility functions

	double omega(double hertz) {
		// convert to angular velocity
		return hertz * 2.0 * PI;
	}

	struct note {
		int id;
		double time_on;
		double time_off;
		bool active;
		int channel;

		note() {
			id = 0;
			time_on = 0;
			time_off = 0;
			active = false;
			channel = 0;

		}
	};

	//oscillator

	const int OSC_SINE = 0;
	const int OSC_SQUARE = 1;
	const int OSC_TRIANGLE = 2;
	const int OSC_SAW_ANA = 3;
	const int OSC_SAW_DIG = 4;
	const int OSC_NOISE = 5;

	double osc(double dHertz, double dTime, int nType = OSC_SINE, double dLFOHertz = 0.0, double dLFOAmp = 0.0, double dCustom = 50.0) {

		double dFreq = omega(dHertz) * dTime + dLFOAmp * dHertz * sin(omega(dLFOHertz) * dTime);

		switch (nType) {

		case OSC_SINE:
			return sin(dFreq);

		case OSC_SQUARE:
			return sin(dFreq) > 0 ? 1 : -1;

		case OSC_TRIANGLE:
			return asin(sin(dFreq) * 2 / PI);

		case OSC_SAW_ANA:
		{
			double output = 0;
			for (double i = 1.0; i < dCustom; i++) {
				output += sin(i * dFreq) / i;
			}
			return output * (2.0 / PI);
		}

		case OSC_SAW_DIG: {
			return (2 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2));
		}

		case OSC_NOISE: {
			return 2.0 * (double)rand() / (double)RAND_MAX - 1.0;
		}
		default:
			return 0.0;
		}
	}

	// convert to frequenct from scale
	const int SCALE_DEFAULT = 0;
	double scale(const int nNoteID, const int nScaleID = SCALE_DEFAULT) {
		switch (nScaleID) {
		case SCALE_DEFAULT:
		default:
			return 256 * pow(1.0594630943592952645618252949463, nNoteID);
		}
	}

	struct envelope {
		virtual double amp(const double dTime, const double dTimeOn, const double dTimeOff) = 0;
	};

	struct envelope_adsr :public envelope {
		double dAttackTime;
		double dDecayTime;
		double dReleaseTime;

		double dSustainAmp;
		double dStartAmp;

		envelope_adsr() {
			dAttackTime = 0.1;
			dDecayTime = 0.1;
			dStartAmp = 1.0;
			dSustainAmp = 1.0;
			dReleaseTime = 0.2;
		}

		virtual double amp(const double dTime, const double dTimeOn, const double dTimeOff) {
			double dAmp = 0;
			double dReleaseAmp = 0;
	
			if (dTimeOn>dTimeOff) {
				// note is on

				double dLifeTime = dTime - dTimeOn;
				if (dLifeTime <= dAttackTime) {
					//attack phase
					dAmp = (dLifeTime / dAttackTime) * dStartAmp;
				}
				else if (dLifeTime <= dAttackTime + dDecayTime) {
					//decay phase
					dAmp = ((dSustainAmp - dStartAmp) / dDecayTime) * (dLifeTime - dAttackTime) + dStartAmp;
				}
				else {
					//sustain phase
					dAmp = dSustainAmp;
				}
			}
			else {
				// note is off
				double dLifeTime = dTimeOff - dTimeOn;
				if (dLifeTime <= dAttackTime) {
					//attack phase
					dReleaseAmp = (dLifeTime / dAttackTime) * dStartAmp;
				}
				else if (dLifeTime <= dAttackTime + dDecayTime) {
					//decay phase
					dReleaseAmp = ((dSustainAmp - dStartAmp) / dDecayTime) * (dLifeTime - dAttackTime) + dStartAmp;
				}
				else {
					//sustain phase
					dReleaseAmp = dSustainAmp;
				}
				dAmp = dReleaseAmp - (dTime - dTimeOff) / dReleaseTime * dReleaseAmp;
			}

			if (dAmp < 0.0001) {
				dAmp = 0;
			}

			return dAmp;
		}
	};

	double env(const double dTime, envelope& env, const double dTimeOn, const double dTimeOff) {
		return env.amp(dTime, dTimeOn, dTimeOff);
	}

	struct instrument_base {
		double dVol;
		synth::envelope_adsr env;
		virtual double sound(const double dTime, synth::note n, bool& bNoteFinished) = 0;
	};

	struct instrument_bell :public instrument_base {
		instrument_bell() {
			env.dAttackTime = 0.01;
			env.dDecayTime = 1.0;
			env.dSustainAmp = 0.0;
			env.dReleaseTime = 1.0;

			dVol = 1;
		}
		virtual double sound(const double dTime, synth::note n, bool& bNoteFinished) {
			double dAmp = synth::env(dTime, env, n.time_on, n.time_off);
			if (dAmp <= 0) bNoteFinished = true;
			double dSound =
				+ 1.00 * synth::osc(n.time_on - dTime, synth::scale(n.id + 12), synth::OSC_SINE, 5, 0.001)
				+ 0.50 * synth::osc(n.time_on - dTime, synth::scale(n.id + 24))
				+ 0.25 * synth::osc(n.time_on - dTime, synth::scale(n.id + 36));
			return dAmp * dVol * dSound;
		}
	};

	struct instrument_bell8 :public instrument_base {
		instrument_bell8() {
			env.dAttackTime = 0.01;
			env.dDecayTime = 0.5;
			env.dSustainAmp = 0.8;
			env.dReleaseTime = 1.0;

			dVol = 1;
		}
		virtual double sound(const double dTime, synth::note n, bool& bNoteFinished) {
			double dAmp = synth::env(dTime, env, n.time_on, n.time_off);
			if (dAmp <= 0) bNoteFinished = true;
			double dSound =
				+ 1.00 * synth::osc(n.time_on - dTime, synth::scale(n.id), synth::OSC_SQUARE, 5, 0.001)
				+ 0.50 * synth::osc(n.time_on - dTime, synth::scale(n.id + 12))
				+ 0.25 * synth::osc(n.time_on - dTime, synth::scale(n.id + 24));
			return dAmp * dVol * dSound;
		}
	};

	struct instrument_harmonica :public instrument_base {
		instrument_harmonica() {
			env.dAttackTime = 0.05;
			env.dDecayTime = 1.0;
			env.dSustainAmp = 0.95;
			env.dReleaseTime = 0.1;

			dVol = 1.0;
		}
		virtual double sound(const double dTime, synth::note n, bool& bNoteFinished) {
			double dAmp = synth::env(dTime, env, n.time_on, n.time_off);
			if (dAmp <= 0) bNoteFinished = true;
			double dSound =
				+1.00 * synth::osc(n.time_on - dTime, synth::scale(n.id), synth::OSC_SQUARE, 5, 0.001)
				+ 0.50 * synth::osc(n.time_on - dTime, synth::scale(n.id + 12), synth::OSC_SQUARE)
				+ 0.05 * synth::osc(n.time_on - dTime, synth::scale(n.id + 24), synth::OSC_NOISE);
			return dAmp * dVol * dSound;
		}
	};
}

vector<synth::note>vecNotes;
mutex muxNotes;
synth::instrument_bell instBell;
synth::instrument_harmonica instHarm;

typedef bool(*lambda)(synth::note const& item);
template<class T>
void safe_remove(T& v, lambda f) {
	auto n = v.begin();
	while (n != v.end()) {
		if (!f(*n)) n = v.erase(n);
		else ++n;
	}
}

double MakeNoise(int nChannel, double dTime) {
	unique_lock<mutex>lm(muxNotes);
	double dMixedOutput = 0;

	for (auto& n : vecNotes) {
		bool bNoteFinished = false;
		double dSound = 0;
		if (n.channel == 2) dSound = instBell.sound(dTime, n, bNoteFinished);
		if (n.channel == 1) dSound += instHarm.sound(dTime, n, bNoteFinished) * 0.5;
		dMixedOutput += dSound;
		if (bNoteFinished && n.time_off > n.time_on) {
			n.active = false;
		}
	}
	safe_remove<vector<synth::note>>(vecNotes, [](synth::note const& item) {return item.active; });

	return dMixedOutput * 0.2;
}

int main()
{
	vector<wstring>devices = olcNoiseMaker<short>::Enumerate(); //getting sound hardware
	for (auto d : devices) wcout << "Found output device: " << d << endl;

	wcout << endl <<
		"|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |" << endl <<
		"|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |" << endl <<
		"|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__" << endl <<
		"|     |     |     |     |     |     |     |     |     |     |" << endl <<
		"|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |" << endl <<
		"|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|" << endl << endl;
	// ^^ layout of keys wrt to the keyboard

	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512); //sample rate etc
	sound.SetUserFunction(MakeNoise);
	
	char keyboard[129];
	memset(keyboard, ' ', 127);
	keyboard[128] = '\0';

	auto clock_old_time = chrono::high_resolution_clock::now();
	auto clock_real_time = chrono::high_resolution_clock::now();
	double elapsed_time = 0;

	while (1) {
		for (int k = 0; k < 16; k++) {
			short nKeyState = GetAsyncKeyState((unsigned char)"ZSXCFVGBNJMK\xbcL\xbe\xbf"[k]);
			double dTimeNow = sound.GetTime();
			muxNotes.lock();
			auto noteFound = find_if(vecNotes.begin(), vecNotes.end(), [&k](synth::note const& item) { return item.id == k; });
			if (noteFound == vecNotes.end()) {
				// note not in vector yet
				if (nKeyState & 0x8000) {
					// key pressed, add to vector
					synth::note n;
					n.id = k;
					n.time_on = dTimeNow;
					n.channel = 1;
					n.active = true;
					vecNotes.emplace_back(n);
				}
			}
			else {
				if (nKeyState & 0x8000) {
					if (noteFound->time_off > noteFound->time_on) {
						noteFound->time_on = dTimeNow;
						noteFound->active = true;
					}
				}
				else {
					if (noteFound->time_off < noteFound->time_on) {
						noteFound->time_off = dTimeNow;
					}
				}
			}
			muxNotes.unlock();
		}
		wcout << "\rNotes: " << vecNotes.size() << "     ";
	}

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
