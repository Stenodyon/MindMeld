/*
 *  This is an interpreter for MindMeld, a BrainFuck derivative that uses two data pointers
 *  Syntax:
 *  Pointer movement:
 *      >A  Move data pointer A forward
 *      >B  Move data pointer B forward
 *      <A  Move data pointer A backward
 *      <A  Move data pointer B backward
 *  Data modification:
 *      +A  Increment the byte that A points at by 1
 *      +B  Increment the byte that B points at by 1
 *      -A  Decrement the byte that A points at by 1
 *      -B  Decrement the byte that B points at by 1
 *  Console IO:
 *      .A  Output the byte that A points at to the console as an ASCII encoded char
 *      .B  Output the byte that B points at to the console as an ASCII encoded char
 *      ,A  Get a char from the console, and set the byte that A points at to the char's ASCII value
 *      ,B  Get a char from the console, and set the byte that B points at to the char's ASCII value
 *  Control Flow:
 *      [A  If the byte that A points to equals 0b00000000,
 *              move the instruction pointer forward to the matching close bracket (Either ]A or ]B is acceptable)
 *              Otherwise, move the instruction pointer to the next instruction
 *      [B  If the byte that B points to equals 0b00000000,
 *              move the instruction pointer forward to the matching close bracket (Either ]A or ]B is acceptable)
 *              Otherwise, move the instruction pointer forward to the next instruction
 *      ]A  If the byte that A points to does NOT equal 0b00000000,
 *              move the instruction pointer backward to the matching open bracket (Either [A or [B is acceptable)
 *              Otherwise, move the instruction pointer forward to the next instruction
 *      ]B  If the byte that A points to does NOT equal 0b00000000,
 *              move the instruction pointer backward to the matching open bracket (Either [A or [B is acceptable)
 *              Otherwise, move the instruction pointer forward to the next instruction
 */
#include <cassert>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <stack>
#include <utility>
#ifdef _WIN32
#include <conio.h>
#endif
#ifdef __unix
#include "getch.hpp"
#endif

enum class InstrType // Represents a BrainF**k instruction
{
    PLUS,
    MINUS,
    LEFT,
    RIGHT,
    INPUT,
    OUTPUT,
    LOOP_OPEN,
    LOOP_CLOSE
};

enum class Ptr { A, B }; // Designates which pointer to affect

// Represents an (Instruction, Pointer) pair, with a special case for loops
struct Instr
{
    InstrType type;
    Ptr ptr;
    uint64_t jump = 0;
};

//Pre-processing functions
std::string source_read(const std::string & filename);
std::string source_sanitize(const std::string & source);
std::string switch_sanitize(const std::string & source);

void tokenize(const std::string & source, std::vector<Instr> & out);

//Execution function
void execute(const char *instructions);
void execute_tokens(const std::vector<Instr> & instructions);

bool SWITCH = false;
bool TOKENS = false;

int main(int argc, char * argv[])
{
    std::string path;
    for(int arg_pos = 1; arg_pos < argc; arg_pos++)
    {
        if(std::string(argv[arg_pos]) == "--switch")
            SWITCH = true;
        else if(std::string(argv[arg_pos]) == "--tokens")
            TOKENS = true;
        else
            path = std::string(argv[arg_pos]);
    }
    if(argc == 0)
    {
        std::cout << "Enter a path to a MindMeld source file: ";
        getline(std::cin, path);
    }
    std::string raw_source(source_read(path));
    std::string instructions;
    if(SWITCH)
        instructions = std::string(switch_sanitize(raw_source));
    else
        instructions = std::string(source_sanitize(raw_source));
    std::cout << instructions << std::endl;
    if(TOKENS)
    {
        std::vector<Instr> tokens;
        tokenize(instructions, tokens);
        execute_tokens(tokens);
    }
    else
    {
        execute(instructions.c_str());
    }
    std::cout << std::endl << "Press any key to continue..." << std::endl;
    getch();
    return 0;
}

//Reads the given file, sanitizes it, and returns a cstring of instruction characters
std::string source_read(const std::string & filename)
{
    std::fstream source(filename, source.in);
    if(source){
        //Get the number of chars in the file
        source.seekg (0, source.end);
        uint64_t length = source.tellg();
        source.seekg (0, source.beg);

        //Read the file into a char array
        char *contents = new char[length+1];
        source.read(contents, length);
        contents[strlen(contents)]='\0';
        return std::string(contents);
    } else {
        std::cerr << "Could not read " << filename << std::endl;
        std::cout << "Press any key to continue..." << std::endl;
        getch();
        exit(-1);
    }
}

