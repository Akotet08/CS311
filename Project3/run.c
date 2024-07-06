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
/* TODO: Implement 5-stage pipeplined MIPS simulator           */
/*                                                             */
/***************************************************************/

static CPU_State temp_state;
static int executed_instrs = 0;

void fetch_stage(){
	instruction * inst;
	
	if(FETCH_BIT == FALSE){
		return;
	}
	// for debugging, print fetch PC
	// printf("FETCH %x \n", CURRENT_STATE.PC);
	inst = get_inst_info(CURRENT_STATE.PC);
	temp_state.IF_ID_INST = inst;
	temp_state.IF_ID_NPC = CURRENT_STATE.PC + BYTES_PER_WORD;
}

void decode_stage(){
	instruction * inst;
	uint32_t opcode;
	uint32_t rs, rt, rd, shamt, funct, target;
	int32_t imm;

	if(temp_state.PIPE[ID_STAGE] == 0){
		return;
	}

	inst = CURRENT_STATE.IF_ID_INST;
	opcode = OPCODE(inst);
	rs = RS(inst);
	rt = RT(inst);
	rd = RD(inst);
	shamt = SHAMT(inst);
	funct = FUNC(inst);
	imm = (int32_t) IMM(inst); // sign-extend
	target =  ((CURRENT_STATE.IF_ID_NPC) & 0xf0000000) | (TARGET(inst) << 2); // 26-bit address

	temp_state.ID_EX_NPC = CURRENT_STATE.IF_ID_NPC;
	temp_state.ID_EX_REG1 = CURRENT_STATE.REGS[rs];
	temp_state.ID_EX_REG2 = CURRENT_STATE.REGS[rt];
	temp_state.ID_EX_IMM = imm;
	temp_state.ID_EX_RT = rt;
	temp_state.ID_EX_RS = rs;
	temp_state.ID_EX_Shamt = shamt;

	// load-use hazard detection
	if (CURRENT_STATE.ID_EX_MEM_READ) {
		if (CURRENT_STATE.ID_EX_DEST == rs || CURRENT_STATE.ID_EX_DEST == rt) {

			temp_state.PIPE_STALL[IF_STAGE] = TRUE;
			temp_state.PIPE_STALL[ID_STAGE] = TRUE;
		}
	}

	//forwarding from MEM_WB because we cannot use the first half cycle to write in this case (not like actual processor)
	if (FORWARDING_BIT) {
		if (CURRENT_STATE.MEM_WB_REG_WRITE && CURRENT_STATE.MEM_WB_DEST != 0) {
			if (CURRENT_STATE.MEM_WB_DEST == rs) {
				// select the value to forward from MEM_out or ALU_out
				if (CURRENT_STATE.MEM_WB_MEM_TO_REG) {
					temp_state.ID_EX_REG1 = CURRENT_STATE.MEM_WB_MEM_OUT;
				}
				else {
					temp_state.ID_EX_REG1 = CURRENT_STATE.MEM_WB_ALU_OUT;
				}
			
				temp_state.MEM_WB_FORWARD_REG = 1; 
			}

			if (CURRENT_STATE.MEM_WB_DEST == rt) {
				// select the value to forward from MEM_out or ALU_out
				if (CURRENT_STATE.MEM_WB_MEM_TO_REG) {
					temp_state.ID_EX_REG2 = CURRENT_STATE.MEM_WB_MEM_OUT;
				}
				else {
					temp_state.ID_EX_REG2 = CURRENT_STATE.MEM_WB_ALU_OUT;
				}
			
				temp_state.MEM_WB_FORWARD_REG = 1; 
			}
		}
	}

	// control signals and destination register,  by default initialized to 0 so no need to set to 0
	switch (opcode){
		case 0x0: // R-type,
			temp_state.ID_EX_DEST = rd;
			temp_state.ID_EX_ALU_OP = funct;
			temp_state.ID_EX_REG_WRITE = 1;

			// special case for JR
			if (funct == 0x08) {  // Correct handling of JR
				temp_state.JUMP_PC = CURRENT_STATE.REGS[rs];
				temp_state.IF_PC = 0;
				temp_state.BRANCH_PC = 0;

				temp_state.ID_EX_REG_WRITE = 0;
				temp_state.PIPE_STALL[IF_STAGE] = TRUE;
			}
			break;
		
		case 0x8:		//(0x001000)ADDI 
			temp_state.ID_EX_DEST = rt;
			temp_state.ID_EX_REG_WRITE = 1;
			temp_state.ID_EX_ALU_SRC = 1;

			// alu operation is add
			temp_state.ID_EX_ALU_OP = 0x20;
			break;
		case 0xc:		//(0x001100)ANDI
			temp_state.ID_EX_DEST = rt;
			temp_state.ID_EX_REG_WRITE = 1;
			temp_state.ID_EX_ALU_SRC = 1;

			// alu operation is and
			temp_state.ID_EX_ALU_OP = 0x24;
			break;
		case 0xf:		//(0x001111)LUI	
			temp_state.ID_EX_DEST = rt;
			temp_state.ID_EX_REG_WRITE = 1;
			temp_state.ID_EX_ALU_SRC = 1;

			// alu operation is shift left by 16 and fill with 0, so new funct code = 127
			temp_state.ID_EX_ALU_OP = 0x7f;
			break;
		case 0xd:		//(0x001101)ORI
			temp_state.ID_EX_DEST = rt;
			temp_state.ID_EX_REG_WRITE = 1;
			temp_state.ID_EX_ALU_SRC = 1;

			// alu operation is or
			temp_state.ID_EX_ALU_OP = 0x25;
			break;
		case 0xa:	//(0x001010)SLTI
			temp_state.ID_EX_DEST = rt;
			temp_state.ID_EX_REG_WRITE = 1;
			temp_state.ID_EX_ALU_SRC = 1;

			// alu operation is less than comparison, with new funct code = 126
			temp_state.ID_EX_ALU_OP = 0x7e;
			break;
		
		case 0x23:		//(0x100011)LW
			temp_state.ID_EX_DEST = rt;
			temp_state.ID_EX_REG_WRITE = 1;
			temp_state.ID_EX_MEM_READ = 1;
			temp_state.ID_EX_MEM_TO_REG = 1;
			temp_state.ID_EX_ALU_SRC = 1;

			// alu operation is add
			temp_state.ID_EX_ALU_OP = 0x20;
			break;
		
		case 0x2b:		//(0x101011)SW
			temp_state.ID_EX_MEM_WRITE = 1;
			temp_state.ID_EX_ALU_SRC = 1;

			// alu operation is add
			temp_state.ID_EX_ALU_OP = 0x20;
			break;
		case 0x4:		//(0x000100)BEQ
			temp_state.ID_EX_BRANCH = 1;

			// alu operation is equality check, with new funct code = 125
			temp_state.ID_EX_ALU_OP = 0x7d;
			break;
		case 0x5:		//(0x000101)BNE
			temp_state.ID_EX_BRANCH = 1;

			// alu operation is inequality check, with new funct code = 124
			temp_state.ID_EX_ALU_OP = 0x7c;
			break;
		
		case 0x2:		//(0x000010)J
			temp_state.JUMP_PC = target;
			temp_state.IF_PC = 0;
			temp_state.BRANCH_PC = 0;
			
			temp_state.PIPE_STALL[IF_STAGE] = TRUE;
			break;
		case 0x3:		//(0x000011)JAL
			CURRENT_STATE.REGS[31] = CURRENT_STATE.IF_ID_NPC;
			temp_state.JUMP_PC = target;
			temp_state.IF_PC = 0;
			temp_state.BRANCH_PC = 0;

			temp_state.PIPE_STALL[IF_STAGE] = TRUE;
			break;
		
		default:
			// printf("Unknown instruction type: %d\n", opcode);
			break;
	}	

	// If BR_BIT is set, flush IF_stage -> compute new PC
	uint32_t branch_target = CURRENT_STATE.IF_ID_NPC + (imm << 2);
	if (BR_BIT && temp_state.ID_EX_BRANCH){
		temp_state.IF_PC = 0;
		temp_state.JUMP_PC = 0;
		temp_state.BRANCH_PC = branch_target;

		temp_state.PIPE_STALL[IF_STAGE] = TRUE;
	}
}


