/*****************************************************************
 * BUAA Fall 2021 Fundamentals of Computer Hardware
 * Project7 Assembler and Linker
 *****************************************************************
 * my_assembler_utils.c
 * Assembler Submission
 *****************************************************************/
#include "my_assembler_utils.h"
#include "assembler-src/assembler_utils.h"
#include "lib/translate_utils.h"

#include <string.h>
#include <stdlib.h>

/*
 * This function reads .data symbol from INPUT and add them to the SYMTBL
 * Note that in order to distinguish in the symbol table whether a symbol
 * comes from the .data segment or the .text segment, we append a % to the
 * symbol name when adding the .data segment symbol. Since only underscores and
 * letters will appear in legal symbols, distinguishing them by adding % will
 * not cause a conflict between the new symbol and the symbol in the assembly file.
 *
 * Return value:
 * Return the number of bytes in the .data segment.
 */
int read_data_segment(FILE *input, SymbolTable *symtbl) {
    char buf[ASSEMBLER_BUF_SIZE];   // 存储每一行的读入数据
    uint32_t input_line = 0;
    int is_valid = 1;
    int byte_offset = 0;
    fgets(buf, ASSEMBLER_BUF_SIZE, input);
    if(strcmp(strtok(buf, ASSEMBLER_IGNORE_CHARS),".data") != 0){   // 如果一开始读入不是.data，直接返回invalid -1
        return -1;
    }
    while(fgets(buf, ASSEMBLER_BUF_SIZE, input)){   // 继续循环读入每一行
        skip_comment(buf);
        input_line++;
        if(buf[0] == '\n'){     // 读到空行退出循环
            break;
        }
        char name[2000] = {'%','\0'};    // 暂定最大的名字为1000行
        char *token = strtok(buf, ASSEMBLER_IGNORE_CHARS);
        if(token[strlen(token)-1] == ':'){
            token[strlen(token)-1] = '\0';
            if(!is_valid_label(token)){         // 如果Label名称违法则读取下一行,且设定返回invalid -1
                is_valid = 0;
                raise_label_error(input_line, token);
                continue;
            }
        }
        else{
            is_valid = 0;
            raise_label_error(input_line, token);
            continue;
        }
        strcat(name, token);

        token = strtok(NULL, ASSEMBLER_IGNORE_CHARS);
        if(strcmp(token,".space") != 0) {     // 如果读取非.space则字段违法，且设定返回invalid -1
            is_valid = 0;
            continue;
        }

        long num = 0;
        token = strtok(NULL, ASSEMBLER_IGNORE_CHARS);
        if(translate_num(&num, token, 0, 100000000) == -1){     // 如果读取到非法数字则设定返回invalid -1
            is_valid = 0;
            continue;
        }
        if(add_to_table(symtbl, name, byte_offset) == 0){
                byte_offset += num;
        }
        
    }
    return is_valid ? byte_offset : -1;
}

/* Adds a new symbol and its address to the SymbolTable pointed to by TABLE.
 * ADDR is given as the byte offset from the first instruction. The SymbolTable
 * must be able to resize itself as more elements are added.
 *
 * Note that NAME may point to a temporary array, so it is not safe to simply
 * store the NAME pointer. You must store a copy of the given string.
 *
 * If ADDR is not word-aligned, you should call addr_alignment_incorrect() and
 * return -1. If the table's mode is SYMTBL_UNIQUE_NAME and NAME already exists
 * in the table, you should call name_already_exists() and return -1. If memory
 * allocation fails, you should call allocation_failed().
 *
 * Otherwise, you should store the symbol name and address and return 0.
 */
int add_to_table(SymbolTable *table, const char *name, uint32_t addr) {
    int ret = 0;
    int base_table_unit = sizeof(Symbol);
    if(addr % 4 != 0){              // 如果偏移量没有对齐，则分配失败
        ret = -1;
        addr_alignment_incorrect();
    }
    if(get_addr_for_symbol(table, name) != -1 && table->mode == SYMTBL_UNIQUE_NAME){    // 如果表中已有且为UNIQUE MODE则分配失败
        ret = -1;
        name_already_exists(name);
    }
    if(table->len == table->cap){
        table->cap = table->cap * SCALING_FACTOR;
        if(!(table->tbl = (Symbol*) realloc(table->tbl, table->cap * base_table_unit))){
            allocation_failed();
            return 1;
        }
        
    }
    char *tmp = create_copy_of_str(name);
    if(ret == 0){
        table->tbl[table->len].name = tmp;
        table->tbl[table->len].addr = addr;
        (table->len)++;
    }
    return ret;
}

/*
 * Convert lui instructions to machine code. Note that for the imm field of lui,
 * it may be an immediate number or a symbol and needs to be handled separately.
 * Output the instruction to the **OUTPUT**.(Consider using write_inst_hex()).
 * 
 * Arguments:
 * opcode:     op segment in MIPS machine code
 * args:       args[0] is the $rt register, and args[1] can be either an imm field or 
 *             a .data segment label. The other cases are illegal and need not be considered
 * num_args:   length of args array
 * addr:       Address offset of the current instruction in the file
 */
int write_lui(uint8_t opcode, FILE *output, char **args, size_t num_args, uint32_t addr, SymbolTable *reltbl) {
    int32_t instr = opcode << 10;
    long imm = 0;
    int rt = translate_reg(args[0]);
    instr += rt;
    instr <<= 16;
    // int is_num = (translate_num(&imm, args[1], 0, 100000000) != -1);    //如果返回-1则不是数字
    // if(!is_num){
    //     if(get_addr_for_symbol(reltbl, args[1]) == -1){     // 如果返回-1，则说明表中没有
    //         add_to_table(reltbl, args[1], addr);
    //     }
    // }
    if((imm = get_addr_for_symbol(reltbl, args[1])) == -1){         // 如果args[1]是数字则无法获取，进入下一步；如果是args[1]是label则从表中获取addr，如果读不到也进入下一步
        if(translate_num(&imm, args[1], 0, 100000000) == -1){       // 如果是数字读入，如果不是数字则加入表中
            char* name = strtok(args[1],"@");
            if(is_valid_label(name)){
                name[strlen(name)] = '@';
                add_to_table(reltbl,args[1],addr);  
            }
            else{
                raise_label_error(addr / 4,name);
            }
        }
    }
    instr += imm;
    write_inst_hex(output, instr);
    return 0;
}