//Remove invalid characters from the raw source
std::string source_sanitize(const std::string & source)
{
    std::ostringstream stream;
    bool lastWasAB = false;
    for(const char c : source)
    {
        switch(c){
            case 'A':
            case 'B': // FALLTHROUGH
                if(lastWasAB)
                    continue;
                lastWasAB = true;
                stream << c;
                break;

            case '<':
            case '>': // FALLTHROUGH
            case '-': // FALLTHROUGH
            case '+': // FALLTHROUGH
            case '.': // FALLTHROUGH
            case ',': // FALLTHROUGH
            case '[': // FALLTHROUGH
            case ']': // FALLTHROUGH
                lastWasAB = false;
                stream << c;
                break;
        }
    }
    return stream.str();
}

//Remove invalid characters from the raw source
std::string switch_sanitize(const std::string & source)
{
    std::ostringstream stream;
    char ptr_specifier = 'A';
    for(const char c : source)
    {
        switch(c){
            case 'A':
            case 'B': // FALLTHROUGH
                ptr_specifier = c;
                break;

            case '<':
            case '>': // FALLTHROUGH
            case '-': // FALLTHROUGH
            case '+': // FALLTHROUGH
            case '.': // FALLTHROUGH
            case ',': // FALLTHROUGH
            case '[': // FALLTHROUGH
            case ']': // FALLTHROUGH
                stream << c << ptr_specifier;
                break;
        }
    }
    return stream.str();
}

typedef std::stack<uint64_t> loop_stack;

void tokenize(const std::string & source, std::vector<Instr> & out)
{
    loop_stack jump_stack;
    assert(source.length() % 2 == 0); // source length MUST be even
    for(uint64_t char_pos = 0; char_pos < source.length(); char_pos += 2)
    {
        Instr ins;
        switch(source[char_pos])
        {
            case '<':
                ins.type = InstrType::LEFT;
                break;
            case '>':
                ins.type = InstrType::RIGHT;
                break;
            case '-':
                ins.type = InstrType::MINUS;
                break;
            case '+':
                ins.type = InstrType::PLUS;
                break;
            case '.':
                ins.type = InstrType::OUTPUT;
                break;
            case ',':
                ins.type = InstrType::INPUT;
                break;
            case '[':
                ins.type = InstrType::LOOP_OPEN;
                break;
            case ']':
                ins.type = InstrType::LOOP_CLOSE;
                break;
        }
        switch(source[char_pos + 1])
        {
            case 'A':
                ins.ptr = Ptr::A;
                break;
            case 'B':
                ins.ptr = Ptr::B;
                break;
        }
        out.push_back(ins);
        if(ins.type == InstrType::LOOP_OPEN)
        {
            jump_stack.push(char_pos / 2);
        }
        if(ins.type == InstrType::LOOP_CLOSE)
        {
            uint64_t pos = jump_stack.top();
            jump_stack.pop();
            uint64_t jump_distance = char_pos / 2 - pos;
            out.back().jump = jump_distance;
            out.at(pos).jump = jump_distance;
        }
    }
}

