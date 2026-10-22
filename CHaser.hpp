#pragma once

#include <iostream>
#include <string>

class CHaserClient {
public:
    explicit CHaserClient();

    std::string get_ready();

    void action(const std::string& act);

    void walk_left();
    void walk_up();
    void walk_right();
    void walk_down();

    std::string look_left();
    std::string look_up();
    std::string look_right();
    std::string look_down();

    std::string search_left();
    std::string search_up();
    std::string search_right();
    std::string search_down();

    void put_left();
    void put_up();
    void put_right();
    void put_down();

private:
    std::string id;
};

inline void ch_walk(char d){
    std::cout << 'w' << d << '\n' << std::flush;
}

// ch_look sends look action then returns response
// forward declaration so callers above can call get_ready inline
std::string get_ready();

inline std::string ch_look(char d){
    std::cout << 'l' << d << '\n' << std::flush;
    return get_ready();
}

// read a line from stdin, return empty string on EOF/error (parity with C version)
inline std::string get_ready(){
    std::string s;
    if (std::getline(std::cin, s)) return s;
    return std::string();
}

inline std::string ch_search(char d){
    std::cout << 's' << d << '\n' << std::flush;
    return get_ready();
}

// ch_put is send-only
inline void ch_put(char d){
    std::cout << 'p' << d << '\n' << std::flush;
}

// convenience wrappers to match C header naming
inline void walk_right(){ ch_walk('r'); }
inline void walk_left(){ ch_walk('l'); }
inline void walk_up(){ ch_walk('u'); }
inline void walk_down(){ ch_walk('d'); }

inline std::string look_right(){ return ch_look('r'); }
inline std::string look_left(){ return ch_look('l'); }
inline std::string look_up(){ return ch_look('u'); }
inline std::string look_down(){ return ch_look('d'); }

inline std::string search_right(){ return ch_search('r'); }
inline std::string search_left(){ return ch_search('l'); }
inline std::string search_up(){ return ch_search('u'); }
inline std::string search_down(){ return ch_search('d'); }

inline void put_right(){ ch_put('r'); }
inline void put_left(){ ch_put('l'); }
inline void put_up(){ ch_put('u'); }
inline void put_down(){ ch_put('d'); }