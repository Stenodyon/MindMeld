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
#include <sstream>
#ifdef _WIN32
#include <conio.h>
#endif
#ifdef __unix
#include "getch.hpp"
#endif

//Pre-processing functions
std::string source_read(const std::string & filename);
std::string source_sanitize(const std::string & source);

//Execution function
void execute(const char *instructions);

int main(int argc, char * argv[])
{
    std::string path;
    if(argc == 1)
    {
        std::cout << "Enter a path to a MindMeld source file: ";
        getline(std::cin, path);
    }
    else
    {
        path = std::string(argv[1]);
    }
    std::string raw_source(source_read(path));
    std::string instructions(source_sanitize(raw_source));
    std::cout << instructions << std::endl;
    execute(instructions.c_str());
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
        source_sanitize(contents);
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
                        instruction_pointer+=1;
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
                        instruction_pointer-=1;
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

        uint64_t n = (instruction_pointer - instructions) / 2; // Debug
    }
}