void execute_stage(){
	uint32_t alu_out;
	uint32_t branch_target;
	uint32_t branch_take;
	uint32_t op1, op2;

	if(temp_state.PIPE[EX_STAGE] == FALSE){
		return;
	}

	// forwarding
	if (FORWARDING_BIT) {
		if (CURRENT_STATE.EX_MEM_REG_WRITE && CURRENT_STATE.EX_MEM_DEST != 0) {
			if (CURRENT_STATE.EX_MEM_DEST == CURRENT_STATE.ID_EX_RS) {
				CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.EX_MEM_ALU_OUT;
				temp_state.EX_MEM_FORWARD_REG = 1;
			}
			if (CURRENT_STATE.EX_MEM_DEST == CURRENT_STATE.ID_EX_RT) {
				CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.EX_MEM_ALU_OUT;
				temp_state.EX_MEM_FORWARD_REG = 1;
			}
		}

		if (CURRENT_STATE.MEM_WB_REG_WRITE && CURRENT_STATE.MEM_WB_DEST != 0) {
			if ((CURRENT_STATE.MEM_WB_DEST == CURRENT_STATE.ID_EX_RS) && (CURRENT_STATE.EX_MEM_DEST != CURRENT_STATE.ID_EX_RS)) {
				CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.MEM_WB_MEM_OUT;
				temp_state.MEM_WB_FORWARD_REG = 1;
			}
			if ((CURRENT_STATE.MEM_WB_DEST == CURRENT_STATE.ID_EX_RT) && (CURRENT_STATE.EX_MEM_DEST != CURRENT_STATE.ID_EX_RT)) {
				CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.MEM_WB_MEM_OUT;
				temp_state.MEM_WB_FORWARD_REG = 1;
			}
		}
	}

	// choose between register or immediate value
	op1 = CURRENT_STATE.ID_EX_REG1;
	op2 = (CURRENT_STATE.ID_EX_ALU_SRC) ? CURRENT_STATE.ID_EX_IMM : CURRENT_STATE.ID_EX_REG2;
	// printf("op1: %x, op2: %x\n", CURRENT_STATE.ID_EX_IMM, CURRENT_STATE.ID_EX_REG2);

	// ALU operation
	switch (CURRENT_STATE.ID_EX_ALU_OP){
		case 0x20:	// ADD
			alu_out = op1 + op2;
			break;
		case 0x22:	// SUB
			alu_out = op1 - op2;
			break;
		case 0x24:	// AND
			alu_out = op1 & op2;
			if(CURRENT_STATE.ID_EX_ALU_SRC){
				alu_out = op1 & (0xffff & op2); // zero-extend
			}
			break;
		case 0x27:	// NOR
			alu_out = ~(op1 | op2);
			break;
		case 0x25:	// OR
			alu_out = op1 | op2;
			if(CURRENT_STATE.ID_EX_ALU_SRC){
				alu_out = op1 | (0xffff & op2); // zero-extend
			}
			break;
		case 0x0:	// SLL
			alu_out = op2 << CURRENT_STATE.ID_EX_Shamt;
			break;
		case 0x2:	// SRL
			alu_out = op2 >> CURRENT_STATE.ID_EX_Shamt;
			break;
		case 0x2A:	// SLT
			alu_out = ((int32_t) op1 < (int32_t) op2) ? 1 : 0;
			break;
		case 0x7f:	// LUI
			alu_out = (op2 << 16) & 0xffff0000;
			break;
		case 0x7e:	// SLTI
			alu_out = ((int32_t) op1 < (int32_t) op2) ? 1 : 0;
			break;
		case 0x7d:	// BEQ
			alu_out = (op1 == op2) ? 1 : 0;
			break;
		case 0x7c:	// BNE
			alu_out = (op1 != op2) ? 1 : 0;
			break;
		case 0x7a:	// JR
			alu_out = CURRENT_STATE.ID_EX_REG1;
			break;
		default:
			// printf("Unknown ALU operation: %d\n", CURRENT_STATE.ID_EX_ALU_OP);
			break;
	}

	// branch target
	branch_target = CURRENT_STATE.ID_EX_NPC + (CURRENT_STATE.ID_EX_IMM << 2);
	branch_take = (alu_out == 1) ? 1 : 0;

	temp_state.EX_MEM_NPC = CURRENT_STATE.ID_EX_NPC;
	temp_state.EX_MEM_ALU_OUT = alu_out;
	temp_state.EX_MEM_W_VALUE = CURRENT_STATE.ID_EX_REG2;
	temp_state.EX_MEM_BR_TARGET = branch_target;
	temp_state.EX_MEM_BR_TAKE = branch_take;
	temp_state.EX_MEM_DEST = CURRENT_STATE.ID_EX_DEST;
	temp_state.EX_MEM_MEM_READ = CURRENT_STATE.ID_EX_MEM_READ;
	temp_state.EX_MEM_MEM_WRITE = CURRENT_STATE.ID_EX_MEM_WRITE;
	temp_state.EX_MEM_MEM_TO_REG = CURRENT_STATE.ID_EX_MEM_TO_REG;
	temp_state.EX_MEM_REG_WRITE = CURRENT_STATE.ID_EX_REG_WRITE;
	temp_state.EX_MEM_BRANCH = CURRENT_STATE.ID_EX_BRANCH;
}

