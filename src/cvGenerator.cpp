#include "NoteGenerator.hpp"
#include "plugin.hpp"
#include <algorithm>

struct CvGenerator : Module {
	enum ParamIds {
		CLOCK_PARAM,
		KEY_PARAM,
        RUN_PARAM,
		NOTECENTRE_PARAM,
		NOTERANGE_PARAM,
        MAJMIN_PARAM,
        SHARPFLAT_PARAM,
        LEVELQUANTISE_PARAM,
//        LEVELRANGE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		EXCLOC_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		CV_PITCH_OUTPUT,
        CV_LEVEL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		BLINK_LIGHT,
        RUNNING_LIGHT,
		NUM_LIGHTS
	};

	// This object mangages the generation of random notes, including key snapping
	NoteGenerator noteGen;

	bool running = true;
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger runningTrigger;

	/** Phase of internal LFO */
	float phase = 0.f;
	float cv_pitch;
    float cv_level;


	CvGenerator() :    
        phase{0.f}
    {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	    configParam(CLOCK_PARAM, -2.f, 6.f, 2.f, "Clock tempo", " bpm", 2.f, 60.f);
		configParam(RUN_PARAM, 0.f, 1.f, 0.f);

        // CV Pitch controls
        configParam(KEY_PARAM, 
            (float)NoteGenerator::KEY_BASE::CHROMATIC, 
            (float)NoteGenerator::KEY_BASE::G, 
            (float)NoteGenerator::KEY_BASE::A, "Key");
		configParam(NOTECENTRE_PARAM, 0.f, 127.f, 64.f, "Key Offset");
		configParam(NOTERANGE_PARAM, 1.f, 127.f, 64.f, "Key Range");
        configParam(MAJMIN_PARAM, 0.f, 1.f, 1.f, "Major Minor");
        configParam(SHARPFLAT_PARAM, -1.0, 1.f, 0.f, "Sharp Flat Natural");

        // CV Level controls: 0 - none, 1 - on/off, 2 - 4 levels
 		configParam(LEVELQUANTISE_PARAM, 0.f, 3.f, 0.f, "Level quantize");
         // Level range is Median +/- Range
		//configParam(LEVELRANGE_PARAM, 0.f, 5.f, 5.f, "Level Range");
	}

	void process(const ProcessArgs& args) override {
		// Run
		if (runningTrigger.process(params[RUN_PARAM].getValue())) {
			running = !running;
		}
		
		if (running) {
    		bool gateIn = false;
            bool bNewNote = false;

			if (inputs[EXCLOC_INPUT].isConnected()) {
				// External clock
				bNewNote = clockTrigger.process(inputs[EXCLOC_INPUT].getVoltage());
				gateIn = clockTrigger.isHigh();
			}
			else {
				// Internal clock
				float clockTime = std::pow(2.f, params[CLOCK_PARAM].getValue());
				phase += clockTime * args.sampleTime;	
				if (phase >= 1.f)
				{
                    bNewNote = true;
					phase = 0.f;
				}
                gateIn = (phase < 0.5f);
			}
		
			if (bNewNote) {
                // Key parameters are handled by their controls below                
                noteGen.setNoteOffset((unsigned)params[NOTECENTRE_PARAM].getValue());
                noteGen.setNoteRange((unsigned)params[NOTERANGE_PARAM].getValue());

				// if gateIn transitions to high generate a new value
				unsigned randomNote = noteGen.generatePitch();
				cv_pitch = noteGen.noteToCv(randomNote);

                float levelQuant = params[LEVELQUANTISE_PARAM].getValue();
                //float levelQuant = params[LEVELRANGE_PARAM].getValue();

                if (levelQuant == 0)
                    // No quantisation
                    cv_level = 10.f;
                else
                {
                    unsigned mask = (1 << (unsigned)(levelQuant)) - 1 ;
                    unsigned lvl = noteGen.generateVelocity() & mask;
                    // Snap to 2^levelQuant
                    cv_level = 10.f * (float)lvl / mask;
                    //DEBUG("CV level %f", cv_level);
                }
			}  

			// TODO - manipulate durations by changing the thresholds!
			outputs[GATE_OUTPUT].setVoltage(gateIn ? 10.f : 0);
			outputs[CV_PITCH_OUTPUT].setVoltage(cv_pitch);
            outputs[CV_LEVEL_OUTPUT].setVoltage(cv_level);

			// Blink light at 1Hz	
			lights[BLINK_LIGHT].setBrightness((phase < 0.5) ? 1.f : 0.f);
            lights[RUNNING_LIGHT].setBrightness(1.f);
		}
        else
        {
			outputs[CV_PITCH_OUTPUT].setVoltage(0);
   			outputs[GATE_OUTPUT].setVoltage(0);
            outputs[CV_LEVEL_OUTPUT].setVoltage(0);
            lights[BLINK_LIGHT].setBrightness(0.f);
            lights[RUNNING_LIGHT].setBrightness(0.f);
        }	    
	}
};

