/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   parse.c                                                   */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "parse.h"

int text_size;
int data_size;

int read_x_bit(const char * buff, int start_idx, int bits){
	char binary[bits + 1];
	int j;
	for(j = 0; j < bits; j++){
		binary[j] = buff[start_idx + j];
	}
	binary[j] = '\0';

	return fromBinary(binary);
}

instruction parsing_instr(const char *buffer, const int index)
{
	
    instruction instr;
	/* Implement your function here */
	char opcode[7];
	unsigned char rs, rt, rd, shamt;
	short func_code, opcode_value, imm;
	uint32_t target;
	int i = 0;

	for(i; i < 6; i++){
		opcode[i] = buffer[i];
	}
	opcode[6] = '\0';

	opcode_value = fromBinary(opcode);
	
	// Three types of Instructions
	// opcode 0: R-type, opcode 0x2, 0x3: J-type, 
	if(opcode_value == 0){ //Rtype

		rs = read_x_bit(buffer, i, 5); i+=5;
		rt = read_x_bit(buffer, i, 5); i+=5;
		rd = read_x_bit(buffer, i, 5); i+=5;
		shamt = read_x_bit(buffer, i, 5); i+=5;
		func_code = read_x_bit(buffer, i, 6); i+=6;
		
		instr.opcode = opcode_value;
		instr.func_code = func_code;
		instr.r_t.r_i.rs = rs;
		instr.r_t.r_i.rt = rt;
		instr.r_t.r_i.r_i.r.rd = rd;
		instr.r_t.r_i.r_i.r.shamt = shamt;	
	} 
	else if(opcode_value == 0x2 || opcode_value == 0x3) { // J or JAL (jump type)

		target = read_x_bit(buffer, 6, 26);

		instr.opcode = opcode_value;
		instr.r_t.target = target;

	}
	else { // I type
		rs = read_x_bit(buffer, i, 5); i+=5;
		rt = read_x_bit(buffer, i, 5); i+=5;
		imm = read_x_bit(buffer, i, 16); i+=16;

		instr.opcode = opcode_value;
		instr.r_t.r_i.rs = rs;
		instr.r_t.r_i.rt = rt;
		instr.r_t.r_i.r_i.imm = imm;
	}

	instr.value = MEM_TEXT_START + index;
	mem_write_32(MEM_TEXT_START + index, fromBinary((char *) buffer));

    return instr;
}

void parsing_data(const char *buffer, const int index)
{
    /* Implement your function here */
	uint32_t value = read_x_bit(buffer, 0, 32);
	uint32_t address = MEM_DATA_START + index;

	mem_write_32(address, value);
}

void print_parse_result()
{
    int i;
    printf("Instruction Information\n");

    for(i = 0; i < text_size/4; i++)
    {
	printf("INST_INFO[%d].value : %x\n",i, INST_INFO[i].value);
	printf("INST_INFO[%d].opcode : %d\n",i, INST_INFO[i].opcode);

	switch(INST_INFO[i].opcode)
	{
	    //Type I
	    case 0x8:		// ADDI
	    case 0xc:		// ANDI
	    case 0xf:		// LUI	
	    case 0xd:		// ORI
	    case 0xa:		// SLTI
	    case 0x23:		// LW	
	    case 0x2b:		// SW
	    case 0x4:		// BEQ
	    case 0x5:		// BNE
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].imm : %d\n",i, INST_INFO[i].r_t.r_i.r_i.imm);
		break;

    	    //TYPE R
	    case 0x0:		// ADD, AND, NOR, OR, SLT, SLL, SRL, SUB
		printf("INST_INFO[%d].func_code : %d\n",i, INST_INFO[i].func_code);
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].rd : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.rd);
		printf("INST_INFO[%d].shamt : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.shamt);
		break;

    	    //TYPE J
	    case 0x2:		// J
	    case 0x3:		// JAL
		printf("INST_INFO[%d].target : %d\n",i, INST_INFO[i].r_t.target);
		break;

	    default:
		printf("Not available instruction\n");
		assert(0);
	}
    }

    printf("Memory Dump - Text Segment\n");
    for(i = 0; i < text_size; i+=4)
	printf("text_seg[%d] : %x\n", i, mem_read_32(MEM_TEXT_START + i));
    for(i = 0; i < data_size; i+=4)
	printf("data_seg[%d] : %x\n", i, mem_read_32(MEM_DATA_START + i));
    printf("Current PC: %x\n", CURRENT_STATE.PC);
}