void memory_stage(){
	uint32_t mem_out;

	if(temp_state.PIPE[MEM_STAGE] == FALSE){
		return;
	}
	// forwarding from EX_MEM
	if (FORWARDING_BIT) {
		if (CURRENT_STATE.MEM_WB_REG_WRITE && CURRENT_STATE.MEM_WB_DEST != 0) {
			if (CURRENT_STATE.MEM_WB_DEST == CURRENT_STATE.EX_MEM_DEST) {
				CURRENT_STATE.EX_MEM_W_VALUE = CURRENT_STATE.MEM_WB_MEM_OUT;
				temp_state.MEM_WB_FORWARD_REG = 1;
			}
		}
	}

	// branch checking
	if(CURRENT_STATE.EX_MEM_BRANCH){
		// check if prediction is correct
		if(BR_BIT != CURRENT_STATE.EX_MEM_BR_TAKE){
			temp_state.PIPE_FLUSH[IF_STAGE] = TRUE;
			temp_state.PIPE_FLUSH[ID_STAGE] = TRUE;
			temp_state.PIPE_FLUSH[EX_STAGE] = TRUE;

			// flip the branch flag
			BR_BIT = !BR_BIT;

			if (CURRENT_STATE.EX_MEM_BR_TAKE){
				temp_state.BRANCH_PC = CURRENT_STATE.EX_MEM_BR_TARGET;
				temp_state.IF_PC = 0;
				temp_state.JUMP_PC = 0;
			}
			else{
				temp_state.BRANCH_PC = CURRENT_STATE.EX_MEM_NPC;
				temp_state.IF_PC = 0;
				temp_state.JUMP_PC = 0;
			}
		}		
	}
	// memory operation
	if(CURRENT_STATE.EX_MEM_MEM_READ){
		mem_out = mem_read_32(CURRENT_STATE.EX_MEM_ALU_OUT);
	}
	else if(CURRENT_STATE.EX_MEM_MEM_WRITE){
		mem_write_32(CURRENT_STATE.EX_MEM_ALU_OUT, CURRENT_STATE.EX_MEM_W_VALUE);
		mem_out = 0;
	}
	else{
		mem_out = CURRENT_STATE.EX_MEM_ALU_OUT;
	}

	temp_state.MEM_WB_NPC = CURRENT_STATE.EX_MEM_NPC;
	temp_state.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
	temp_state.MEM_WB_MEM_OUT = mem_out;
	temp_state.MEM_WB_BR_TAKE = CURRENT_STATE.EX_MEM_BR_TAKE;
	temp_state.MEM_WB_DEST = CURRENT_STATE.EX_MEM_DEST;
	temp_state.MEM_WB_MEM_TO_REG = CURRENT_STATE.EX_MEM_MEM_TO_REG;
	temp_state.MEM_WB_REG_WRITE = CURRENT_STATE.EX_MEM_REG_WRITE;
}

