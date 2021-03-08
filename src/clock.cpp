#include "plugin.hpp"


struct Clock : Module {
	enum ParamIds {
		RATE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		CLOCK_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

  	float phase = 0.f;

	Clock() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	    configParam(RATE_PARAM, -2.f, 6.f, 2.f, "Clock tempo", " bpm", 2.f, 60.f);
	}

	void process(const ProcessArgs& args) override {
        float clockTime = std::pow(2.f, params[RATE_PARAM].getValue());
        phase += clockTime * args.sampleTime;	
        if (phase >= 1.f)
            phase = 0.f;

        outputs[CLOCK_OUTPUT].setVoltage((phase < 0.5f) ? 10.f : 0.f);
	}
};


struct ClockWidget : ModuleWidget {
	ClockWidget(Clock* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Clock.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.24, 46.063)), module, Clock::RATE_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 108.713)), module, Clock::CLOCK_OUTPUT));
	}
};


Model* modelClock = createModel<Clock, ClockWidget>("Clock");