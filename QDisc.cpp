#include "QDisc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <sstream>
#include <iomanip>
#include <vector>

namespace gtc {
    namespace {
        static const s32 BUFFER_SIZE = 255;

        bool execute(const char* command) {
            FILE* f = popen(command, "r");
            if(GTC_NULL == f) {
                return false;
            }
            char buffer[BUFFER_SIZE + 1];
            while(!feof(f)) {
                char* line = fgets(buffer, BUFFER_SIZE, f);
                if(GTC_NULL == line) {
                    break;
                }
            }
            pclose(f);
            return true;
        }

        bool execute(std::stringstream& ss) {
            FILE* f = popen(ss.str().c_str(), "r");
            if(GTC_NULL == f) {
                return false;
            }
            ss.str("");

            char buffer[BUFFER_SIZE + 1];
            while(!feof(f)) {
                char* line = fgets(buffer, BUFFER_SIZE, f);
                if(GTC_NULL == line) {
                    break;
                }
                s32 len = strlen(line);
                for(s32 i = 0; i < len; ++i) {
                    if('\n' == line[i] || '\r' == line[i]) {
                        line[i] = ' ';
                    }
                }
                ss << line;
            }
            pclose(f);
            return true;
        }

        void tokenize(std::vector<const char*>& tokens, std::string& str,
            std::function<bool(char)> delemiter) {
            tokens.clear();
            tokens.reserve(8);

            size_t pos = 0;
            bool inToken = false;
            for(; pos < str.size(); ++pos) {
                if(delemiter(str[pos])) {
                    str[pos] = '\0';
                    inToken = false;
                } else if(!inToken) {
                    tokens.push_back(&str[pos]);
                    inToken = true;
                }
            }
        }

        void addNetemCommand(std::stringstream& ss, const QDisc qdisc)
        {
            ss << "netem";
            if(0<qdisc.delay_){
                ss << " delay " << qdisc.delay_ << "ms";
                if(0<qdisc.delayJitter_){
                    ss << " " << qdisc.delayJitter_ << "ms";
                }
            }
            if(0.0<qdisc.loss_){
                ss << " loss " << std::setprecision(3) << qdisc.loss_ << "%";
            }
            if(0.0<qdisc.corrupt_){
                ss << " corrupt " << std::setprecision(3) << qdisc.corrupt_ << "%";
            }
        }

        void addTbfCommand(std::stringstream& ss, const QDisc qdisc)
        {
            static const s32 Frequency = 250;
            //kbits
            s32 bandwidth = qdisc.bandwidth_;
            s32 burst = ((bandwidth)/8)/Frequency;
            if(burst<=8){
                burst = 8;
            }
            s32 limit = burst*8;

            ss << "tbf limit " << limit << "kb burst " << burst << "kb rate " << bandwidth << "kbit";
        }
    } // namespace

    bool QDisc::hasNetem() const
    {
        return 0 < delay_ || 0.0 < loss_ || 0.0 < corrupt_;
    }

    bool QDisc::hasTbf() const
    {
        return 0 < bandwidth_;
    }

    QDisc checkQDisc(const char* name) {
        std::stringstream ss;
        QDisc qdisc = {false, 0, 0, 0.0, 0.0, 0};
        ss << "tc qdisc show dev " << name;
        if(!execute(ss)) {
            return qdisc;
        }

        std::string str = ss.str();
        std::vector<const char*> tokens;
        tokenize(tokens, str,
            [](char c) { return ' ' == c || '\t' == c || '\v' == c; });
        for(size_t i = 0; i < tokens.size(); ++i) {
            if(0 == strcmp(tokens[i], "netem")) {
                qdisc.exists_ = true;
            } else if(0 == strcmp(tokens[i], "tbf")) {
                qdisc.exists_ = true;
            } else if(0 == strcmp(tokens[i], "delay")) {
                ++i;
                if(tokens.size() <= i) {
                    continue;
                }
                qdisc.delay_ = atoi(tokens[i]);
                size_t next = i + 1;
                if(tokens.size() <= next || !isdigit(tokens[next][0])) {
                    continue;
                }
                qdisc.delayJitter_ = atoi(tokens[next]);
                i = next;

            } else if(0 == strcmp(tokens[i], "loss")) {
                ++i;
                if(tokens.size() <= i) {
                    continue;
                }
                qdisc.loss_ = static_cast<f32>(atof(tokens[i]));
            } else if(0 == strcmp(tokens[i], "corrupt")) {
                ++i;
                if(tokens.size() <= i) {
                    continue;
                }
                qdisc.corrupt_ = static_cast<f32>(atof(tokens[i]));

            } else if(0 == strcmp(tokens[i], "rate")) {
                ++i;
                if(tokens.size() <= i) {
                    continue;
                }
                qdisc.bandwidth_ = atoi(tokens[i]);
                if(NULL != strstr(tokens[i], "Kbit")){
                } else if(NULL != strstr(tokens[i], "Mbit")) {
                    qdisc.bandwidth_ *= 1000;
                } else if(NULL != strstr(tokens[i], "Gbit")) {
                    qdisc.bandwidth_ *= 1000 * 1000;
                } else if(NULL != strstr(tokens[i], "bit")) {
                    qdisc.bandwidth_ /= 1000;
                }
            }
        } // for(size_t
        return qdisc;
    } // checkNetem


    bool deleteQDisc(const char* name) {
        char buffer[BUFFER_SIZE + 1];
        s32 size = snprintf(buffer, BUFFER_SIZE, "tc qdisc del dev %s root", name);
        buffer[size] = '\0';
        return execute(buffer);
    }

    QDisc addQDisc(const char* name, const QDisc& qdisc) {
        if(qdisc.exists_) {
            deleteQDisc(name);
        }
        std::stringstream ss;
        ss << "tc qdisc add dev " << name;

        std::string command;
        if(qdisc.hasNetem() && qdisc.hasTbf()){
            ss << " root handle 1: ";
            addTbfCommand(ss, qdisc);
            command = ss.str();
            execute(command.c_str());

            ss.str("");
            ss << "tc qdisc add dev " << name << " parent 1: handle 10: ";
            addNetemCommand(ss, qdisc);
            command = ss.str();
            execute(command.c_str());

        } else if(qdisc.hasNetem()){
            ss << " root ";
            addNetemCommand(ss, qdisc);
            command = ss.str();
            execute(command.c_str());

        } else if(qdisc.hasTbf()){
            ss << " root ";
            addTbfCommand(ss, qdisc);
            command = ss.str();
            execute(command.c_str());
        }
        return checkQDisc(name);
    }
} // namespace gtc
