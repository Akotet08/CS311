/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc) 
{ 
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/

void process_instruction(){
	/* Implement your function here */
    if(CURRENT_STATE.PC - MEM_TEXT_START >= (NUM_INST << 2)) {
        RUN_BIT = FALSE;
        return;
    }
    
    instruction *cur_instr = get_inst_info(CURRENT_STATE.PC);
    unsigned char rs, rt, rd, shamt;
    uint32_t target;
    short imm, opcode, func_code;
    int test = 0;

    opcode = OPCODE(cur_instr);
    CURRENT_STATE.PC += 4;

    // printf("opcode: %d, instr: %x,\n", opcode, NUM_INST);
    // fflush(stdout);
    if (opcode == 0x0){ // R type instruction
        rs = RS(cur_instr);
        rt = RT(cur_instr);
        rd = RD(cur_instr);
        shamt = SHAMT(cur_instr);

        func_code = FUNC(cur_instr);
        switch(func_code){
            case 0x20: // add
                CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
                break;
            case 0x22: // sub
                CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
                break;
            case 0x24: // and
                CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
                break;
            case 0x25: // or
                CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
                break;
            case 0x27: // nor
                CURRENT_STATE.REGS[rd] = ~(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
                break;
            case 0x8: // JR
                CURRENT_STATE.PC = CURRENT_STATE.REGS[rs];
                break;
            case 0x00: // sll
                CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << shamt;
                break;
            case 0x02: // srl
                CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> shamt;
                break;
            case 0x2a: // slt
                CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt];
                break;
            default: // not a correct command
                CURRENT_STATE.PC -= 4;
                RUN_BIT = FALSE;
        }

    }
    else if(opcode == 0x2 || opcode == 0x3){ // J or JALtype
        target = ((CURRENT_STATE.PC & 0xF0000000) | (TARGET(cur_instr) << 2));

        if(opcode == 0x3) { //j and link 
            CURRENT_STATE.REGS[31] = CURRENT_STATE.PC;
        }
        CURRENT_STATE.PC = target;
    }
    else { // I type
        rs = RS(cur_instr);
        rt = RT(cur_instr);
        imm = IMM(cur_instr);

        switch(opcode){
            case 0x8:		// ADDI
                CURRENT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + (uint32_t)(SIGN_EX(imm));
                break;
            case 0xc:		// ANDI
                CURRENT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] & ((uint32_t)imm & 0x0000ffff);
                break;
            case 0xf:		// LUI
                CURRENT_STATE.REGS[rt] = ((uint32_t)imm) << 16;
                break;	
            case 0xd:		// ORI
                CURRENT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] | ((uint32_t)imm & 0x0000ffff);
                break;
            case 0xa:		// SLTI
                CURRENT_STATE.REGS[rt] = (int32_t)CURRENT_STATE.REGS[rs] < imm;
                break;
            case 0x23:		// LW	
                CURRENT_STATE.REGS[rt] = mem_read_32(CURRENT_STATE.REGS[rs] + SIGN_EX(imm));
                break;
            case 0x2b:		// SW
                mem_write_32(CURRENT_STATE.REGS[rs] + SIGN_EX(imm), CURRENT_STATE.REGS[rt]);
                break;
            case 0x4:		// BEQ
                BRANCH_INST(CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt], CURRENT_STATE.PC + (imm << 2), 0);
                break;
            case 0x5:	    // BNE
                BRANCH_INST(CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt], CURRENT_STATE.PC + (imm << 2), 0);
                break;
            default:
            // printf("Halted for opcode: %d\n", opcode);
            CURRENT_STATE.PC -= 4;
            RUN_BIT = FALSE;
        }

    }

}
