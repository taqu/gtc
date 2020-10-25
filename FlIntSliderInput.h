#ifndef INC_FLINTSLIDERINPUT_H_
#define INC_FLINTSLIDERINPUT_H_
#include <FL/Fl_Group.H>

class Fl_Int_Input;
class Fl_Slider;

namespace fltk
{
    class IntSliderInput : public Fl_Group
    {
    public:
        IntSliderInput(int x, int y, int w, int h, const char* name = nullptr);

        int  value() const;
        void value(int val);
        void minumum(int val);
        int  minumum() const;
        void maximum(int val);
        int  maximum() const;
        void bounds(int low, int high);

    private:
        static void input_callback(Fl_Widget* widget, void* data);
        static void slider_callback(Fl_Widget* widget, void* data);

        Fl_Int_Input* input_;
        Fl_Slider* slider_;
        char recurseInput_;
        char recurseSlider_;
    };
}
#endif //INC_FLINTSLIDERINPUT_H_
