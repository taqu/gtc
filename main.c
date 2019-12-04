#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <ncurses.h>
#include <form.h>
#include <menu.h>

#define  BUFFER_SIZE (255)

#define NI_NAME_MAX (15)
#define KEY_ESCAPE (27)

int32_t maximum(int32_t x0, int32_t x1)
{
    return x0<x1? x1 : x0;
}

int32_t minimum(int32_t x0, int32_t x1)
{
    return x0<x1? x0 : x1;
}

typedef struct NetInterface_
{
    char name_[NI_NAME_MAX+1];
} NetInterface;

int32_t getNetInterfaces(NetInterface** interfaces)
{
    struct if_nameindex* if_nameindices = if_nameindex();
    if(NULL == if_nameindices){
        *interfaces = NULL;
        return 0;
    }
    int32_t count = 0;
    for(struct if_nameindex* i = if_nameindices; 0!=i->if_index && NULL != i->if_name; ++i, ++count);

    *interfaces = (NetInterface*)malloc(sizeof(NetInterface)*count);
    count = 0;
    for(struct if_nameindex* i = if_nameindices; 0!=i->if_index && NULL != i->if_name; ++i, ++count){
        int32_t len = strlen(i->if_name);
        if(NI_NAME_MAX<=len){
            len = NI_NAME_MAX;
        }
        strcpy((*interfaces)[count].name_, i->if_name);
    }
    if_freenameindex(if_nameindices);
    return count;
}

typedef struct Netem_
{
    bool exists_;
    int32_t delay_;
    int32_t delay_jitter_;
    double loss_;
    int32_t bandwidth_; ///< k bits per second
} Netem;

bool execute(char* buffer, int32_t size)
{
    FILE* f = popen(buffer, "r");
    if(NULL == f){
        return false;
    }
    int32_t len = 0;
    while(!feof(f)){
        char* line = fgets(buffer+len, size-len, f);
        if(NULL == line){
            break;
        }
        len += strlen(line);
    }
    pclose(f);
    return true;
}

Netem checkNetem(const char* ifname)
{
#define DELIMITER (" ")
    Netem netem = {false, 0, 0, 0.0, 0};
    char buffer[BUFFER_SIZE+1];
    snprintf(buffer, BUFFER_SIZE, "tc qdisc show dev %s", ifname);
    if(!execute(buffer, BUFFER_SIZE)){
        return netem;
    }
    FILE* log = fopen("log.txt", "wb");
    fprintf(log, "%s\n", buffer);
    fclose(log);
    char* token = strtok(buffer, DELIMITER);
    while(NULL != token){

        if(0 == strcmp(token, "netem")){
            netem.exists_ = true;
        }else if(0 == strcmp(token, "tbf")){
            netem.exists_ = true;

        }else if(0 == strcmp(token, "delay")){
            token = strtok(NULL, DELIMITER);
            if(NULL == token){
                continue;
            }
            netem.delay_ = atoi(token);
            token = strtok(NULL, DELIMITER);
            if(NULL == token || NULL == strstr(token, "ms")){
                continue;
            }
            netem.delay_jitter_ = atoi(token);
        } else if(0 == strcmp(token, "loss")){
            token = strtok(NULL, DELIMITER);
            if(NULL == token){
                continue;
            }
            netem.loss_ = atof(token);

        } else if(0 == strcmp(token, "rate")){
            token = strtok(NULL, DELIMITER);
            if(NULL == token){
                continue;
            }
            netem.bandwidth_ = atoi(token);
            if(NULL != strstr(token, "Kbit")){
            }else if(NULL != strstr(token, "Mbit")){
                netem.bandwidth_ *= 1000;
            }else if(NULL != strstr(token, "Gbit")){
                netem.bandwidth_ *= 1000*1000;
            }else if(NULL != strstr(token, "bit")){
                netem.bandwidth_ /= 1000;
            }
        }
        token = strtok(NULL, DELIMITER);
    }
    return netem;
}



void deleteTC(const char* ifname)
{
    char buffer[BUFFER_SIZE+1];
    snprintf(buffer, BUFFER_SIZE, "tc qdisc del dev %s root", ifname);
    execute(buffer, BUFFER_SIZE);
}

