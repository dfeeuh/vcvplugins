#include "plugin.hpp"
#include <iomanip>


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
    float clockTime;

	Clock() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	    configParam(RATE_PARAM, -2.f, 6.f, 2.f, "Clock tempo", " bpm", 2.f, 60.f);
	}

	void process(const ProcessArgs& args) override {
        //clockTime set directly from its widget
        phase += clockTime * args.sampleTime;	
        if (phase >= 1.f)
            phase = 0.f;

        outputs[CLOCK_OUTPUT].setVoltage((phase < 0.5f) ? 10.f : 0.f);
	}
};

struct DisplayWidget : TransparentWidget {

    float *value = NULL;
    std::shared_ptr<Font> font;

    DisplayWidget() {
        font = APP->window->loadFont(asset::plugin(pluginInstance, "res/Segment7Standard.ttf"));
    };

    void draw(const DrawArgs &args) override {
        if (!value) {
            return;
        }

        // text 
        nvgFontSize(args.vg, 18);
        nvgFontFaceId(args.vg, font->handle);
        nvgTextLetterSpacing(args.vg, 2.5);

        std::stringstream to_display;   
        to_display << std::right  << std::setw(5) << (unsigned)((*value) * 60.f);
        //to_display << std::right << *value;

        Vec textPos = Vec(4.0f, 17.0f); 
        /*
        NVGcolor textColor = nvgRGB(0xdf, 0xd2, 0x2c);
        nvgFillColor(args.vg, nvgTransRGBA(textColor, 16));
        nvgText(args.vg, textPos.x, textPos.y, "~~~~~", NULL);
        textColor = nvgRGB(0xda, 0xe9, 0x29);
        nvgFillColor(args.vg, nvgTransRGBA(textColor, 16));
        nvgText(args.vg, textPos.x, textPos.y, "\\\\\\\\\\", NULL);
        */
        NVGcolor textColor = nvgRGB(0xf0, 0x00, 0x00);
        nvgFillColor(args.vg, textColor);
        nvgText(args.vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
    }
};

struct ClockCtrlKnob : RoundBlackKnob
{
    float *rate = nullptr;

    void bind(float *x)
    {
        rate = x;
    }

    void onChange(const event::Change& e) override
    {
        RoundBlackKnob::onChange(e);

        if (rate != nullptr)
        {
            *rate = std::pow(2.f, paramQuantity->getValue());
        }
    }
};


struct ClockWidget : ModuleWidget {
	ClockWidget(Clock* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Clock.svg")));

        // Use a custom knob to bind value
        ClockCtrlKnob *bpmKnob = createParamCentered<ClockCtrlKnob>(mm2px(Vec(15.24, 46.063)), module, Clock::RATE_PARAM);
		addParam(bpmKnob);
        
        // DISPLAY 
		DisplayWidget *display = new DisplayWidget();
		display->box.pos = Vec(14,50);
		display->box.size = Vec(70, 20);
		if (module) {
            display->value = &module->clockTime;
            bpmKnob->bind(&module->clockTime);
        }
		addChild(display);

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));



		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 108.713)), module, Clock::CLOCK_OUTPUT));
	}
};


Model* modelClock = createModel<Clock, ClockWidget>("Clock");