// Templated class to abstract the handling of updateKey
template <typename T>
struct KeyControl { 
private:
    NoteGenerator *noteGen_ = nullptr;

public:
    // Bind the noteGenerator object with the UI
    void setNoteGenerator(NoteGenerator *ng) { noteGen_ = ng; }

    void updateKey(T val) {
        if (noteGen_ != nullptr)
            noteGen_->updateKey(val);
    }
};

// Override onChange to update the key based on the value of RoundBlackSnapKnob
struct KeyControlKnob : RoundBlackSnapKnob, KeyControl<NoteGenerator::KEY_BASE> {
 
    void onChange(const event::Change& e) override {
        RoundBlackSnapKnob::onChange(e);

        updateKey((NoteGenerator::KEY_BASE)paramQuantity->getValue());
    }
};

// Override onChange to update the key based on the value of CKSS switch
struct MajorMinorSwitch : CKSS, KeyControl<bool> {

    void onChange(const event::Change& e) override {
        CKSS::onChange(e);

        updateKey(paramQuantity->getValue() == 1.0f ? false : true);
    }
};

// Override onChange to update the key based on the value of CKSSThree switch
struct AccidentalSwitch : CKSSThree, KeyControl<NoteGenerator::ACCIDENTAL> {

    void onChange(const event::Change& e) override {
        CKSSThree::onChange(e);

        updateKey((NoteGenerator::ACCIDENTAL)paramQuantity->getValue());
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
		addParam(createParamCentered<LEDButton>(mm2px(Vec(9.0, 21.35)), module, CvGenerator::RUN_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(9.0, 21.35)), module, CvGenerator::RUNNING_LIGHT));

        // Musical Key control parameters
        auto keyCtrl = createParamCentered<KeyControlKnob>(mm2px(Vec(25.4, 39.551)), module, CvGenerator::KEY_PARAM);
        keyCtrl->setNoteGenerator(&module->noteGen);
		addParam(keyCtrl);

        auto majminSw = createParamCentered<MajorMinorSwitch>(mm2px(Vec(41.0, 40.7)), module, CvGenerator::MAJMIN_PARAM);
        majminSw->setNoteGenerator(&module->noteGen);
        addParam(majminSw);

        auto accSw = createParamCentered<AccidentalSwitch>(mm2px(Vec(9.0, 40.7)), module, CvGenerator::SHARPFLAT_PARAM);
        accSw->setNoteGenerator(&module->noteGen);
        addParam(accSw);

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.273, 58.756)), module, CvGenerator::NOTECENTRE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.683, 58.756)), module, CvGenerator::NOTERANGE_PARAM));

        addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(56.688, 39.551)), module, CvGenerator::LEVELQUANTISE_PARAM));
        //addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(56.688, 58.756)), module, CvGenerator::LEVELRANGE_PARAM));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(57.4, 83.234)), module, CvGenerator::CV_LEVEL_OUTPUT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.172, 106.832)), module, CvGenerator::EXCLOC_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.683, 83.234)), module, CvGenerator::GATE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.786, 107.579)), module, CvGenerator::CV_PITCH_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.4, 21.356)), module, CvGenerator::BLINK_LIGHT));
	}
};


Model* modelCvGenerator = createModel<CvGenerator, CvGeneratorWidget>("CvGenerator");