void write_back_stage(){
	uint32_t mem_out;

	if(temp_state.PIPE[WB_STAGE] == FALSE){
		return;
	}

	// write back operation
	if(CURRENT_STATE.MEM_WB_MEM_TO_REG){
		CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_MEM_OUT;
	}
	else if(CURRENT_STATE.MEM_WB_REG_WRITE){
		CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_ALU_OUT;
	}	

	executed_instrs++;
}

void cpy_to_current_state(){
	CURRENT_STATE.PC = temp_state.PC;
	CURRENT_STATE.IF_ID_INST = temp_state.IF_ID_INST;
	CURRENT_STATE.IF_ID_NPC = temp_state.IF_ID_NPC;

	CURRENT_STATE.ID_EX_NPC = temp_state.ID_EX_NPC;
	CURRENT_STATE.ID_EX_REG1 = temp_state.ID_EX_REG1;
	CURRENT_STATE.ID_EX_REG2 = temp_state.ID_EX_REG2;
	CURRENT_STATE.ID_EX_IMM = temp_state.ID_EX_IMM;
	CURRENT_STATE.ID_EX_DEST = temp_state.ID_EX_DEST;
	CURRENT_STATE.ID_EX_RT = temp_state.ID_EX_RT;
	CURRENT_STATE.ID_EX_RS = temp_state.ID_EX_RS;
	CURRENT_STATE.ID_EX_Shamt = temp_state.ID_EX_Shamt;
	CURRENT_STATE.ID_EX_ALU_SRC = temp_state.ID_EX_ALU_SRC;
	CURRENT_STATE.ID_EX_ALU_OP = temp_state.ID_EX_ALU_OP;
	CURRENT_STATE.ID_EX_MEM_READ = temp_state.ID_EX_MEM_READ;
	CURRENT_STATE.ID_EX_MEM_WRITE = temp_state.ID_EX_MEM_WRITE;
	CURRENT_STATE.ID_EX_MEM_TO_REG = temp_state.ID_EX_MEM_TO_REG;
	CURRENT_STATE.ID_EX_REG_WRITE = temp_state.ID_EX_REG_WRITE;
	CURRENT_STATE.ID_EX_BRANCH = temp_state.ID_EX_BRANCH;

	CURRENT_STATE.EX_MEM_NPC = temp_state.EX_MEM_NPC;
	CURRENT_STATE.EX_MEM_ALU_OUT = temp_state.EX_MEM_ALU_OUT;
	CURRENT_STATE.EX_MEM_W_VALUE = temp_state.EX_MEM_W_VALUE;
	CURRENT_STATE.EX_MEM_BR_TARGET = temp_state.EX_MEM_BR_TARGET;
	CURRENT_STATE.EX_MEM_BR_TAKE = temp_state.EX_MEM_BR_TAKE;
	CURRENT_STATE.EX_MEM_DEST = temp_state.EX_MEM_DEST;
	CURRENT_STATE.EX_MEM_MEM_READ = temp_state.EX_MEM_MEM_READ;
	CURRENT_STATE.EX_MEM_MEM_WRITE = temp_state.EX_MEM_MEM_WRITE;
	CURRENT_STATE.EX_MEM_MEM_TO_REG = temp_state.EX_MEM_MEM_TO_REG;
	CURRENT_STATE.EX_MEM_REG_WRITE = temp_state.EX_MEM_REG_WRITE;
	CURRENT_STATE.EX_MEM_BRANCH = temp_state.EX_MEM_BRANCH;

	CURRENT_STATE.MEM_WB_NPC = temp_state.MEM_WB_NPC;
	CURRENT_STATE.MEM_WB_ALU_OUT = temp_state.MEM_WB_ALU_OUT;
	CURRENT_STATE.MEM_WB_MEM_OUT = temp_state.MEM_WB_MEM_OUT;
	CURRENT_STATE.MEM_WB_BR_TAKE = temp_state.MEM_WB_BR_TAKE;
	CURRENT_STATE.MEM_WB_DEST = temp_state.MEM_WB_DEST;
	CURRENT_STATE.MEM_WB_MEM_TO_REG = temp_state.MEM_WB_MEM_TO_REG;
	CURRENT_STATE.MEM_WB_REG_WRITE = temp_state.MEM_WB_REG_WRITE;

	for (int i = 0; i < PIPE_STAGE; i++){
		CURRENT_STATE.PIPE[i] = temp_state.PIPE[i];
		CURRENT_STATE.PIPE_STALL[i] = temp_state.PIPE_STALL[i];
		CURRENT_STATE.PIPE_FLUSH[i] = temp_state.PIPE_FLUSH[i];
	}

	CURRENT_STATE.IF_PC = temp_state.IF_PC;
	CURRENT_STATE.JUMP_PC = temp_state.JUMP_PC;
	CURRENT_STATE.BRANCH_PC = temp_state.BRANCH_PC;

	CURRENT_STATE.EX_MEM_FORWARD_REG = temp_state.EX_MEM_FORWARD_REG;
	CURRENT_STATE.MEM_WB_FORWARD_REG = temp_state.MEM_WB_FORWARD_REG;
	CURRENT_STATE.EX_MEM_FORWARD_VALUE = temp_state.EX_MEM_FORWARD_VALUE;
	CURRENT_STATE.MEM_WB_FORWARD_VALUE = temp_state.MEM_WB_FORWARD_VALUE;
}