void setNetem(const char* ifname, Netem* netem)
{
    char buffer[BUFFER_SIZE+1];
    if(netem->exists_){
        deleteTC(ifname);
    }
    if(netem->bandwidth_<=0){
        snprintf(buffer, BUFFER_SIZE, "tc qdisc add dev %s root netem delay %dms %dms loss %.1lf%%",
            ifname, netem->delay_, netem->delay_jitter_, netem->loss_);
        execute(buffer, BUFFER_SIZE);
    }else{
        snprintf(buffer, BUFFER_SIZE, "tc qdisc add dev %s root handle 1: tbf limit 256kb burst 32kb rate %dkbit",
            ifname, netem->bandwidth_);
        execute(buffer, BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "tc qdisc add dev %s parent 1: handle 10: netem delay %dms %dms loss %.1lf%%",
            ifname, netem->delay_, netem->delay_jitter_, netem->loss_);
        execute(buffer, BUFFER_SIZE);
    }
    *netem = checkNetem(ifname);
}

void deleteNetem(const char* ifname, Netem* netem)
{
    if(netem->exists_){
        deleteTC(ifname);
    }
    *netem = checkNetem(ifname);
}

enum State
{
    State_SelectInterface = 0,
    State_SetTC,
    State_End,
    State_Num,
};

typedef void(*proc_type)(void* data);

typedef struct Proc_
{
    proc_type init_;
    proc_type proc_;
    proc_type term_;
} Proc;

typedef struct Mode_
{
    int32_t current_;
    int32_t next_;
    Proc* proc_;
} Mode;

void initMode(Mode* mode, Proc* proc, int32_t initial)
{
    mode->current_ = -1;
    mode->next_ = initial;
    mode->proc_ = proc;
}

void update(Mode* mode, void* data)
{
    while(mode->current_ != mode->next_){
        if(0<=mode->current_ && NULL != mode->proc_[mode->current_].term_){
            mode->proc_[mode->current_].term_(data);
        }
        mode->current_ = mode->next_;
        if(NULL != mode->proc_[mode->next_].init_){
            mode->proc_[mode->next_].init_(data);
        }
    }
    if(NULL != mode->proc_[mode->current_].proc_){
        mode->proc_[mode->current_].proc_(data);
    }
}

#define NUM_FIELDS (10)
#define NUM_ITEMS (3)

typedef struct Context_
{
    int32_t key_;
    int32_t width_;
    int32_t height_;
    int32_t numInterfaces_;
    int32_t selected_;
    NetInterface* interfaces_;
    Netem netem_;
    Mode mode_;
    Proc procs_[State_Num];
    WINDOW* win_base_;

    WINDOW* win_main_;
    WINDOW* win_main_sub_;
    WINDOW* win_menu_;
    WINDOW* win_menu_sub_;
    WINDOW* win_status_;
    FIELD* fields_[NUM_FIELDS];
    FORM* form_;
    ITEM* items_[NUM_ITEMS];
    MENU* menu_;
    bool is_on_menu_;
} Context;

int32_t getLimitedSize(int32_t size, int32_t screenSize, int32_t border)
{
    size = minimum(size, screenSize);
    return maximum(size-border*2, 0);
}

int32_t getCentering(int32_t size, int32_t screenSize)
{
    return (screenSize - size)/2;
}

void deleteWindow(WINDOW** window)
{
    if(NULL != *window){
        delwin(*window);
        *window = NULL;
    }
}

void deleteField(FIELD** field)
{
    if(NULL != *field){
        free_field(*field);
        *field = NULL;
    }
}

void deleteForm(FORM** form)
{
    if(NULL != *form){
        free_form(*form);
        *form = NULL;
    }
}

void deleteItem(ITEM** item)
{
    if(NULL != *item){
        free_item(*item);
        *item = NULL;
    }
}

void deleteMenu(MENU** menu)
{
    if(NULL != *menu){
        free_menu(*menu);
        *menu = NULL;
    }
}

//---State_SelectInterface
//----------------------------------------------
/*
 +--------+
 | lo     |
 | en0    |
 +--------+
 +--------+
 | keys   |
 +--------+
 */
void State_SelectInterface_Init(void* data)
{
    erase();
    Context* context = (Context*)data;
    int32_t w = getLimitedSize(28, context->width_, 1);
    int32_t h = getLimitedSize(16, context->height_, 1);
    int32_t l = getCentering(w, context->width_);
    int32_t t = getCentering(h, context->height_);
    context->win_main_ = newwin(h-3, w, t, l);
    context->win_menu_ = newwin(3, w, t+h-3, l);

    //----------------
    wclear(context->win_menu_);
    box(context->win_menu_, 0, 0);
    wattron(context->win_menu_, COLOR_PAIR(2));
    mvwaddstr(context->win_menu_, 1, 1, "ESC");
    wattron(context->win_menu_, COLOR_PAIR(1));
    mvwaddstr(context->win_menu_, 1, 5, "Exit");

    wattron(context->win_menu_, COLOR_PAIR(2));
    mvwaddstr(context->win_menu_, 1, 10, "UP/DOWN");
    wattron(context->win_menu_, COLOR_PAIR(1));
    mvwaddstr(context->win_menu_, 1, 18, "Select");
    wattroff(context->win_menu_, COLOR_PAIR(1));
    wrefresh(context->win_menu_);

    timeout(-1);
    refresh();
}

