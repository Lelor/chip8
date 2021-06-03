#pragma once
#ifndef CHIP8
#define CHIP8

#include <array>
#include <cstdint>
#include <random>
#include <string>

class Chip8 {
   private:
    std::array<uint8_t, 4096> memory;
    std::array<uint8_t, 16> V;
    std::array<uint16_t, 16> stack;
    std::array<uint8_t, 16> keys;
    std::array<uint8_t, 64 * 32> graphics;
    std::uint8_t delay_timer;
    std::uint8_t sound_timer;
    std::uint16_t I;
    std::uint16_t pc;
    std::uint16_t sp;
    std::uint16_t opcode;
    std::random_device rd;
    std::mt19937 gen;
    bool draw_flag;

   public:
    Chip8();
    void load_rom(std::string path);
    void emulate_cycle();
    void press_key(int keycode);
    void release_key(int keycode);
    void step_timers();
    bool get_draw_flag();
    void reset_draw_flag();
    std::uint8_t get_pixel_data(int i);
    std::uint8_t get_sound_timer();
};
#endif