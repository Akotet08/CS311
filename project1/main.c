#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void int_to_binary(int integer, char * target, int num_bits){
	if (num_bits > 32) num_bits = 32;

    for (int i = 0; i < num_bits; i++) {
        target[num_bits - 1 - i] = ((integer >> i) & 1) + '0';
    }
    target[num_bits] = '\0';
}

void process_register(char * reg, char * target){ // return a 5 bit representaion of the register number from input $n
	int r_number = 0;
	sscanf(reg, "$%d", &r_number);

	char bin[33];
	int_to_binary(r_number, bin, 32);
	strncpy(target, bin + 27, 5);
	target[5] = '\0';
}

int search(char label[], char * labels[]){
	int i = 0;
    while (labels[i] != NULL) { 
        if (strcmp(label, labels[i]) == 0) {
            return i; 
        }
        i++;
    }
    return -1; 
}

void parse_file(int firstpass, int * data_size, int * text_size, char * labels[], int addresses[], int datavalues[]){
	char line[128];
	char c;
	int c_data_label = 0;
	int c_text_label = 0;

	int labels_idx = 0;

	int in_text = 0;
	int PC = 0x400000;

	while(scanf("%s", line) == 1){
		// printf("Read line: %s \n", line);
		if(firstpass && !in_text && line[strlen(line) - 1] == ':') { // lable in the data section
			// printf("%s \n", line);
			line[strlen(line) - 1] = '\0';

			labels[labels_idx] = strdup(line);
			addresses[labels_idx] = 0x10000000 + (c_data_label * 4);

			labels_idx ++;
		}
		else if(strcmp(".data", line) == 0) {
			in_text = 0;
			continue;
		}
		else if(strcmp(".word", line) == 0){
			int word;

			scanf("%i", &word); // reads integer considering the 0x for hex

			if(firstpass) {
				datavalues[c_data_label] = word;
				c_data_label += 1; // keep count of the data under the label
			}

		}

		else if(strcmp(".text", line) == 0) {
			in_text = 1;
			continue;
		}
		else {
			char * cmd = line;

			if(firstpass) {
				if(strcmp("j", line) == 0 || strcmp("jr", line) == 0 || strcmp("jal", line) == 0){
					scanf("%s", line);
					c_text_label += 1;
				}
				else if(strcmp("la", line) == 0 || strcmp("lw", line) == 0 || strcmp("sw", line) == 0 || strcmp("lui", line) == 0){

					if(strcmp("la", line) == 0){
						char label[64];

						scanf("%s", line); 
						scanf("%s", label);
						
						int i = search(label, labels);

						if(i == -1){
							printf("label NOT FOUND");
						}

						int address = addresses[i];

						int lower = address & 0xFFFF;
						
						if(lower != 0){
							c_text_label += 1;
						}

						c_text_label += 1;
					}
					else {
						scanf("%s", line);
						scanf("%s", line);
						c_text_label += 1;
					}
										
				}

				else if(cmd[strlen(cmd) - 1] == ':'){
					cmd[strlen(line) - 1] = '\0';
					labels[labels_idx] = strdup(cmd);
					addresses[labels_idx] = 0x400000 + (c_text_label * 4);

					labels_idx ++;
				}
				else {
					scanf("%s", line);
					scanf("%s", line);
					scanf("%s", line);
					c_text_label += 1;
				}
			}
			
			else {
				char ins_encoding[33];
				ins_encoding[32] = '\0';			
				// printf("Read line: %s \n", line);
				if(strcmp("add", line) == 0){
					PC += 4;
					char reg1[4], reg2[4], reg3[4];
					char en1[6], en2[6], en3[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%s", reg3);
					process_register(reg1, en1); process_register(reg2, en2); process_register(reg3, en3);

					// op-code(000000) + reg2 + reg3 + reg1 + shamt(00000) + functon(100000)
					char * opcode = "000000";
					char * shamt = "00000";
					char * fun = "100000";

					sprintf(ins_encoding, "%s%s%s%s%s%s", opcode, en2, en3, en1, shamt, fun);
					// printf("add Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
				}
				else if(strcmp("addi", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					int immediate;
					char en1[6], en2[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%i", &immediate);
					process_register(reg1, en1); process_register(reg2, en2);

					// op-code(001000) + reg2 + reg1 + immediate 
					char * opcode = "001000";
					char immediate_16bit[17];
					int_to_binary(immediate, immediate_16bit, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, en2, en1, immediate_16bit);
					// printf("addi Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
				}
				else if(strcmp("and", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4], reg3[4];
					char en1[6], en2[6], en3[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%s", reg3);
					process_register(reg1, en1); process_register(reg2, en2); process_register(reg3, en3);

					// op-code(000000) + reg2 + reg3 + reg1 + shamt(00000) + functon(100100)
					char *opcode = "000000";
					char *shamt = "00000";
					char *fun = "100100";

					sprintf(ins_encoding, "%s%s%s%s%s%s", opcode, en2, en3, en1, shamt, fun);
					// printf("and Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
									
				}
				else if(strcmp("andi", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					int immediate;
					char en1[6], en2[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%i", &immediate);
					process_register(reg1, en1); process_register(reg2, en2);

					// op-code(001000) + reg2 + reg1 + immediate 
					char * opcode = "001100";
					char immediate_16bit[17];
					int_to_binary(immediate, immediate_16bit, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, en2, en1, immediate_16bit);
					// printf("andi Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("beq", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					char label[64];
					char en1[6], en2[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%s", label);
					process_register(reg1, en1); process_register(reg2, en2);
					
					int i = search(label, labels);

					if(i == -1){
						printf("label NOT FOUND");
					}
					int address = addresses[i];

					int pc_relative = (address - PC)/4;
					// op-code(001000) + reg2 + reg1 + address 
					char * opcode = "000100";
					char pc_address[17];
					int_to_binary(pc_relative, pc_address, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, en1, en2, pc_address);
					// printf("PC: %d, encoding: %s", pc_relative, ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("bne", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					char label[64];
					char en1[6], en2[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%s", label);
					process_register(reg1, en1); process_register(reg2, en2);
					
					int i = search(label, labels);

					if(i == -1){
						printf("label NOT FOUND");
					}
					int address = addresses[i];

					int pc_relative = (address - PC)/4;
					// op-code(001000) + reg2 + reg1 + address 
					char * opcode = "000101";
					char pc_address[17];
					int_to_binary(pc_relative, pc_address, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, en1, en2, pc_address);
					// printf("PC: %d, encoding: %s", pc_relative, ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("nor", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4], reg3[4];
					char en1[6], en2[6], en3[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%s", reg3);
					process_register(reg1, en1); process_register(reg2, en2); process_register(reg3, en3);

					// op-code(000000) + reg2 + reg3 + reg1 + shamt(00000) + functon(100100)
					char *opcode = "000000";
					char *shamt = "00000";
					char *fun = "100111";

					sprintf(ins_encoding, "%s%s%s%s%s%s", opcode, en2, en3, en1, shamt, fun);
					// printf("and Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
				}
				else if(strcmp("or", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4], reg3[4];
					char en1[6], en2[6], en3[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%s", reg3);
					process_register(reg1, en1); process_register(reg2, en2); process_register(reg3, en3);

					// op-code(000000) + reg2 + reg3 + reg1 + shamt(00000) + functon(100100)
					char *opcode = "000000";
					char *shamt = "00000";
					char *fun = "100101";

					sprintf(ins_encoding, "%s%s%s%s%s%s", opcode, en2, en3, en1, shamt, fun);
					// printf("and Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("ori", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					int immediate;
					char en1[6], en2[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%i", &immediate);
					process_register(reg1, en1); process_register(reg2, en2);

					// op-code(001000) + reg2 + reg1 + immediate 
					char * opcode = "001101";
					char immediate_16bit[17];
					int_to_binary(immediate, immediate_16bit, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, en2, en1, immediate_16bit);
					// printf("ori Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("slti", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					int immediate;
					char en1[6], en2[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%i", &immediate);
					process_register(reg1, en1); process_register(reg2, en2);

					// op-code(001000) + reg2 + reg1 + immediate 
					char * opcode = "001010";
					char immediate_16bit[17];
					int_to_binary(immediate, immediate_16bit, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, en2, en1, immediate_16bit);
					// printf("slti Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("slt", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4], reg3[4];
					char en1[6], en2[6], en3[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%s", reg3);
					process_register(reg1, en1); process_register(reg2, en2); process_register(reg3, en3);

					// op-code(000000) + reg2 + reg3 + reg1 + shamt(00000) + functon(100100)
					char *opcode = "000000";
					char *shamt = "00000";
					char *fun = "101010";

					sprintf(ins_encoding, "%s%s%s%s%s%s", opcode, en2, en3, en1, shamt, fun);
					// printf("slt Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("sll", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					int shift;
					char en1[6], en2[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%i", &shift);
					process_register(reg1, en1); process_register(reg2, en2);

					// op-code(000000) + reg2 + reg3 + reg1 + shamt(00000) + functon(100100)
					char *opcode = "000000";
					char shamnt[6];
					int_to_binary(shift, shamnt, 5);

					char *fun = "000000";

					sprintf(ins_encoding, "%s%s%s%s%s%s", opcode, "00000", en2, en1, shamnt, fun);
					// printf("sll Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);					
				}
				else if(strcmp("srl", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					int shift;
					char en1[6], en2[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%i", &shift);
					process_register(reg1, en1); process_register(reg2, en2);

					// op-code(000000) + reg2 + reg3 + reg1 + shamt(00000) + functon(100100)
					char *opcode = "000000";
					char shamnt[6];
					int_to_binary(shift, shamnt, 5);
					
					char *fun = "000010";

					sprintf(ins_encoding, "%s%s%s%s%s%s", opcode, "00000", en2, en1, shamnt, fun);
					// printf("srl Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("sub", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4], reg3[4];
					char en1[6], en2[6], en3[6];

					scanf("%s", reg1); scanf("%s", reg2); scanf("%s", reg3);
					process_register(reg1, en1); process_register(reg2, en2); process_register(reg3, en3);

					// op-code(000000) + reg2 + reg3 + reg1 + shamt(00000) + functon(100000)
					char * opcode = "000000";
					char * shamt = "00000";
					char * fun = "100010";

					sprintf(ins_encoding, "%s%s%s%s%s%s", opcode, en2, en3, en1, shamt, fun);
					// printf("sub Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);

				}
				else if(strcmp("jr", cmd) == 0){
					PC += 4;
					char reg1[4];
					char en1[6];

					scanf("%s", reg1);
					process_register(reg1, en1);

					// op-code(000000) + reg2 + reg3 + reg1 + shamt(00000) + functon(100000)
					char * opcode = "000000";
					char * shamt = "00000";
					char * fun = "001000";

					sprintf(ins_encoding, "%s%s%s%s%s%s", opcode, en1, shamt, shamt, shamt, fun);
					// printf("jr Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("j", cmd) == 0){
					PC += 4;

					char label[64];

					scanf("%s", label);				

					int i = search(label, labels);

					if(i == -1){
						printf("label NOT FOUND");
					}

					int address = addresses[i];

					address >>= 2;
					address = address & 0x03FFFFFF;

					// op-code(001000) + reg2 + reg1 + address 
					char * opcode = "000010";
					char jr_address[27];
					int_to_binary(address, jr_address, 26);

					sprintf(ins_encoding, "%s%s", opcode, jr_address);
					// printf("j encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("jal", cmd) == 0){
					PC += 4;

					char label[64];

					scanf("%s", label);				

					int i = search(label, labels);

					if(i == -1){
						printf("label NOT FOUND");
					}

					int address = addresses[i];

					address >>= 2;
					address = address & 0x03FFFFFF;

					// op-code(001000) + reg2 + reg1 + address 
					char * opcode = "000011";
					char jr_address[27];
					int_to_binary(address, jr_address, 26);

					sprintf(ins_encoding, "%s%s", opcode, jr_address);
					// printf("j encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("lui", cmd) == 0){
					PC += 4;

					char reg1[4];
					int immediate;
					char en1[6];

					scanf("%s", reg1); scanf("%i", &immediate);
					process_register(reg1, en1);
					
					// op-code(001000) + '00000' + reg1 + immediate 
					char * opcode = "001111";
					char immediate_16bit[17];
					int_to_binary(immediate, immediate_16bit, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, "00000", en1, immediate_16bit);
					// printf("lui Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("la", cmd) == 0){
					PC += 4;

					char reg1[4];
					char label[64];
					char en1[6];

					scanf("%s", reg1); 
					scanf("%s", label);

					process_register(reg1, en1);
					
					int i = search(label, labels);

					if(i == -1){
						printf("label NOT FOUND");
					}

					int address = addresses[i];

					int lower = address & 0xFFFF;
					int upper = (address >> 16) & 0xFFFF;

					char * opcode = "001111";
					char immediate_16bit[17];
					int_to_binary(upper, immediate_16bit, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, "00000", en1, immediate_16bit);
					// printf("lui Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);


					if(lower != 0){
						PC += 4;

						char * opcode = "001101";
						char immediate_16bit[17];
						int_to_binary(lower, immediate_16bit, 16);

						sprintf(ins_encoding, "%s%s%s%s", opcode, en1, en1, immediate_16bit);
						// printf("ori Encoding: %s", ins_encoding);
						printf("%s", ins_encoding);
						
					}
					
				}
				else if(strcmp("lw", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					int immediate;
					char en1[6], en2[6];

					char immediate_plus_reg[12];

					scanf("%s", reg1);					
					scanf("%s", immediate_plus_reg);

					sscanf(immediate_plus_reg, "%d(%s)", &immediate, reg2);

					process_register(reg1, en1); process_register(reg2, en2);
					
					char * opcode = "100011";
					char immediate_16bit[17];
					int_to_binary(immediate, immediate_16bit, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, en2, en1, immediate_16bit);
					// printf("sw Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
					
				}
				else if(strcmp("sw", cmd) == 0){
					PC += 4;

					char reg1[4], reg2[4];
					int immediate;
					char en1[6], en2[6];

					char immediate_plus_reg[12];

					scanf("%s", reg1);					
					scanf("%s", immediate_plus_reg);

					sscanf(immediate_plus_reg, "%d(%s)", &immediate, reg2);

					process_register(reg1, en1); process_register(reg2, en2);
					
					char * opcode = "101011";
					char immediate_16bit[17];
					int_to_binary(immediate, immediate_16bit, 16);

					sprintf(ins_encoding, "%s%s%s%s", opcode, en2, en1, immediate_16bit);
					// printf("sw Encoding: %s", ins_encoding);
					printf("%s", ins_encoding);
				}
				else {  
					
				}

			}
		}
	}
	
	if(firstpass){
		*data_size = 4 * c_data_label;
		*text_size = 4 * c_text_label;

		// printf("%d %d \n", c_data_label, c_text_label);

		labels[labels_idx] = NULL;
		addresses[labels_idx] = -1;
		datavalues[63] = c_data_label; 
	}
}

int main(int argc, char* argv[]){

	if(argc != 2){
		printf("Usage: ./runfile <assembly file>\n"); //Example) ./runfile /sample_input/example1.s
		printf("Example) ./runfile ./sample_input/example1.s\n");
		exit(0);
	}
	else
	{ 

		// To help you handle the file IO, the deafult code is provided.
		// If we use freopen, we don't need to use fscanf, fprint,..etc. 
		// You can just use scanf or printf function 
		// ** You don't need to modify this part **
		// If you are not famailiar with freopen,  you can see the following reference
		// http://www.cplusplus.com/reference/cstdio/freopen/

		//For input file read (sample_input/example*.s)

		char *file=(char *)malloc(strlen(argv[1])+3);
		strncpy(file,argv[1],strlen(argv[1]));

		if(freopen(file, "r",stdin)==0){
			printf("File open Error!\n");
			exit(1);
		}

		//From now on, if you want to read string from input file, you can just use scanf function.

		// First pass, compute sizes and get location information for jump instructions
		int data_size = 0;
		int text_size = 0;

		char *labels[128];
		int addresses[128];
		int datavalues[64];

		// first pass
		parse_file(1, &data_size, &text_size, labels, addresses, datavalues);

		file=(char *)malloc(strlen(argv[1])+3);
		strncpy(file,argv[1],strlen(argv[1]));

		char datasize[33];
		char textsize[33];

		int_to_binary(data_size, datasize, 32);
		int_to_binary(text_size, textsize, 32);

		if(freopen(file, "r",stdin)==0){
			printf("File open Error!\n");
			exit(1);
		}
		
		file[strlen(file)-1] ='o';
		freopen(file,"w",stdout);

		printf("%s", textsize);
		printf("%s", datasize);
		
		//second pass
		parse_file(0, &data_size, &text_size, labels, addresses, NULL);

		for(int i =0; i < datavalues[63]; i++){
			int val = datavalues[i];
			char bin_val[33];

			int_to_binary(val, bin_val, 32);
			printf("%s", bin_val);			
			
		}
		// For output file write 
		// You can see your code's output in the sample_input/example#.o 
		// So you can check what is the difference between your output and the answer directly if you see that file
		// make test command will compare your output with the answer
	}
	return 0;
}