void State_SelectInterface_Proc(void* data)
{
    Context* context = (Context*)data;

    wclear(context->win_main_);
    box(context->win_main_, 0, 0);
    for(int32_t i = 0; i<context->numInterfaces_; ++i){
        int32_t color = i==context->selected_? 2 : 1;
        wattron(context->win_main_, COLOR_PAIR(color));
        mvwaddstr(context->win_main_, i+1, 1, context->interfaces_[i].name_);
        wattroff(context->win_main_, COLOR_PAIR(color));
    }
    wrefresh(context->win_main_);
    box(context->win_menu_, 0, 0);
    wrefresh(context->win_menu_);
    refresh();
    context->key_ = getch();
    switch(context->key_){
    case KEY_DOWN:
        if(context->numInterfaces_<=++context->selected_){
            context->selected_ = 0;
        }
        break;
    case KEY_UP:
        if(--context->selected_<0){
            context->selected_ = maximum(0, context->numInterfaces_-1);
        }
        break;
    case 10: //ENTER
        context->mode_.next_ = State_SetTC;
        break;
    case KEY_ESCAPE:
        context->mode_.next_ = State_End;
        break;
    }
}

void State_SelectInterface_Term(void* data)
{
    Context* context = (Context*)data;
    deleteWindow(&context->win_menu_);
    deleteWindow(&context->win_main_);
}

//---State_SetTC
//----------------------------------------------
/*
 +--------+
 | form   |
 |        |
 +--------+
 +--------+
 | status |
 | keys   |
 +--------+
 +--------+
 | menu   |
 +--------+
 */
void setFieldInt(FIELD* field, int32_t value)
{
    char buffer[32];
    snprintf(buffer, 32, "%d", value);
    set_field_buffer(field, 0, buffer);
}

void setFieldDouble(FIELD* field, double value)
{
    char buffer[32];
    snprintf(buffer, 31, "%3.1lf", value);
    set_field_buffer(field, 0, buffer);
}

int32_t getFieldInt(FIELD* field)
{
    char* buffer = field_buffer(field, 0);
    if(NULL == field){
        return 0;
    }
    return atoi(buffer);
}

double getFieldDouble(FIELD* field)
{
    char* buffer = field_buffer(field, 0);
    if(NULL == field){
        return 0.0;
    }
    return atof(buffer);
}

void setNetemStatus(WINDOW* window, Netem* netem)
{
    char buffer[128];
    if(netem->exists_){
        snprintf(buffer, 127, "delay %dms jitter %dms loss %4.1lf%% bandwidth %d kbps",
            netem->delay_, netem->delay_jitter_, netem->loss_, netem->bandwidth_);
    }else{
        snprintf(buffer, 127, "none");
    }
    mvwaddstr(window, 1, 1, buffer);
}

void drawNetemStatus(WINDOW* window, Netem* netem)
{
    wclear(window);
    box(window, 0, 0);
    setNetemStatus(window, netem);
    wattron(window, COLOR_PAIR(2));
    mvwaddstr(window, 2, 1, "ESC");
    wattron(window, COLOR_PAIR(1));
    mvwaddstr(window, 2, 5, "Back");

    wattron(window, COLOR_PAIR(2));
    mvwaddstr(window, 2, 10, "TAB");
    wattron(window, COLOR_PAIR(1));
    mvwaddstr(window, 2, 14, "Switch");
    wattroff(window, COLOR_PAIR(1));
    wrefresh(window);
}

