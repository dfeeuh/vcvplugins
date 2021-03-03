#include "NoteGenerator.hpp"
#include "plugin.hpp"

struct CvGenerator : Module {
	enum ParamIds {
		CLOCK_PARAM,
		KEY_PARAM,
        RUN_PARAM,
		RANGEMIN_PARAM,
		RANGEMAX_PARAM,
        MAJMIN_PARAM,
        SHARPFLAT_PARAM,
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
        RUNNING_LIGHT,
		NUM_LIGHTS
	};

	// This object mangages the generation of random notes
	NoteGenerator noteGen;

	bool running = true;
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger runningTrigger;

	/** Phase of internal LFO */
	float phase = 0.f;
	float cv_out;

	CvGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	    configParam(CLOCK_PARAM, -2.f, 6.f, 2.f, "Clock tempo", " bpm", 2.f, 60.f);
		configParam(RUN_PARAM, 0.f, 1.f, 0.f);
        configParam(KEY_PARAM, 0.f, 7.f, 0.f, "Key");
		configParam(RANGEMIN_PARAM, 0.f, 126.f, 0.f, "Minimum");
		configParam(RANGEMAX_PARAM, 1.f, 127.f, 127.f, "Maximum");
        configParam(MAJMIN_PARAM, 0.f, 1.f, 1.f, "Major Minor");
        configParam(SHARPFLAT_PARAM, -1.0, 1.f, 0.f, "Sharp Flat Natural");


		noteGen.setKey(noteGen.C_MAG);
	}

	void process(const ProcessArgs& args) override {
		// Run - TODO Add this
		if (runningTrigger.process(params[RUN_PARAM].getValue())) {
			running = !running;
		}
		
		bool gateIn = false;
		if (running) {
            // TODO read a parameter for Key, upper and lower limits here
            // Key could be set with dial A-G, Switch for Major/Minor, Natural/Sharp

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
				unsigned randomNote = noteGen.generate();
				cv_out = noteGen.noteToCv(randomNote);
			}  

			// To Do - manipulate durations by changing the thresholds!

			// Oops - What happens for external clock? There is no phase increment!
			outputs[GATE_OUTPUT].setVoltage((phase < 0.5f) ? 10.f : 0);
			outputs[CV_OUTPUT].setVoltage(cv_out);

			// Blink light at 1Hz	
			lights[BLINK_LIGHT].setBrightness((phase < 0.5) ? 1.f : 0.f);
		    lights[RUNNING_LIGHT].value = (running);
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.273, 83.552)), module, CvGenerator::CLOCK_PARAM));
		addParam(createParam<LEDButton>(mm2px(Vec(9.0, 21.35)), module, CvGenerator::RUN_PARAM));
		addChild(createLight<MediumLight<GreenLight>>(mm2px(Vec(9.0, 21.35)), module, CvGenerator::RUNNING_LIGHT));

		addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(25.4, 39.551)), module, CvGenerator::KEY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.273, 58.756)), module, CvGenerator::RANGEMIN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.683, 58.756)), module, CvGenerator::RANGEMAX_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(41.0, 40.7)), module, CvGenerator::MAJMIN_PARAM));
        addParam(createParamCentered<CKSSThree>(mm2px(Vec(9.0, 40.7)), module, CvGenerator::SHARPFLAT_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.172, 106.832)), module, CvGenerator::EXCLOC_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.683, 83.234)), module, CvGenerator::GATE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.786, 107.579)), module, CvGenerator::CV_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.4, 21.356)), module, CvGenerator::BLINK_LIGHT));
	}
};


Model* modelCvGenerator = createModel<CvGenerator, CvGeneratorWidget>("CvGenerator");