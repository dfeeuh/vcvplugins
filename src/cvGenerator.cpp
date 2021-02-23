#include "plugin.hpp"

#define NUM_NOTES 7

struct CvGenerator : Module {
	enum ParamIds {
		CLOCK_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		EXCLOC_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		BLINK_LIGHT,
		NUM_LIGHTS
	};

	typedef enum keyIds {
		C_MAG=0,
		NUM_KEYS
	} KEY;

	const unsigned keyMapBasis[NUM_NOTES] = {0, 2, 4, 5, 7, 9, 11};
	unsigned keyMap[NUM_NOTES];

	bool running = true;
	dsp::SchmittTrigger clockTrigger;
	/** Phase of internal LFO */
	float phase = 0.f;

	uint16_t start_state;
	uint16_t lfsr;
	float cv_out;

	CvGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(CLOCK_PARAM, -2.f, 6.f, 2.f, "Clock tempo", " bpm", 2.f, 60.f);

		start_state = 0xACE1u;  /* Any nonzero start state will work. */
		lfsr = start_state;
	}

	// run the linear feedback shift register
	// From https://en.wikipedia.org/wiki/Linear-feedback_shift_register#Galois_LFSRs
	inline unsigned lfsr2(void)
	{
#ifndef LEFT
		unsigned lsb = lfsr & 1u;  /* Get LSB (i.e., the output bit). */
		lfsr >>= 1;                /* Shift register */
		if (lsb)                   /* If the output bit is 1, */
			lfsr ^= 0xB400u;       /*  apply toggle mask. */
#else
		unsigned msb = (int16_t) lfsr < 0;   /* Get MSB (i.e., the output bit). */
		lfsr <<= 1;                          /* Shift register */
		if (msb)                             /* If the output bit is 1, */
			lfsr ^= 0x002Du;                 /*  apply toggle mask. */
#endif
		return lfsr;
	}

	void generateNewKeyIds(KEY newKeyId)
	{
		// Everytime a new key is selected, generate a new key map
		for (unsigned i=0; i<NUM_NOTES; i++)
		{
			// Key = (C major + new key root note) modulo 12
			keyMap[i] = (newKeyId + keyMapBasis[i]) % 12;
		}

		// Sort to make all notes in order
		std::sort(keyMap, keyMap+NUM_NOTES);
	}

	inline unsigned snapToKey(unsigned midiNoteIn, KEY keyId)
	{
		// First get octave and map midiNoteIn to 0 to 11:
		// unsigned octave = midiNoteIn / 12;
		// unsigned basisNote  = midiNoteIn - (octave * 12);

		// Then use a binary search algorithm to find the nearest value.

		// In order to cope with numbers exactly half, e.g. 10 for C Major 
		// is exactly half way between 9 and 11, the LFSR generates
		// Qx.1 (i.e. with 0 or 0.5). Then round up and using a search algorithm
		// that favours the lower side (10 will snap to 9, 10.5 will snap to 11).
		// Read https://en.wikipedia.org/wiki/Binary_search_algorithm
		// This looks promising: https://thecleverprogrammer.com/2020/11/11/binary-search-in-c/

		// return (snappedNote + octave * 12);
		return midiNoteIn;
	}

	void process(const ProcessArgs& args) override {
		// Run - TODO Add this
		// if (runningTrigger.process(params[RUN_PARAM].getValue())) {
		// 	running = !running;
		// }
		
		bool gateIn = false;
		if (running) {
			if (inputs[EXCLOC_INPUT].isConnected()) {
				// External clock
				clockTrigger.process(inputs[EXCLOC_INPUT].getVoltage());
				gateIn = clockTrigger.isHigh();
			}
			else {
				// Internal clock
				float clockTime = std::pow(2.f, params[CLOCK_PARAM].getValue());
				phase += clockTime * args.sampleTime;	
				if (phase >= 1.f)
				{
					gateIn = true;
					phase = 0.f;
				}
			}
		
			if (gateIn)
			{
				// if gateIn transitions to high generate a new value
				unsigned randomNote = lfsr2() & 0x7F;
				randomNote = snapToKey(randomNote, C_MAG);
				cv_out = (randomNote - 60.0f) / 12.f;
			}  

			// To Do - manipulate durations by changing the thresholds!

			// Oops - What happens for external clock? There is no phase increment!
			outputs[GATE_OUTPUT].setVoltage((phase < 0.5f) ? 10.f : 0);
			outputs[CV_OUTPUT].setVoltage(cv_out);

			// Blink light at 1Hz	
			lights[BLINK_LIGHT].setBrightness((phase < 0.5) ? 1.f : 0.f);
		}
	}
};

struct CvGeneratorWidget : ModuleWidget {
	CvGeneratorWidget(CvGenerator* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CvGenerator.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.62, 38.4)), module, CvGenerator::CLOCK_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.519, 61.679)), module, CvGenerator::EXCLOC_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.574, 83.234)), module, CvGenerator::GATE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.678, 107.579)), module, CvGenerator::CV_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15.64, 21.356)), module, CvGenerator::BLINK_LIGHT));
	}
};


Model* modelCvGenerator = createModel<CvGenerator, CvGeneratorWidget>("CvGenerator");