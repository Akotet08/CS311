# Project 3. MIPS Pipelined Simulator
Skeleton developed by CMU,
modified for KAIST CS311 purpose by THKIM, BKKIM and SHJEON.

## Instructions
There are three files you may modify: `util.h`, `run.h`, and `run.c`.

### 1. util.h

We have setup the basic CPU\_State that is sufficient to implement the project.
However, you may decide to add more variables, and modify/remove any misleading variables.

### 2. run.h

You may add any additional functions that will be called by your implementation of `process_instruction()`.
In fact, we encourage you to split your implementation of `process_instruction()` into many other helping functions.
You may decide to have functions for each stages of the pipeline.
Function(s) to handle flushes (adding bubbles into the pipeline), etc.

### 3. run.c

**Implement** the following function:

    void process_instruction()

The `process_instruction()` function is used by the `cycle()` function to simulate a `cycle` of the pipelined simulator.
Each `cycle()` the pipeline will advance to the next instruction (if there are no stalls/hazards, etc.).
Your internal register, memory, and pipeline register state should be updated according to the instruction
that is being executed at each stage.

### Definition of `Cpu_state` Fields

**Modified `IF_ID_INST` to be of the instruction pointer type:**  
This change enables the use of existing field extraction macros defined in `run.h`.

- `instruction *IF_ID_INST;`

**Register Identifiers (RT and RS) for Forwarding in the Execution Stage:**  
These identifiers assist in forwarding data correctly within the pipeline.

- `unsigned char ID_EX_RT;`
- `unsigned char ID_EX_RS;`

**Shift Amount (`shamt`) for R-type Instructions:**
- `unsigned char ID_EX_Shamt;`

**Control Signals Generated at the Instruction Decode (ID) Stage:**  
These signals are consumed in subsequent stages (EXE, MEM, WB). Note that the RegDst and Jump signals are handled separately.

- `unsigned char ID_EX_ALU_SRC;` **Selects between the immediate value and the RT register.**
- `unsigned char ID_EX_ALU_OP;` **Specifies the appropriate ALU operation.**
- `unsigned char ID_EX_MEM_READ;` **Memory read control signal.**
- `unsigned char ID_EX_MEM_WRITE;` **Memory write control signal.**
- `unsigned char ID_EX_MEM_TO_REG;` **Selects between the ALU and memory output when writing to the destination register.**
- `unsigned char ID_EX_REG_WRITE;` **Indicates whether the destination register should be written to.**
- `unsigned char ID_EX_BRANCH;` **Indicates if the instruction is a branch.**

**Control Signals for the MEM and WB Stages, Passed from the EXE Stage:**  
These are definitions of control signals that continue to subsequent stages.

- `unsigned char EX_MEM_MEM_READ;`
- `unsigned char EX_MEM_MEM_WRITE;`
- `unsigned char EX_MEM_MEM_TO_REG;`
- `unsigned char EX_MEM_REG_WRITE;`
- `unsigned char EX_MEM_BRANCH;`

**Control Signals for the WB Stage, Passed from the MEM Stage:**  
These signals continue to the write-back stage.

- `unsigned char MEM_WB_MEM_TO_REG;`
- `unsigned char MEM_WB_REG_WRITE;`
