#include "FlFloatSliderInput.h"
#include <stdio.h>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Slider.H>

namespace fltk
{
    FloatSliderInput::FloatSliderInput(int x, int y, int w, int h, const char* name)
        : Fl_Group(x, y, w, h, name)
        , recurseInput_(0)
        , recurseSlider_(0)
    {
        int in_w = 55;
        input_ = new Fl_Int_Input(x, y, in_w, h);
        input_->callback(input_callback, this);
        input_->when(FL_WHEN_ENTER_KEY | FL_WHEN_NOT_CHANGED);

        slider_ = new Fl_Slider(x + in_w, y, w - in_w, h);
        slider_->type(1);
        slider_->callback(slider_callback, this);

        bounds(0, 10);
        value(0);
        end();
    }

    float FloatSliderInput::value() const
    {
        return static_cast<float>(slider_->value());
    }
    void FloatSliderInput::value(float val)
    {
        slider_->value(val);
        slider_callback(this, this);
    }
    void FloatSliderInput::minumum(float val)
    {
        slider_->minimum(val);
    }
    float FloatSliderInput::minumum() const
    {
        return static_cast<float>(slider_->minimum());
    }
    void FloatSliderInput::maximum(float val)
    {
        slider_->maximum(val);
    }
    float FloatSliderInput::maximum() const
    {
        return static_cast<float>(slider_->maximum());
    }
    void FloatSliderInput::bounds(float low, float high)
    {
        slider_->bounds(low, high);
    }

    void FloatSliderInput::input_callback(Fl_Widget* widget, void* data)
    {
        FloatSliderInput* sliderInput = reinterpret_cast<FloatSliderInput*>(data);
        if(0 < sliderInput->recurseInput_){
            return;
        }
        sliderInput->recurseInput_ = 1;
        float val;
        if(sscanf(sliderInput->input_->value(), "%f", &val) <= 0) {
            val = 0.0f;
        }
        sliderInput->slider_->value(val);
        sliderInput->recurseInput_ = 0;
    }

    void FloatSliderInput::slider_callback(Fl_Widget* widget, void* data)
    {
        FloatSliderInput* sliderInput = reinterpret_cast<FloatSliderInput*>(data);
        if(0 < sliderInput->recurseSlider_){
            return;
        }
        sliderInput->recurseSlider_ = 1;
        char s[32];
        snprintf(s, 31, "%4.2f", static_cast<float>(sliderInput->slider_->value()));
        sliderInput->input_->value(s);
        sliderInput->recurseSlider_ = 0;
    }
}