//Interpret and execute the MM code.
void execute(const char *instructions)
{
    const char *instruction_pointer;         //Keeps track of the interpreter's position in the program.
    std::unique_ptr<uint8_t> data_tape;                //Stores the data used by the program. Basically, it's RAM.
    uint8_t *data_pointer_A;           //Used to modify/read cells on the data_tape. Controllable.
    uint8_t *data_pointer_B;           //Used to modify/read cells on the data_tape. Controllable.


    instruction_pointer = instructions;
    data_tape = std::unique_ptr<uint8_t>(new uint8_t[30000]()); //BrainFuck interpreters conventionally have a 30000 byte memory block.
    memset(data_tape.get(), 0, 30000); // Clearing the entire tape, just in case
    data_pointer_A = data_tape.get();
    data_pointer_B = data_tape.get();


    while(*instruction_pointer) //Until the instruction pointer reaches the end of the instructions
    {
        uint8_t **data_ptr; //Holds the address of the pointer specified by the command.
        switch(*(instruction_pointer + 1))
        {
            case 'A':
                data_ptr = &data_pointer_A;
                break;
            case 'B':
                data_ptr = &data_pointer_B;
                break;
            default:
                assert(false); // Should never happen
        }

        //Execute the appropriate instruction, using the appropriate data_pointer.
        switch(*instruction_pointer)
        {
            case '+':
                (**data_ptr)++;
                break;
            case '-':
                (**data_ptr)--;
                break;
            case '>':
                (*data_ptr)++;
                break;
            case '<':
                (*data_ptr)--;
                break;
            case '.':
                std::cout << (char)+(**data_ptr);
                break;
            case ',':
                **data_ptr = getch();
                std::cout << (char)+(**data_ptr);
                break;
            case '[':
                if(**data_ptr == 0){       //If the data_ptr is zero, skip to the matching close bracket
                    int level = 1;
                    while(level){
                        instruction_pointer+=2;
                        if (*instruction_pointer == '[') {
                            level++;
                        } else
                        if (*instruction_pointer == ']') {
                            level--;
                        }
                    }
                }                           //If the data_ptr isn't zero, let the interpreter enter the loop.
                break;
            case ']':
                if(**data_ptr != 0){       //If the data_ptr is not zero, jump back to the matching open bracket
                    int level = 1;
                    while(level){
                        instruction_pointer-=2;
                        if (*instruction_pointer == '[') {
                            level--;
                        } else
                        if (*instruction_pointer == ']') {
                            level++;
                        }
                    }
                    instruction_pointer-=2;
                }                           //If the data_ptr isn't zero, let the interpreter exit the loop.
                break;
            default:
                assert(false); // Should never happen
        }
        instruction_pointer+=2;         //Move to the next instruction
    }
}

//Interpret and execute the MM code.
void execute_tokens(const std::vector<Instr> & instructions)
{
    std::unique_ptr<uint8_t> data_tape;                //Stores the data used by the program. Basically, it's RAM.
    uint8_t *data_pointer_A;           //Used to modify/read cells on the data_tape. Controllable.
    uint8_t *data_pointer_B;           //Used to modify/read cells on the data_tape. Controllable.


    data_tape = std::unique_ptr<uint8_t>(new uint8_t[30000]()); //BrainFuck interpreters conventionally have a 30000 byte memory block.
    memset(data_tape.get(), 0, 30000); // Clearing the entire tape, just in case
    data_pointer_A = data_tape.get();
    data_pointer_B = data_tape.get();


    for(auto instruction_pointer = instructions.begin();
            instruction_pointer != instructions.end();
            instruction_pointer++)
    {
        const Instr * instr_ptr = instructions.data(); // debug
        uint8_t **data_ptr; //Holds the address of the pointer specified by the command.
        switch(instruction_pointer->ptr)
        {
            case Ptr::A:
                data_ptr = &data_pointer_A;
                break;
            case Ptr::B:
                data_ptr = &data_pointer_B;
                break;
            default:
                assert(false); // Should never happen
        }

        //Execute the appropriate instruction, using the appropriate data_pointer.
        switch(instruction_pointer->type)
        {
            case InstrType::PLUS:
                (**data_ptr)++;
                break;
            case InstrType::MINUS:
                (**data_ptr)--;
                break;
            case InstrType::RIGHT:
                (*data_ptr)++;
                break;
            case InstrType::LEFT:
                (*data_ptr)--;
                break;
            case InstrType::OUTPUT:
                std::cout << (char)+(**data_ptr);
                break;
            case InstrType::INPUT:
                **data_ptr = getch();
                std::cout << (char)+(**data_ptr);
                break;
            case InstrType::LOOP_OPEN:
                if(**data_ptr == 0)       //If the data_ptr is zero, skip to the matching close bracket
                {
                    assert(instruction_pointer->jump != 0);
                    instruction_pointer = std::next(instruction_pointer, instruction_pointer->jump);
                }
                break;
            case InstrType::LOOP_CLOSE:
                if(**data_ptr != 0)       //If the data_ptr is not zero, jump back to the matching open bracket
                {
                    assert(instruction_pointer->jump != 0);
                    instruction_pointer = std::prev(instruction_pointer, instruction_pointer->jump);
                }
                break;
            default:
                assert(false); // Should never happen
        }
    }
}

