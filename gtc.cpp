#include <stdio.h>
#include <vector>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Button.H>
#include "gtc.h"
#include "NIC.h"
#include "QDisc.h"
#include "FlIntSliderInput.h"
#include "FlFloatSliderInput.h"

namespace
{
    std::vector<gtc::NIC> nics_;
    gtc::QDisc qdisc_;
    Fl_Window* window_ = nullptr;
    Fl_Select_Browser* browser_ = nullptr;
    Fl_Text_Buffer* textBuffer_ = nullptr;
    Fl_Text_Display* textDisplay_ = nullptr;

    fltk::IntSliderInput* inputDelay_ = nullptr;
    fltk::IntSliderInput* inputDelayJitter_ = nullptr;
    fltk::FloatSliderInput* inputLoss_ = nullptr;
    fltk::FloatSliderInput* inputCorrupt_ = nullptr;
    fltk::IntSliderInput* inputBandwidth_ = nullptr;

    Fl_Button* buttonAdd_ = nullptr;
    Fl_Button* buttonDelete_ = nullptr;
}

void update(const char* name)
{
    char buffer[128];
    qdisc_ = gtc::checkQDisc(name);
    snprintf(buffer, 127, "%dms %dms, %3.2f%%, %3.2f%%, %dKbit\n",
            qdisc_.delay_,
            qdisc_.delayJitter_,
            qdisc_.loss_,
            qdisc_.corrupt_,
            qdisc_.bandwidth_);
    textBuffer_->text(buffer);
    inputDelay_->value(qdisc_.delay_);
    inputDelayJitter_->value(qdisc_.delayJitter_);
    inputLoss_->value(qdisc_.loss_);
    inputCorrupt_->value(qdisc_.corrupt_);
    inputBandwidth_->value(qdisc_.bandwidth_);
}

void onClickNICBlowser(Fl_Widget* widget)
{
    const Fl_Select_Browser* browser = reinterpret_cast<const Fl_Select_Browser*>(widget);
    gtc::s32 selected = browser->value();
    if(selected<=0 || nics_.size()<static_cast<gtc::u32>(selected)){
        return;
    }
    update(nics_[selected-1].name());
}

void onClickAdd(Fl_Widget* widget)
{
    gtc::s32 selected = browser_->value();
    if(selected<=0 || nics_.size()<static_cast<gtc::u32>(selected)){
        return;
    }
    qdisc_.delay_ = inputDelay_->value();
    qdisc_.delayJitter_ = inputDelayJitter_->value();
    qdisc_.loss_ = inputLoss_->value();
    qdisc_.corrupt_ = inputCorrupt_->value();
    qdisc_.bandwidth_ = inputBandwidth_->value();
    qdisc_ = gtc::addQDisc(nics_[selected-1].name(), qdisc_);
#if 0
    char buffer[128];
    snprintf(buffer, 127, "%dms %dms, %3.1f%%, %3.1f%%, %dKbit\n",
            qdisc_.delay_,
            qdisc_.delayJitter_,
            qdisc_.loss_,
            qdisc_.corrupt_,
            qdisc_.bandwidth_);
    printf("%s", buffer);
#else
    update(nics_[selected-1].name());
#endif
}

void onClickDelete(Fl_Widget* widget)
{
    gtc::s32 selected = browser_->value();
    if(selected<=0 || nics_.size()<static_cast<gtc::u32>(selected)){
        return;
    }
    if(gtc::deleteQDisc(nics_[selected-1].name())){
        update(nics_[selected-1].name());
    }
}

int main(int argc, char** argv)
{
    static const gtc::s32 width = 350;
    static const gtc::s32 height = 290;
    static const gtc::s32 padding = 5;
    static const gtc::s32 textLine = 40;

    gtc::getNICs(nics_);

    window_ = new Fl_Window(width, height, "Traffic Control");
    window_->begin();
    browser_ = new Fl_Select_Browser(padding, padding, 100, height-padding*2-textLine);
    for(size_t i=0; i<nics_.size(); ++i){
        browser_->add(nics_[i].name());
    }
    browser_->callback(onClickNICBlowser);

    textBuffer_ = new Fl_Text_Buffer();
    textDisplay_ = new Fl_Text_Display(padding, height-padding-textLine, width-padding*2, textLine);
    textDisplay_->buffer(textBuffer_);

    //--- right menu
    //----------------------------------
    static const gtc::s32 SliderWidth = 240;
    gtc::s32 rx = 100 + padding;
    gtc::s32 ry = padding + 10;

    inputDelay_ = new fltk::IntSliderInput(rx, ry, SliderWidth, 25, "Delay (ms)");
    inputDelay_->bounds(0, 500);
    inputDelay_->value(0);
    ry += 25 + 10 + padding;

    inputDelayJitter_ = new fltk::IntSliderInput(rx, ry, SliderWidth, 25, "Delay Jitter (ms)");
    inputDelayJitter_->bounds(0, 500);
    inputDelayJitter_->value(0);
    ry += 25 + 10 + padding;

    inputLoss_ = new fltk::FloatSliderInput(rx, ry, SliderWidth, 25, "Loss (%)");
    inputLoss_->bounds(0.0f, 99.0f);
    inputLoss_->value(0.0f);
    ry += 25 + 10 + padding;

    inputCorrupt_ = new fltk::FloatSliderInput(rx, ry, SliderWidth, 25, "Corrupt (%)");
    inputCorrupt_->bounds(0.0f, 99.0f);
    inputCorrupt_->value(0.0f);
    ry += 25 + 10 + padding;

    inputBandwidth_ = new fltk::IntSliderInput(rx, ry, SliderWidth, 25, "Bandwidth (Kbit)");
    inputBandwidth_->bounds(0, 512000);
    inputBandwidth_->value(0);
    ry += 30 + padding;

    rx += 10;
    buttonAdd_ = new Fl_Button(rx, ry, 80, 25, "Add");
    buttonAdd_->callback(onClickAdd);
    buttonDelete_ = new Fl_Button(rx+80+padding, ry, 80, 25, "Delete");
    buttonDelete_->callback(onClickDelete);

    if(0<nics_.size()){
        browser_->select(1);
        update(nics_[0].name());
    }else{
        qdisc_ = {};
    }

    window_->end();
    window_->show(argc, argv);
    return Fl::run();
}
