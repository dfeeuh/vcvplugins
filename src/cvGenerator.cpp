#include "NoteGenerator.hpp"
#include "plugin.hpp"

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

	// This object mangages the generation of random notes
	NoteGenerator noteGen;

	bool running = true;
	dsp::SchmittTrigger clockTrigger;
	/** Phase of internal LFO */
	float phase = 0.f;
	float cv_out;

	CvGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(CLOCK_PARAM, -2.f, 6.f, 2.f, "Clock tempo", " bpm", 2.f, 60.f);

		noteGen.setKey(noteGen.C_MAG);
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
				unsigned randomNote = noteGen.generate();
				cv_out = noteGen.noteToCv(randomNote);
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