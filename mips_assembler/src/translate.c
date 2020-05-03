#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tables.h"
#include "translate_utils.h"
#include "translate.h"

/* Writes instructions during the assembler's first pass to OUTPUT. The case
   for general instructions has already been completed, but you need to write
   code to translate the li and blt pseudoinstructions. Your pseudoinstruction 
   expansions should not have any side effects.

   NAME is the name of the instruction, ARGS is an array of the arguments, and
   NUM_ARGS specifies the number of items in ARGS.

   Error checking for regular instructions are done in pass two. However, for
   pseudoinstructions, you must make sure that ARGS contains the correct number
   of arguments. You do NOT need to check whether the registers / label are 
   valid, since that will be checked in part two.

   Also for li:
    - make sure that the number is representable by 32 bits. (Hint: the number 
        can be both signed or unsigned).
    - if the immediate can fit in the imm field of an addiu instruction, then
        expand li into a single addiu instruction. Otherwise, expand it into 
        a lui-ori pair.

   And for blt:
    - your expansion should use the fewest number of instructions possible.

   MARS has slightly different translation rules for li, and it allows numbers
   larger than the largest 32 bit number to be loaded with li. You should follow
   the above rules if MARS behaves differently.

   Use fprintf() to write. If writing multiple instructions, make sure that 
   each instruction is on a different line.

   Returns the number of instructions written (so 0 if there were any errors).
 */
unsigned write_pass_one(FILE* output, const char* name, char** args, int num_args) {
    if (strcmp(name, "li") == 0) {
        /* YOUR CODE HERE */
        if (num_args == 2) {
            long int imm;
            /*  make sure that the immediate can be represented by 32 bits.
                NOTE: MARS makes the 32-bit immediate signed, but we assume signed or unsigned here.
            */
            int err = translate_num(&imm, args[1], INT32_MIN, UINT32_MAX);
            if (err >= 0) {
                if (imm <= UINT16_MAX && imm >= INT16_MIN) {
                    fprintf(output, "addiu ");
                    fprintf(output, "%s $0 %ld\n", args[0], imm);
                    return 1;
                } else {
                    long int imm_upper = imm >> 16;
                    long int imm_lower = imm & 0xFFFF;
                    fprintf(output, "lui ");
                    fprintf(output, "$at %ld\n", imm_upper);
                    fprintf(output, "ori ");
                    fprintf(output, "%s $at %ld\n", args[0], imm_lower);      
                    return 2;  
                }
            }
        }
        return 0;
    } else if (strcmp(name, "blt") == 0) {
        /* YOUR CODE HERE */
        /*  NOTE: we only need to implement the form like blt $rs, $rt, label   */
        if (num_args == 3) {
            fprintf(output, "slt ");
            fprintf(output, "$at %s %s\n", args[0], args[1]);
            fprintf(output, "bne ");
            fprintf(output, "$at $0 %s\n", args[2]);
            return 2;
        }
        return 0;
    } else {
        write_inst_string(output, name, args, num_args);
        return 1;
    }
}

/* Writes the instruction in hexadecimal format to OUTPUT during pass #2.
   
   NAME is the name of the instruction, ARGS is an array of the arguments, and
   NUM_ARGS specifies the number of items in ARGS. 

   The symbol table (SYMTBL) is given for any symbols that need to be resolved
   at this step. If a symbol should be relocated, it should be added to the
   relocation table (RELTBL), and the fields for that symbol should be set to
   all zeros. 

   You must perform error checking on all instructions and make sure that their
   arguments are valid. If an instruction is invalid, you should not write 
   anything to OUTPUT but simply return -1. MARS may be a useful resource for
   this step.

   Note the use of helper functions. Consider writing your own! If the function
   definition comes afterwards, you must declare it first (see translate.h).

   Returns 0 on success and -1 on error. 
 */
