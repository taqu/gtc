#ifndef INC_FLFLOATSLIDERINPUT_H_
#define INC_FLFLOATSLIDERINPUT_H_
#include <FL/Fl_Group.H>

class Fl_Int_Input;
class Fl_Slider;

namespace fltk
{
    class FloatSliderInput : public Fl_Group
    {
    public:
        FloatSliderInput(int x, int y, int w, int h, const char* name = nullptr);

        float  value() const;
        void value(float val);
        void minumum(float val);
        float  minumum() const;
        void maximum(float val);
        float  maximum() const;
        void bounds(float low, float high);

    private:
        static void input_callback(Fl_Widget* widget, void* data);
        static void slider_callback(Fl_Widget* widget, void* data);

        Fl_Int_Input* input_;
        Fl_Slider* slider_;
        char recurseInput_;
        char recurseSlider_;
    };
}
#endif //INC_FLFLOATSLIDERINPUT_H_