void process_instruction(){
	CPU_State *state_ptr = (CPU_State *) calloc(1, sizeof(CPU_State)); // calloc initializaes everything to zero
	temp_state = *state_ptr;
	
	// handle stall
	if(CURRENT_STATE.PIPE_STALL[IF_STAGE]){
		CURRENT_STATE.PIPE[IF_STAGE] = 0;
	}
	// setting PC for pipline stages
	for(int st = 0; st < PIPE_STAGE - 1; st++){
		temp_state.PIPE[st + 1] = CURRENT_STATE.PIPE[st];

		if(st == ID_STAGE && CURRENT_STATE.PIPE_STALL[ID_STAGE]){
			temp_state.PIPE[st] = CURRENT_STATE.PIPE[st];
			temp_state.PIPE[st + 1] = 0;
		}
	}
	temp_state.PIPE[0] = CURRENT_STATE.PC;
	temp_state.PC = CURRENT_STATE.PC;
	// printf("PC: %x\n", CURRENT_STATE.PC);

	fetch_stage();
	decode_stage();
	execute_stage();
	memory_stage();
	write_back_stage();

	// for debugging, print pipeline registers
	// printf("IF: %x, ID: %x, EX: %x, MEM: %x, WB: %x\n", temp_state.PIPE[0], temp_state.PIPE[1], temp_state.PIPE[2], temp_state.PIPE[3], temp_state.PIPE[4]);
	// printf("IF: %x, ID: %x, EX: %x, MEM: %x, WB: %x\n", CURRENT_STATE.PIPE[0], CURRENT_STATE.PIPE[1], CURRENT_STATE.PIPE[2], CURRENT_STATE.PIPE[3], CURRENT_STATE.PIPE[4]);

	// clean temp state considering pipeline stalls and flushes
	for(int st = 0; st < PIPE_STAGE; st++){
		if(temp_state.PIPE_FLUSH[st]){
			temp_state.PIPE[st] = 0;

			// set reg_write and mem_write to 0 for the flushed instruction
			if(st == ID_STAGE){
				temp_state.ID_EX_REG_WRITE = 0;
				temp_state.ID_EX_MEM_WRITE = 0;
			}
			else if(st == EX_STAGE){
				temp_state.EX_MEM_REG_WRITE = 0;
				temp_state.EX_MEM_MEM_WRITE = 0;
			}
			else if(st == MEM_STAGE){
				temp_state.MEM_WB_REG_WRITE = 0;
			}
		}
	}

	if(temp_state.JUMP_PC) temp_state.PC = temp_state.JUMP_PC;
	else if(temp_state.BRANCH_PC) temp_state.PC = temp_state.BRANCH_PC;
	else {
		temp_state.PC = CURRENT_STATE.PC + BYTES_PER_WORD;
	}

	// stall IF stage if needed
	if (temp_state.PIPE_STALL[IF_STAGE]){
		// check if it is a branch predicition stall
		if (temp_state.BRANCH_PC){
			temp_state.PC = temp_state.BRANCH_PC;
		}
		else if (temp_state.JUMP_PC){
			temp_state.PC = temp_state.JUMP_PC;
		}
		else{
			temp_state.PC = CURRENT_STATE.PC;
		}
	}

	if(temp_state.PIPE_STALL[ID_STAGE]){
		temp_state.IF_ID_INST = CURRENT_STATE.IF_ID_INST;
	}

	cpy_to_current_state();
	// Zero reg is hard-wired to zero!
	CURRENT_STATE.REGS[0] = 0;

	// check for halt condition and exit, MAX_INSTRUCTION_NUM is defined in util.h
	if(executed_instrs >= MAX_INSTRUCTION_NUM){
		RUN_BIT = FALSE;
	}

	// check if PC is out of bounds
    if (CURRENT_STATE.PC < MEM_REGIONS[0].start || CURRENT_STATE.PC > (MEM_REGIONS[0].start + (NUM_INST * 4))){
		
		// check if all pipline stages are empty
		int run_pipline = 0;
		for(int st = 0; st < PIPE_STAGE; st++){
			run_pipline |= CURRENT_STATE.PIPE[st];
		}

		if (run_pipline == 0){
			RUN_BIT = FALSE;
		}

		FETCH_BIT = FALSE;
		CURRENT_STATE.PIPE[IF_STAGE] = 0;
		CURRENT_STATE.PC = CURRENT_STATE.PC - BYTES_PER_WORD;
	}

	int run_pipline = 0;
	for(int st = 0; st < PIPE_STAGE - 1; st++){
		run_pipline |= CURRENT_STATE.PIPE[st];
	}

	if (run_pipline == 0){
		RUN_BIT = FALSE;
	}

	free(state_ptr);
}
