#include "chip8.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

Chip8::Chip8() {
    pc = 0x200;

    memory.fill(0);
    V.fill(0);
    keys.fill(0);
    graphics.fill(0);
    stack.fill(0);

    delay_timer = 0;
    sound_timer = 0;
    I = 0;
    sp = 0;
    opcode = 0;
    draw_flag = false;

    gen.seed(rd());

    std::array<std::uint8_t, 80> fontset = {{
        0xF0, 0x90, 0x90, 0x90, 0xF0,
        0x20, 0x60, 0x20, 0x20, 0x70,
        0xF0, 0x10, 0xF0, 0x80, 0xF0,
        0xF0, 0x10, 0xF0, 0x10, 0xF0,
        0x90, 0x90, 0xF0, 0x10, 0x10,
        0xF0, 0x80, 0xF0, 0x10, 0xF0,
        0xF0, 0x80, 0xF0, 0x90, 0xF0,
        0xF0, 0x10, 0x20, 0x40, 0x40,
        0xF0, 0x90, 0xF0, 0x90, 0xF0,
        0xF0, 0x90, 0xF0, 0x10, 0xF0,
        0xF0, 0x90, 0xF0, 0x90, 0x90,
        0xE0, 0x90, 0xE0, 0x90, 0xE0,
        0xF0, 0x80, 0x80, 0x80, 0xF0,
        0xE0, 0x90, 0x90, 0x90, 0xE0,
        0xF0, 0x80, 0xF0, 0x80, 0xF0,
        0xF0, 0x80, 0xF0, 0x80, 0x80 
    }};
    for (int i = 0; i < 80; ++i) {
        memory[i] = fontset[i];
    }
}

void Chip8::load_rom(std::string path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::ifstream::pos_type file_size = file.tellg();
    std::vector<std::uint8_t> buffer(file_size);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    for (int i = 0; i < file_size; i++) {
        memory[i + 512] = buffer[i];
    }
}

