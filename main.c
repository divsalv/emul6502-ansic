#include <stdio.h>
#include <stdlib.h>
// 6502 CPU Registers
typedef struct {
    unsigned char a;  // Accumulator
    unsigned char x;  // Index Register X
    unsigned char y;  // Index Register Y
    unsigned short pc; // Program Counter
    unsigned char sp; // Stack Pointer
    unsigned char p;  // Processor Status Register
} CPU6502;
// Memory (64 KB)
#define MEMORY_SIZE (65536)
unsigned char memory[MEMORY_SIZE];
// Stack (Simplified)
#define STACK_SIZE 256
unsigned short stack[STACK_SIZE];
unsigned char stack_pointer = STACK_SIZE - 1; 
// Initialize the CPU
void cpu_init(CPU6502 *cpu) {
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->pc = 0;
    cpu->sp = 0xFF;  // Initialize Stack Pointer
    cpu->p = 0;
}
// Fetch a byte from memory
unsigned char fetch_byte(CPU6502 *cpu) {
    return memory[cpu->pc++];
}
// Addressing Modes (Simplified)
unsigned short get_address(CPU6502 *cpu, unsigned char mode) {
    unsigned short address = 0;
    switch (mode) {
        case 0: // Immediate
            address = cpu->pc;
            cpu->pc++;
            break;
        case 1: // Zero Page
            address = fetch_byte(cpu);
            break;
        case 2: // Absolute
            address = fetch_byte(cpu) | (fetch_byte(cpu) << 8);
            break;
        case 3: // Zero Page, X Indexed
            address = (fetch_byte(cpu) + cpu->x) & 0xFF;
            break;
        case 4: // Absolute, X Indexed
            address = (fetch_byte(cpu) | (fetch_byte(cpu) << 8)) + cpu->x;
            break;
        case 5: // Zero Page, Y Indexed
            address = (fetch_byte(cpu) + cpu->y) & 0xFF;
            break;
        case 6: // Absolute, Y Indexed
            address = (fetch_byte(cpu) | (fetch_byte(cpu) << 8)) + cpu->y;
            break;
        // ... (Add more addressing modes) ...
        default:
            printf("Invalid addressing mode: %d\n", mode);
            exit(1);
    }
    return address;
}
// Read a byte from memory (with addressing mode)
unsigned char read_byte(CPU6502 *cpu, unsigned char mode) {
    unsigned short address = get_address(cpu, mode);
    return memory[address];
}
// Write a byte to memory (with addressing mode)
void write_byte(CPU6502 *cpu, unsigned char mode, unsigned char value) {
    unsigned short address = get_address(cpu, mode);
    memory[address] = value;
}
// Push a value onto the stack
void push(unsigned short value) {
    if (stack_pointer == 0) {
        printf("Stack Overflow!\n");
        exit(1);
    }
    stack[stack_pointer--] = value;
}
// Pop a value from the stack
unsigned short pop() {
    if (stack_pointer == STACK_SIZE - 1) {
        printf("Stack Underflow!\n");
        exit(1);
    }
    return stack[++stack_pointer];
}
// Decode and execute 6502 instructions
void execute_instruction(CPU6502 *cpu) {
    unsigned char opcode = fetch_byte(cpu);
    switch (opcode) {
        case 0xA9: // LDA #$xx (Load Accumulator Immediate)
            cpu->a = fetch_byte(cpu);
            break;
        case 0x8D: // STA $xxxx (Store Accumulator)
            write_byte(cpu, 2, cpu->a); // Absolute addressing
            break;
        case 0x69: // ADC #$xx (Add with Carry)
            cpu->a += fetch_byte(cpu);
            // ... (Handle carry flag) ...
            break;
        case 0xAD: // LDA $xxxx (Load Accumulator)
            cpu->a = read_byte(cpu, 2); // Absolute addressing
            break;
        case 0xAE: // LDY $xxxx (Load Y Register)
            cpu->y = read_byte(cpu, 2); // Absolute addressing
            break;
        case 0xA0: // LDY #$xx (Load Y Register Immediate)
            cpu->y = fetch_byte(cpu);
            break;
        case 0xA2: // LDX #$xx (Load X Register Immediate)
            cpu->x = fetch_byte(cpu);
            break;
        case 0xA1: // LDA ($xx,X) (Load Accumulator, Indexed Indirect)
            unsigned char zero_page_address = fetch_byte(cpu);
            unsigned short address = ((memory[zero_page_address] | (memory[zero_page_address + 1] << 8)) + cpu->x) & 0xFFFF;
            cpu->a = memory[address];
            break;
        case 0xA6: // LDA $xx (Load Accumulator, Zero Page)
            cpu->a = read_byte(cpu, 1); // Zero page addressing
            break;
        case 0xE8: // INX (Increment X Register)
            cpu->x = (cpu->x + 1) & 0xFF;
            break;
        case 0xC8: // INY (Increment Y Register)
            cpu->y = (cpu->y + 1) & 0xFF;
            break;
        case 0xE6: // INC $xx (Increment Zero Page) 
            write_byte(cpu, 1, (read_byte(cpu, 1) + 1) & 0xFF); // Increment value in zero page
            break;
        case 0x9E: // STX $xxxx (Store X Register)
            write_byte(cpu, 2, cpu->x); // Absolute addressing
            break;
        case 0x9D: // STZ $xxxx (Store Zero)
            write_byte(cpu, 2, 0x00); // Absolute addressing
            break;
        case 0xAC: // LDY $xxxx (Load Y Register)
            cpu->y = read_byte(cpu, 2); // Absolute addressing
            break;
        case 0xC9: // CMP #$xx (Compare Immediate)
            cpu->p &= ~0x01;  // Clear the Zero flag
            if (cpu->a == fetch_byte(cpu)) {
                cpu->p |= 0x01; // Set the Zero flag if values are equal
            }
            break;
        case 0xD0: // BNE $xx (Branch if Not Equal)
            if (cpu->p & 0x01) { // Check the Zero flag (bit 0)
                cpu->pc += fetch_byte(cpu); // Relative branch 
            } else {
                cpu->pc++; // Increment PC for the next instruction
            }
            break;
        case 0xF0: // BEQ $xx (Branch if Equal)
            if (!(cpu->p & 0x01)) { // Check the Zero flag (bit 0)
                cpu->pc += fetch_byte(cpu); // Relative branch 
            } else {
                cpu->pc++; // Increment PC for the next instruction
            }
            break;
        case 0x4C: // JMP $xxxx (Jump)
            cpu->pc = fetch_byte(cpu) | (fetch_byte(cpu) << 8);
            break;
        case 0x20: // JSR $xxxx (Jump to Subroutine)
            push(cpu->pc + 2); // Push the return address
            cpu->pc = fetch_byte(cpu) | (fetch_byte(cpu) << 8);

            // Special handling for JSR $0025 (Call to 'putchar')
            if (cpu->pc == 0x0025) {
                // Call the C putchar function 
                putchar(cpu->a);
                cpu->pc = pop(); // Pop the return address from the stack
            }
            break;
        case 0x60: // RTS (Return from Subroutine)
            cpu->pc = pop(); // Pop the return address
            break;
        case 0x9A: // TXS (Transfer X to Stack Pointer)
            cpu->sp = cpu->x;
            break;
        case 0xBA: // TSX (Transfer Stack Pointer to X)
            cpu->x = cpu->sp;
            break;
        case 0xAA: // TAX (Transfer A to X)
            cpu->x = cpu->a;
            break;
        case 0x8A: // TXA (Transfer X to A)
            cpu->a = cpu->x;
            break;
        case 0xA8: // TAY (Transfer A to Y)
            cpu->y = cpu->a;
            break;
        case 0x98: // TYA (Transfer Y to A)
            cpu->a = cpu->y;
            break;
        case 0x90: // BCC $xx (Branch if Carry Clear)
            if (!(cpu->p & 0x02)) { // Check the Carry flag (bit 1)
                cpu->pc += fetch_byte(cpu); // Relative branch 
            } else {
                cpu->pc++; // Increment PC for the next instruction
            }
            break;
        case 0xB0: // BCS $xx (Branch if Carry Set)
            if (cpu->p & 0x02) { // Check the Carry flag (bit 1)
                cpu->pc += fetch_byte(cpu); // Relative branch 
            } else {
                cpu->pc++; // Increment PC for the next instruction
            }
            break;
        // ... (Add more 6502 opcodes) ...
        default:
            printf("Unrecognized opcode: 0x%02X\n", opcode);
            exit(1);
    }
}
// Simple memory dump function (for debugging)
void dump_memory(int start, int end) {
    printf("Memory Dump (0x%04X - 0x%04X)\n", start, end);
    for (int i = start; i <= end; i++) {
        if ((i % 16) == 0) {
            printf("%04X: ", i);
        }
        printf("%02X ", memory[i]);
        if ((i % 16) == 15) {
            printf("\n");
        }
    }
    printf("\n");
}
int main() {
    CPU6502 cpu;
    cpu_init(&cpu);
    /*// Load the program into memory
    // Print "A"
    memory[0x100] = 0xA9; // LDA #$41 ('A')
    memory[0x101] = 0x41;
    memory[0x102] = 0x20; // JSR $0025 (Jump to Subroutine)
    memory[0x103] = 0x25;
    memory[0x104] = 0x00;
    // Print "B"
    memory[0x105] = 0xA9; // LDA #$41 ('B')
    memory[0x106] = 0x42;
    memory[0x107] = 0x20; // JSR $0025 (Jump to Subroutine)
    memory[0x108] = 0x25;
    memory[0x109] = 0x00;
    // Print "C"
    memory[0x10A] = 0xA9; // LDA #$41 ('\n')
    memory[0x10B] = 0x0A;
    memory[0x10C] = 0x20; // JSR $0025 (Jump to Subroutine)
    memory[0x10D] = 0x25;
    memory[0x10E] = 0x00;*/
    
    
    // Load the program into memory
    memory[0x100] = 0xA9; // LDA #$41 ('A')
    memory[0x101] = 0x41;
    memory[0x102] = 0x20; // JSR $2000 (Jump to Subroutine)
    memory[0x103] = 0x00;
    memory[0x104] = 0x20; 
    memory[0x105] = 0x4C; // JMP $102 (Jump back to the start)
    memory[0x106] = 0x02;
    memory[0x107] = 0x01;
    memory[0x200] = 0x00; // Initial value for the counter
    // Subroutine to print a character
    memory[0x2000] = 0xA9; // LDA #$41 ('A')
    memory[0x2001] = 0x41;
    memory[0x2002] = 0x20; // JSR $0025 (Jump to Subroutine)
    memory[0x2003] = 0x25;
    memory[0x2004] = 0x00;
    memory[0x2005] = 0x60; // RTS (Return from Subroutine)
    // Print function (for demonstration, this calls putchar)
    memory[0x0020] = 0x98; // TYA 
    memory[0x0021] = 0x20; // JSR $0025 (Jump to putchar)
    memory[0x0022] = 0x25;
    memory[0x0023] = 0x00;
    memory[0x0024] = 0x60; // RTS
    
    // Set PC to start executing at 0x100
    cpu.pc = 0x100;
    // Emulator loop
    while (1) {
        execute_instruction(&cpu);
        //dump_memory(0x100, 0x10E); // Example: Dump memory from 0x100 to 0x104
        printf("A: 0x%02X, X: 0x%02X, Y: 0x%02X, PC: 0x%04X, SP: 0x%02X, P: 0x%02X\n",cpu.a, cpu.x, cpu.y, cpu.pc, cpu.sp, cpu.p);

        /*if (cpu.pc == 0x105) {
            break;
        }*/
    }
    return 0;
}