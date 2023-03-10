#include <Bela.h>
#include <stdlib.h> 
#include <libraries/math_neon/math_neon.h>
#include <cmath>
#include "GrnTap.h"

// Class constructor
GrnTap::GrnTap(int t, GrnSrc& s, GUIData& g, Global& glb) : tapNum(t), Src(s), Gui(g), Glob(glb),
								 scheduler(t, s, g), env(scheduler), grn(scheduler) {}

// Class Destructor
GrnTap::~GrnTap(){}

// Initializer
void GrnTap::setup(){
	this->scheduler.setup();
	this->env.setup();
	this->grn.setup();
}

// Get and set tap params from GUI
void GrnTap::setParams(){
	this->scheduler.setAllParams();
}

// Begin scheduler, envelope and granulator pointers/calculations etc..
void GrnTap::process(){
	this->scheduler.tapScheduler(); // Calculates grnTrigs, grnPtrs and envPtrs
	this->env.process();            // Processes grn envelopes
	this->grn.process();            // Processes grn src
	
	// Random Panning
	float noise = 2.0 * (float)rand() / (float)RAND_MAX - 1.0;
	this->pan = this->scheduler.grnTrig ? (0.5 + (noise * 0.5 * this->Glob._spread)) : this->pan;

	this->fadeOut = process_killSwitch(this->scheduler.kill, 22050);
	
	/*
	// Autopan based on grnSz 
	float invSz = 1/this->scheduler.sz;
	float mod = ((int) powf_neon(-1, this->tapNum)) * sinf_neon(2.0f * M_PI * this->scheduler.envPtr[0] * invSz);
	this->pan = 0.5 + this->Glob._spread * 0.5 * mod; 
	*/
	
}

// Get output from tap
float GrnTap::out(int channel){
	float out = 0.0f;
	float pan = (channel == 0) ? sqrt(1 - this->pan) : sqrt(this->pan);
	
	for (unsigned int voice = 0; voice < this->scheduler.maxVoices; voice++){
		out += this->grn.grnOut[channel][voice] * this->env.envOut[voice] * pan * this->fadeOut;
	}
	
	if ((int) this->scheduler.dtime == 0){ 
		// If delay is 0, no output from tap
		return 0.0f;
	}
	else {
		return out;
	}
}

// Kill switch functionality: Individually enable or disable tap.
float GrnTap::process_killSwitch(int killState, int fadeLen){
	float out = 0.0f;
	
	this->fadeState = (this->scheduler.kill != this->prev_kill) ? 1 : 0; 

	switch(killState) {
		
		case 0:
			if (this->fadeCounter < fadeLen && this->fadeState == 1){
				out = 1 - (float) (this->fadeCounter/fadeLen);
				this->fadeCounter++;
			}
			else {
				out = 0.0f;
				this->fadeState = 0;
				this->fadeCounter = 0;
				this->prev_kill = this->scheduler.kill;
			}
			break;

		case 1:
			if (this->fadeCounter < fadeLen && this->fadeState == 1){
				out = (float) this->fadeCounter/fadeLen;
				this->fadeCounter++;
			}
			else {
				out = 1.0f;
				this->fadeState = 0;
				this->fadeCounter = 0;
				this->prev_kill = this->scheduler.kill;
			}
			break;
	}
	
	return out;
	
}
