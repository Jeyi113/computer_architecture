# computer_architecture

This main has 4 projects that simulates MIPS.

The goal of project 1 is build the MIPS ISA assembler that translates MIPS(Big endian) assembly code into the binary code but except linking. The program is excuted with the command "$ ./runfile <assembly file>" on the linux console. It receives assembly file (*.s) as input and then outputs the object file (*.o).

Project 2 is to establish simple MIPS emulator that is able to execute MIPS instructions. It loads the binary from object file (*.o) and executes the instruction and modifies the states of data and registers. 
The execution command is "$./runfile [-m addr1:addr2] [-d] [-n num_instruction] <input file>". According to the options given on the command, the program prints out PC, register, memory info on the console.

Project 3 is to extand the emulator and simulates the 5 step pipelining process. The execution command form is "$ ./runfile <-atp or -antp> [-m addr1:addr2] [-d] [-p] [-n num_instr] <binary file>". It receives object file (*.o) as input.

Project 4 builds 2 level cache simulator (L1, L2). The input file format is trace file that records PEC benchmark's memory behaviour. Execution Command is "$ ./runfile <-c capacity> <-a associativity> <-b block_size> <-lru or -random> <trace file>. When the progress is done, the simulator prints out the recoreded results into .out file.