void Chip8::emulate_cycle() {
    opcode = memory[pc] << 8 | memory[pc + 1];
    std::uint16_t x = (opcode & 0x0F00) >> 8;
    std::uint16_t y = (opcode & 0x00F0) >> 4;
    std::uint16_t kk = opcode & 0x00FF;
    std::uint16_t n = opcode & 0x000F;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode) {
                case 0x00E0:
                    graphics.fill(0);
                    draw_flag = true;
                    pc += 2;
                    break;
                case 0x00EE:
                    pc = stack[--sp];
                    pc += 2;
                    break;
                default:
                    std::cerr << "Unmapped opcode: " << opcode << "\n";
            }
            break;
        case 0x1000:
            pc = opcode & 0x0FFF;
            break;
        case 0x2000:
            stack[sp++] = pc;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000:
            pc += 2;
            if (V[x] == kk) {
                pc += 2;
            }
            break;
        case 0x4000:
            pc += 2;
            if (V[x] != kk) {
                pc += 2;
            }
            break;
        case 0x5000:
            pc += 2;
            if (V[x] == V[y]) {
                pc += 2;
            }
            break;
        case 0x6000:
            pc += 2;
            V[x] = kk;
            break;
        case 0x7000:
            pc += 2;
            V[x] += kk;
            break;
        case 0x8000:
            switch (n) {
                case 0x0000:
                    pc += 2;
                    V[x] = V[y];
                    break;
                case 0x0001:
                    pc += 2;
                    V[x] |= V[y];
                    break;
                case 0x0002:
                    pc += 2;
                    V[x] &= V[y];
                    break;
                case 0x0003:
                    pc += 2;
                    V[x] ^= V[y];
                    break;
                case 0x0004:
                    pc += 2;
                    V[0xF] = (V[x] + V[y]) > 0xFF;
                    V[x] += V[y];
                    break;
                case 0x0005:
                    pc += 2;
                    V[0xF] = V[x] > V[y];
                    V[x] -= V[y];
                    break;
                case 0x0006:
                    pc += 2;
                    V[0xF] = V[x] & 1;
                    V[x] >>= 1;
                    break;
                case 0x0007:
                    pc += 2;
                    V[0xF] = V[y] > V[x];
                    V[x] = V[y] - V[x];
                    break;
                case 0x000E:
                    pc += 2;
                    V[0xF] = V[x] >> 7;
                    V[x] <<= 1;
                    V[x] = V[y] << 1;
                    break;
                default:
                    std::cerr << "Unmapped opcode: " << opcode << "\n";
            }
            break;
        case 0x9000:
            pc += 2;
            if (V[x] != V[y]) {
                pc += 2;
            }
            break;
        case 0xA000:
            pc += 2;
            I = opcode & 0xFFF;
            break;
        case 0xB000:
            pc = (opcode & 0xFFF) + V[0];
            break;
        case 0xC000:
            pc += 2;
            {
                V[x] = std::uniform_int_distribution<>(0, 255)(gen) & kk;
            }
            break;
        case 0xD000:
            pc += 2;
            draw_flag = true;
            V[0xF] = 0;
            std::uint8_t pixel_row;
            for (int y_line = 0; y_line < n; ++y_line) {
                pixel_row = memory[I + y_line];
                for (int x_line = 0; x_line < 8; ++x_line) {
                    if (pixel_row & (0b10000000 >> x_line)) {
                        std::uint16_t coord = (V[x] + x_line + ((V[y] + y_line) * 64)) % 2048;
                        bool collision = (graphics[coord] == 1);
                        V[0xF] |= collision;
                        graphics[coord] ^= 1;
                    }
                }
            }
            break;
        case 0xE000:
            switch (kk) {
                case 0x009E:
                    pc += 2;
                    if (keys[V[x]]) {
                        pc += 2;
                    }
                    break;
                case 0x00A1:
                    pc += 2;
                    if (!keys[V[x]]) {
                        pc += 2;
                    }
                    break;
                default:
                    std::cerr << "Unmapped opcode: " << opcode << "\n";
            }
            break;
        case 0xF000:
            switch (kk) {
                case 0x0007:
                    pc += 2;
                    V[x] = delay_timer;
                    break;
                case 0x000A:
                {
                    bool waiting = true;
                    for (int i = 0; i < keys.size(); ++i) {
                        if (keys[i] != 0) {
                            V[x] = i;
                            waiting = false;
                            break;
                        }
                    }
                    if (waiting) {
                        return;
                    }
                    pc += 2;
                    break;
                }
                case 0x0015:
                    pc += 2;
                    delay_timer = V[x];
                    break;
                case 0x0018:
                    pc += 2;
                    sound_timer = V[x];
                    break;
                case 0x001E:
                    pc += 2;
                    V[0xF] = (I + V[x]) > 0xFFF;
                    I += V[x];
                    break;
                case 0x0029:
                    pc += 2;
                    I = V[x] * 5;
                    break;
                case 0x0033:
                    pc += 2;
                    memory[I] = V[x] / 100;
                    memory[I + 1] = (V[x] / 10) % 10;
                    memory[I + 2] = V[x] % 10;
                    break;
                case 0x0055:
                    pc += 2;
                    for (int i = 0; i <= x; i++) {
                        memory[I + i] = V[i];
                    }
                    break;
                case 0x0065:
                    pc += 2;
                    for (int i = 0; i <= x; i++) {
                        V[i] = memory[I + i];
                    }
                    break;
                default:
                    std::cerr << "Unmapped opcode: " << opcode << "\n";
            }
            break;
        default:
            std::cerr << "Unmapped opcode: " << opcode << "\n";
    }
}

void Chip8::step_timers() {
    if (delay_timer > 0) {
        delay_timer--;
    }
    if (sound_timer > 0) {
        sound_timer--;
    }
}

void Chip8::press_key(int keycode) {
    keys[keycode] = 1;
}

void Chip8::release_key(int keycode) {
    keys[keycode] = 0;
}

void Chip8::reset_draw_flag() {
    draw_flag = false;
}

std::uint8_t Chip8::get_pixel_data(int i) {
    return graphics[i];
}

bool Chip8::get_draw_flag() {
    return draw_flag;
}

std::uint8_t Chip8::get_sound_timer() {
    return sound_timer;
}