int translate_inst(FILE* output, const char* name, char** args, size_t num_args, uint32_t addr,
    SymbolTable* symtbl, SymbolTable* reltbl) {

    // printf("%d %s ", addr/4, name);
    // int k;
    // for (k = 0; k < num_args; k++) {
    //     printf("%s ", args[k]);
    // }
    // printf("\n");

    if (strcmp(name, "addu") == 0)       return write_rtype (0x21, output, args, num_args);
    else if (strcmp(name, "or") == 0)    return write_rtype (0x25, output, args, num_args);
    else if (strcmp(name, "slt") == 0)   return write_rtype (0x2a, output, args, num_args);
    else if (strcmp(name, "sltu") == 0)  return write_rtype (0x2b, output, args, num_args);
    else if (strcmp(name, "sll") == 0)   return write_shift (0x00, output, args, num_args);
    /* YOUR CODE HERE */
    /* Note: R-type has same opcode 0, opcode in I-type and J-type differs */
    else if (strcmp(name, "jr") == 0)    return write_jr (0x08, output, args, num_args);
    else if (strcmp(name, "addiu") == 0) return write_addiu (0x09, output, args, num_args);
    else if (strcmp(name, "ori") == 0)   return write_ori (0x0d, output, args, num_args);
    else if (strcmp(name, "lui") == 0)   return write_lui (0x0f, output, args, num_args);
    else if (strcmp(name, "lb") == 0)    return write_mem (0x20, output, args, num_args);
    else if (strcmp(name, "lbu") == 0)   return write_mem (0x24, output, args, num_args);
    else if (strcmp(name, "lw") == 0)    return write_mem (0x23, output, args, num_args);
    else if (strcmp(name, "sb") == 0)    return write_mem (0x28, output, args, num_args);
    else if (strcmp(name, "sw") == 0)    return write_mem (0x2b, output, args, num_args);
    else if (strcmp(name, "beq") == 0)   return write_branch (0x04, output, args, num_args, addr, symtbl);
    else if (strcmp(name, "bne") == 0)   return write_branch (0x05, output, args, num_args, addr, symtbl);
    else if (strcmp(name, "j") == 0)     return write_jump (0x02, output, args, num_args, addr, reltbl);
    else if (strcmp(name, "jal") == 0)   return write_jump (0x03, output, args, num_args, addr, reltbl);
    else                                 return -1;
}

/* A helper function for writing most R-type instructions. You should use
   translate_reg() to parse registers and write_inst_hex() to write to 
   OUTPUT. Both are defined in translate_utils.h.

   This function is INCOMPLETE. Complete the implementation below. You will
   find bitwise operations to be the cleanest way to complete this function.
 */
int write_rtype(uint8_t funct, FILE* output, char** args, size_t num_args) {
    // Perhaps perform some error checking?
    if (num_args == 3) {    // error checking #1: then number of arguments must be 3 
        int rd = translate_reg(args[0]);
        int rs = translate_reg(args[1]);
        int rt = translate_reg(args[2]);
        if (rd >= 0 && rs >= 0 && rt >= 0) {    // error checking #2: translate_reg must work right
            int opcode = 0b000000 << 26;
            rs = rs << 21;
            rt = rt << 16;
            rd = rd << 11;
            int shamt = 0b00000 << 6;
            uint32_t instruction = opcode | rs | rd | rt | shamt | funct;
            write_inst_hex(output, instruction);
            return 0;
        }
    }
    return -1;
}

/* A helper function for writing shift instructions. You should use 
   translate_num() to parse numerical arguments. translate_num() is defined
   in translate_utils.h.

   This function is INCOMPLETE. Complete the implementation below. You will
   find bitwise operations to be the cleanest way to complete this function.
 */
int write_shift(uint8_t funct, FILE* output, char** args, size_t num_args) {
	// Perhaps perform some error checking?
    if (num_args == 3) {    
        long int shamt; // output in translate_num is lont int type!
        int rd = translate_reg(args[0]);
        int rt = translate_reg(args[1]);
        int err = translate_num(&shamt, args[2], 0, 31); 
        if (rd >= 0 && rt >= 0 && err >= 0) {    
            int opcode = 0b000000 << 26;
            int rs = 0b00000 << 21;
            rt = rt << 16;
            rd = rd << 11;
            shamt = shamt << 6;
            uint32_t instruction = opcode | rs | rd | rt | shamt | funct;
            write_inst_hex(output, instruction);
            return 0;
        }
    }
   return -1;
}

int write_jr(uint8_t funct, FILE* output, char** args, size_t num_args) {
	 if (num_args == 1) {    
        int rs = translate_reg(args[0]);
        if (rs >= 0) {    
            int opcode = 0b000000 << 26;
            rs = rs << 21;
            int rt =  0b00000 << 16;
            int rd = 0b00000 << 11;
            int shamt = 0b00000 << 6;
            uint32_t instruction = opcode | rs | rd | rt | shamt | funct;
            write_inst_hex(output, instruction);
            return 0;
        }
    }
   return -1;
}