void State_SetTC_Init(void* data)
{
    Context* context = (Context*)data;
    context->netem_ = checkNetem(context->interfaces_[context->selected_].name_);

    erase();

    int32_t w = getLimitedSize(60, context->width_, 1);
    int32_t h = getLimitedSize(18, context->height_, 1);
    int32_t l = getCentering(w, context->width_);
    int32_t t = getCentering(h, context->height_);

    context->win_base_ = newwin(h, w, t, l);
    context->win_main_ = derwin(context->win_base_, 7, w-2, 1, 1);
    context->win_main_sub_ = derwin(context->win_main_, 5, w-4, 1, 1);
    context->win_status_ = derwin(context->win_base_, 4, w-2, 8, 1);
    context->win_menu_ = derwin(context->win_base_, 3, w-2, 12, 1);
    context->win_menu_sub_ = derwin(context->win_menu_, 1, w-4, 1, 1);

    //--------------------------------------------------
    keypad(context->win_main_, TRUE);
    box(context->win_main_, 0, 0);
    FIELD** fields = context->fields_;
    for(int32_t i = 0; i<NUM_FIELDS; ++i){
        fields[i] = NULL;
    }
    for(int32_t i = 0; i<4; ++i){
        fields[i] = new_field(1, 10, i+1, 15, 0, 0);
        set_field_back(fields[i], A_UNDERLINE);
        set_field_opts(fields[i], O_ACTIVE | O_VISIBLE | O_PUBLIC | O_EDIT);
        //set_field_back(fields[i], A_STANDOUT);
        field_opts_off(fields[i], O_AUTOSKIP);
        //set_field_just(fields[i], JUSTIFY_RIGHT);
    }
    set_field_type(fields[0], TYPE_INTEGER, 1, 0, 500);
    set_field_type(fields[1], TYPE_INTEGER, 1, 0, 500);
    set_field_type(fields[2], TYPE_NUMERIC, 1, 0.0, 100.0);
    set_field_type(fields[3], TYPE_INTEGER, 1, 0, 10000000);//10Gbps

    setFieldInt(fields[0], context->netem_.delay_);
    setFieldInt(fields[1], context->netem_.delay_jitter_);
    setFieldDouble(fields[2], context->netem_.loss_);
    setFieldInt(fields[3], context->netem_.bandwidth_);

    fields[4] = new_field(1, 12, 0, 0, 0, 0);
    fields[5] = new_field(1, 12, 1, 0, 0, 0);
    fields[6] = new_field(1, 12, 2, 0, 0, 0);
    fields[7] = new_field(1, 12, 3, 0, 0, 0);
    fields[8] = new_field(1, 12, 4, 0, 0, 0);
    fields[9] = NULL;
    for(int32_t i = 4; i<9; ++i){
        set_field_opts(fields[i], O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
    }
    set_field_buffer(fields[4], 0, context->interfaces_[context->selected_].name_);
    set_field_buffer(fields[5], 0, " Delay(ms)");
    set_field_buffer(fields[6], 0, "  Jitter(ms)");
    set_field_buffer(fields[7], 0, " Loss(%)");
    set_field_buffer(fields[8], 0, " Band(kbps)");

    context->form_ = new_form(fields);
    set_form_win(context->form_, context->win_main_);
    set_form_sub(context->form_, context->win_main_sub_);
    post_form(context->form_);
    set_field_back(current_field(context->form_), A_STANDOUT);

    //--------------------------------------------------
    drawNetemStatus(context->win_status_, &context->netem_);

    //--------------------------------------------------
    keypad(context->win_menu_, TRUE);
    box(context->win_menu_, 0, 0);
    ITEM** items = context->items_;
    items[0] = new_item("SET", "");
    items[1] = new_item("DELETE", "");
    items[2] = NULL;
    context->menu_ = new_menu(items);
    set_menu_format(context->menu_, 1, 2);

    set_menu_win(context->menu_, context->win_menu_);
    set_menu_sub(context->menu_, context->win_menu_sub_);
    set_menu_mark(context->menu_, "");
    post_menu(context->menu_);

    context->is_on_menu_ = false;
    set_menu_fore(context->menu_, A_NORMAL); //hide

    refresh();
    wrefresh(context->win_base_);
    wrefresh(context->win_main_);
    wrefresh(context->win_menu_);
    timeout(-1);
}

void drive_menu(Context* context, ITEM *item)
{
	const char *name = item_name(item);
	if(0 == strcmp(name, "SET")) {
        //read back
        FIELD** fields = context->fields_;
        context->netem_.delay_ = getFieldInt(fields[0]);
        context->netem_.delay_jitter_ = getFieldInt(fields[1]);
        context->netem_.loss_ = getFieldDouble(fields[2]);
        context->netem_.bandwidth_ = getFieldInt(fields[3]);
        setNetem(context->interfaces_[context->selected_].name_, &context->netem_);
	}else if(0 == strcmp(name, "DELETE")){
        deleteNetem(context->interfaces_[context->selected_].name_, &context->netem_);
    }
    drawNetemStatus(context->win_status_, &context->netem_);
}

void nextField(Context* context)
{
    set_field_back(current_field(context->form_), A_UNDERLINE);
    form_driver(context->form_, REQ_NEXT_FIELD);
    form_driver(context->form_, REQ_END_LINE);
    set_field_back(current_field(context->form_), A_STANDOUT);
}

void prevField(Context* context)
{
    set_field_back(current_field(context->form_), A_UNDERLINE);
    form_driver(context->form_, REQ_PREV_FIELD);
    form_driver(context->form_, REQ_END_LINE);
    set_field_back(current_field(context->form_), A_STANDOUT);
}

void switchToForm(Context* context)
{
    context->is_on_menu_ = false;
    set_field_back(current_field(context->form_), A_STANDOUT);

    set_menu_fore(context->menu_, A_NORMAL); //hide
    wrefresh(context->win_menu_);
}

void switchToMenu(Context* context)
{
    context->is_on_menu_ = true;
    set_field_back(current_field(context->form_), A_UNDERLINE);

	set_menu_fore(context->menu_, A_REVERSE); //show
    wrefresh(context->win_main_);
}

void State_SetTC_Proc(void* data)
{
    Context* context = (Context*)data;

    if(context->is_on_menu_){
        context->key_ = wgetch(context->win_menu_);
    } else{
        context->key_ = wgetch(context->win_main_);
    }
    //context->key_ = getch();
    switch(context->key_){
    case KEY_DOWN:
        if(!context->is_on_menu_){
            nextField(context);
        }
        break;
    case KEY_UP:
        if(!context->is_on_menu_){
            prevField(context);
        }
        break;
    case KEY_LEFT:
        if(context->is_on_menu_){
            menu_driver(context->menu_, REQ_LEFT_ITEM);
        }
        break;
    case KEY_RIGHT:
        if(context->is_on_menu_){
            menu_driver(context->menu_, REQ_RIGHT_ITEM);
        }
        break;
    case KEY_BACKSPACE:
        if(!context->is_on_menu_){
            form_driver(context->form_, REQ_DEL_PREV);
        }
        break;
    case 10://ENTER
        if(context->is_on_menu_){
            drive_menu(context, current_item(context->menu_));
        }
        break;
    case KEY_ESCAPE:
        context->mode_.next_ = State_SelectInterface;
        break;
    case '\t'://TAB
        if(context->is_on_menu_){
            switchToForm(context);
        }else{
            switchToMenu(context);
        }
        break;
    default:
        if(!context->is_on_menu_){
            form_driver(context->form_, context->key_);
        }
        break;
    }
    if(context->is_on_menu_){
        pos_menu_cursor(context->menu_);
        wrefresh(context->win_menu_);
    }else{
        pos_form_cursor(context->form_);
        wrefresh(context->win_main_);
    }
}

void State_SetTC_Term(void* data)
{
    Context* context = (Context*)data;
    unpost_menu(context->menu_);
    unpost_form(context->form_);

    deleteMenu(&context->menu_);
    deleteItem(&context->items_[0]);
    deleteItem(&context->items_[1]);
    deleteWindow(&context->win_menu_sub_);
    deleteWindow(&context->win_menu_);

    deleteWindow(&context->win_status_);

    deleteForm(&context->form_);
    for(int32_t i = 0; i<NUM_FIELDS; ++i){
        deleteField(&context->fields_[i]);
    }
    deleteWindow(&context->win_main_sub_);
    deleteWindow(&context->win_main_);
    deleteWindow(&context->win_base_);
}

void initContext(Context* context)
{
    context->procs_[State_SelectInterface].init_ = State_SelectInterface_Init;
    context->procs_[State_SelectInterface].proc_ = State_SelectInterface_Proc;
    context->procs_[State_SelectInterface].term_ = State_SelectInterface_Term;

    context->procs_[State_SetTC].init_ = State_SetTC_Init;
    context->procs_[State_SetTC].proc_ = State_SetTC_Proc;
    context->procs_[State_SetTC].term_ = State_SetTC_Term;

    context->procs_[State_End].init_ = NULL;
    context->procs_[State_End].proc_ = NULL;
    context->procs_[State_End].term_ = NULL;
    initMode(&context->mode_, context->procs_, State_SelectInterface);
    getmaxyx(stdscr, context->height_, context->width_);
    context->numInterfaces_ = 0;
    context->interfaces_ = NULL;
    context->numInterfaces_ = getNetInterfaces(&(context->interfaces_));
    context->selected_ = 0;
}

int32_t main(int32_t argc, char** argv)
{
    initscr();
    curs_set(0);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    bkgd(COLOR_PAIR(1));

    Context context_;
    initContext(&context_);

    while(State_End != context_.mode_.current_){
        update(&context_.mode_, &context_);
    }

    clear();
    endwin();
    free(context_.interfaces_);
    return 0;
}

