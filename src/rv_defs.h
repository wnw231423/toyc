#ifndef RV_DEFS_H
#define RV_DEFS_H
#include <string>

class Position {
public:
    int type; // 0: Register, 1: Memory
    std::string reg_name;
    int mem_offset;
    std::string relative_reg;

    Position() = default;

    Position(const std::string& reg_name)
        : type(0), reg_name(reg_name), mem_offset(0) {}

    Position(int mem_offset)
        : type(1), reg_name(""), mem_offset(mem_offset), relative_reg("sp") {}

    Position(int mem_offset, const std::string& relative_reg)
        : type(1), reg_name(""), mem_offset(mem_offset), relative_reg(relative_reg) {}
};

std::string move(Position src, Position dest) {
    if (src.type == 0) {
        // reg -> reg
        if (dest.type == 0) {
            return "  mv " + dest.reg_name + ", " + src.reg_name + "\n"; // Register to Register
        }

        // reg -> mem
        if (dest.mem_offset < 2047 && dest.mem_offset >= -2048) {
            return "  sw " + src.reg_name + ", " + std::to_string(dest.mem_offset) + "(sp)\n"; // Register to Memory
        } else {
            return "  li t6, " + std::to_string(dest.mem_offset) + "\n" +
                   "  add t6, sp, t6\n" +
                   "  sw " + src.reg_name + ", 0(t6)\n"; // Register to Memory with offset
        }
    }

    // mem -> reg
    if (dest.type == 0) {
        if (src.mem_offset < 2047 && src.mem_offset >= -2048) {
            return "  lw " + dest.reg_name + ", " + std::to_string(src.mem_offset) + "(sp)\n"; // Memory to Register
        }

        return "  li t6, " + std::to_string(src.mem_offset) + "\n" +
               "  add t6, sp, t6\n" +
               "  lw " + dest.reg_name + ", 0(t6)\n"; // Memory with offset to Register
    }

    // mem -> mem
    Position temp_reg("t0");
    return move(src, temp_reg) + move(temp_reg, dest);
}

#endif //RV_DEFS_H
