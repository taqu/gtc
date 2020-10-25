#include "FlIntSliderInput.h"
#include <stdio.h>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Slider.H>

namespace fltk
{
    IntSliderInput::IntSliderInput(int x, int y, int w, int h, const char* name)
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

    int IntSliderInput::value() const
    {
        return((int)(slider_->value() + 0.5));
    }
    void IntSliderInput::value(int val)
    {
        slider_->value(val);
        slider_callback(this, this);
    }
    void IntSliderInput::minumum(int val)
    {
        slider_->minimum(val);
    }
    int IntSliderInput::minumum() const
    {
        return((int)slider_->minimum());
    }
    void IntSliderInput::maximum(int val)
    {
        slider_->maximum(val);
    }
    int IntSliderInput::maximum() const
    {
        return((int)slider_->maximum());
    }
    void IntSliderInput::bounds(int low, int high)
    {
        slider_->bounds(low, high);
    }

    void IntSliderInput::input_callback(Fl_Widget* widget, void* data)
    {
        IntSliderInput* sliderInput = reinterpret_cast<IntSliderInput*>(data);
        if(0 < sliderInput->recurseInput_){
            return;
        }
        sliderInput->recurseInput_ = 1;
        int val;
        if(sscanf(sliderInput->input_->value(), "%d", &val) <= 0) {
            val = 0;
        }
        sliderInput->slider_->value(val);
        sliderInput->recurseInput_ = 0;
    }

    void IntSliderInput::slider_callback(Fl_Widget* widget, void* data)
    {
        IntSliderInput* sliderInput = reinterpret_cast<IntSliderInput*>(data);
        if(0 < sliderInput->recurseSlider_){
            return;
        }
        sliderInput->recurseSlider_ = 1;
        char s[20];
        snprintf(s, 19, "%d", static_cast<int>(sliderInput->slider_->value() + 0.5));
        sliderInput->input_->value(s);
        sliderInput->recurseSlider_ = 0;
    }
}