/* NOTE: addiu $s1, $s2, -100 -- set $s1 to ($s2 plus signed 16-bit immediate) without overflow */
int write_addiu(uint8_t opcode, FILE* output, char** args, size_t num_args) {
	 if (num_args == 3) {
        long int imm;    
        int rt = translate_reg(args[0]);
        int rs = translate_reg(args[1]);
        int err = translate_num(&imm, args[2], INT16_MIN, INT16_MAX); // INT16_MIN defined in stdint.h included in translate.h
        if (rt >= 0 && rs >= 0 && err >= 0) {    
            int op_code = opcode << 26;
            rs = rs << 21;
            rt = rt << 16;
            if (imm < 0) {  // important! imm's value between INT16_MIN and INT16_MAX but represented as long int!
                imm = imm & 0xFFFF;
            }
            uint32_t instruction = op_code | rs | rt | imm;
            write_inst_hex(output, instruction);
            return 0;
        }
    }
   return -1;
}

int write_ori(uint8_t opcode, FILE* output, char** args, size_t num_args) {
	 if (num_args == 3) {
        long int imm;    
        int rt = translate_reg(args[0]);
        int rs = translate_reg(args[1]);
        int err = translate_num(&imm, args[2], 0, UINT16_MAX);
        if (rt >= 0 && rs >= 0 && err >= 0) {    
            int op_code = opcode << 26;
            rs = rs << 21;
            rt = rt << 16;
            uint32_t instruction = op_code | rs | rt | imm;
            write_inst_hex(output, instruction);
            return 0;
        }
    }
   return -1;
}

int write_lui(uint8_t opcode, FILE* output, char** args, size_t num_args) {
	 if (num_args == 2) {
        long int imm;    
        int rt = translate_reg(args[0]);
        int err = translate_num(&imm, args[1], 0, UINT16_MAX); 
        if (rt >= 0 && err >= 0) {    
            int op_code = opcode << 26;
            int rs = 0b00000 << 21;
            rt = rt << 16;
            uint32_t instruction = op_code | rs | rt | imm;
            write_inst_hex(output, instruction);
            return 0;
        }
    }
   return -1;
}

int write_mem(uint8_t opcode, FILE* output, char** args, size_t num_args) {
	 if (num_args == 3) {
        long int imm;    
        int rt = translate_reg(args[0]);
        int rs = translate_reg(args[2]); // Attention!
        /* imm here is signed! */
        int err = translate_num(&imm, args[1], INT16_MIN, INT16_MAX); 
        if (rt >= 0 && rs >= 0 && err >= 0) {    
            int op_code = opcode << 26;
            rs = rs << 21;
            rt = rt << 16;
            if (imm < 0) {
                imm = imm & 0xFFFF;
            }
            uint32_t instruction = op_code | rs | rt | imm;
            write_inst_hex(output, instruction);
            return 0;
        }
    }
   return -1;
}

int write_branch(uint8_t opcode, FILE* output, char** args, size_t num_args, 
    uint32_t addr, SymbolTable* symtbl) {
	if (num_args == 3) {   
        int rs = translate_reg(args[0]);
        int rt = translate_reg(args[1]);
        int label_addr = get_addr_for_symbol(symtbl, args[2]); 
        // printf("%s %d\n", args[2], label_addr / 4);
        if (rs >= 0 && rt >= 0 && label_addr >= 0) {    
            int op_code = opcode << 26;
            rs = rs << 21;
            rt = rt << 16;
            int32_t offset = label_addr - addr - 4;
            if (offset % 4 == 0) { 
                offset = (offset / 4);
                /* NOTE: now offset is a 32-bit signed number, but we need a 16-bit one
                   Because offset is certainly between INT16_MIN and INT16_MAX (for pass #2),
                   we need just to cut the left 16-bit.
                */
                // printf("%d\n", offset);
                offset = offset & 0xFFFF;
                uint32_t instruction = op_code | rs | rt | offset;
                write_inst_hex(output, instruction);
                return 0;
            }
        }
    }
   return -1;
}

int write_jump(uint8_t opcode, FILE* output, char** args, size_t num_args, 
    uint32_t addr, SymbolTable* reltbl) {
	if (num_args == 1) {
        char *target_label = args[0];
        /*  We can use the same data structure of symbol table. j and jal instructions in out file 
            don't contain the details of target address. The linker will do it with the relocation
            table!
        */
        add_to_table(reltbl, target_label, addr);
        uint32_t instruction = opcode << 26;
        write_inst_hex(output, instruction);
        return 0;
    }
    return -1;
}
