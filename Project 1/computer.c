#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */

unsigned int endianSwap(unsigned int);

void PrintInfo (int changedReg, int changedMem);
unsigned int Fetch (int);
void Decode (unsigned int, DecodedInstr*, RegVals*);
int Execute (DecodedInstr*, RegVals*);
int Mem(DecodedInstr*, int, int *);
void RegWrite(DecodedInstr*, int, int *);
void UpdatePC(DecodedInstr*, int);
void PrintInstruction (DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */
void InitComputer (FILE* filein, int printingRegisters, int printingMemory,
  int debugging, int interactive) {
    int k;
    unsigned int instr;

    /* Initialize registers and memory */

    for (k=0; k<32; k++) {
        mips.registers[k] = 0;
    }
    
    /* stack pointer - Initialize to highest address of data segment */
    mips.registers[29] = 0x00400000 + (MAXNUMINSTRS+MAXNUMDATA)*4;

    for (k=0; k<MAXNUMINSTRS+MAXNUMDATA; k++) {
        mips.memory[k] = 0;
    }

    k = 0;
    while (fread(&instr, 4, 1, filein)) {
	/*swap to big endian, convert to host byte order. Ignore this.*/
        mips.memory[k] = ntohl(endianSwap(instr));
        k++;
        if (k>MAXNUMINSTRS) {
            fprintf (stderr, "Program too big.\n");
            exit (1);
        }
    }

    mips.printingRegisters = printingRegisters;
    mips.printingMemory = printingMemory;
    mips.interactive = interactive;
    mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i) {
    return (i>>24)|(i>>8&0x0000ff00)|(i<<8&0x00ff0000)|(i<<24);
}

/*
 *  Run the simulation.
 */
void Simulate () {
    char s[40];  /* used for handling interactive input */
    unsigned int instr;
    int changedReg=-1, changedMem=-1, val;
    DecodedInstr d;
    
    /* Initialize the PC to the start of the code section */
    mips.pc = 0x00400000;
    while (1) {
        if (mips.interactive) {
            printf ("> ");
            fgets (s,sizeof(s),stdin);
            if (s[0] == 'q') {
                return;
            }
        }

        /* Fetch instr at mips.pc, returning it in instr */
        instr = Fetch (mips.pc);

        printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

        /* 
	 * Decode instr, putting decoded instr in d
	 * Note that we reuse the d struct for each instruction.
	 */
        Decode (instr, &d, &rVals);

        /*Print decoded instruction*/
        PrintInstruction(&d);

        /* 
	 * Perform computation needed to execute d, returning computed value 
	 * in val 
	 */
        val = Execute(&d, &rVals); // val will have return value of temp in execute();

	UpdatePC(&d,val);

        /* 
	 * Perform memory load or store. Place the
	 * address of any updated memory in *changedMem, 
	 * otherwise put -1 in *changedMem. 
	 * Return any memory value that is read, otherwise return -1.
         */
        val = Mem(&d, val, &changedMem);

        /* 
	 * Write back to register. If the instruction modified a register--
	 * (including jal, which modifies $ra) --
         * put the index of the modified register in *changedReg,
         * otherwise put -1 in *changedReg.
         */
        RegWrite(&d, val, &changedReg);

        PrintInfo (changedReg, changedMem);
    }
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */

void PrintInfo ( int changedReg, int changedMem) {
    int k, addr;
    printf ("New pc = %8.8x\n", mips.pc);
    if (!mips.printingRegisters && changedReg == -1) {
        printf ("No register was updated.\n");
    } else if (!mips.printingRegisters) {
        printf ("Updated r%2.2d to %8.8x\n",
        changedReg, mips.registers[changedReg]);
    } else {
        for (k=0; k<32; k++) {
            printf ("r%2.2d: %8.8x  ", k, mips.registers[k]);
            if ((k+1)%4 == 0) {
                printf ("\n");
            }
        }
    }
    if (!mips.printingMemory && changedMem == -1) {
        printf ("No memory location was updated.\n");
    } else if (!mips.printingMemory) {
        printf ("Updated memory at address %8.8x to %8.8x\n",
        changedMem, Fetch (changedMem));
    } else {
        printf ("Nonzero memory\n");
        printf ("ADDR	  CONTENTS\n");
        for (addr = 0x00400000+4*MAXNUMINSTRS;
             addr < 0x00400000+4*(MAXNUMINSTRS+MAXNUMDATA);
             addr = addr+4) {
            if (Fetch (addr) != 0) {
                printf ("%8.8x  %8.8x\n", addr, Fetch (addr));
            }
        }
    }
}

/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch. 
 */
unsigned int Fetch ( int addr) {
    return mips.memory[(addr-0x00400000)/4];
}

void r_decode(unsigned int instr, DecodedInstr* d, RegVals* rVals){
    int r_rs, r_rt, r_rd, r_shamt, r_funct;
    //get funct
    r_funct = instr & 0x3f;
    (*d).regs.r.funct = r_funct;
    //printf("R, funct is: %d\n", (*d).regs.r.funct);
    //get shamt
    instr = instr >> 6;
    r_shamt = instr & 0x1f;
    (*d).regs.r.shamt = r_shamt;
    //printf("R, shamt is: %d\n", (*d).regs.r.shamt);
    //get rd
    instr = instr >> 5;
    r_rd = instr & 0x1f;
    (*d).regs.r.rd = r_rd;
    //printf("R, rd is: %d\n", (*d).regs.r.rd);
    //get rt
    instr = instr >> 5;
    r_rt = instr & 0x1f;
    (*d).regs.r.rt = r_rt;
    //printf("R, rt is: %d\n", (*d).regs.r.rt);
    //get rs
    instr = instr >> 5;
    r_rs = instr & 0x1f;
    (*d).regs.r.rs = r_rs;
    //printf("R, rs is: %d\n", (*d).regs.r.rs);
    //write to register values
    (*rVals).R_rs = mips.registers[(*d).regs.r.rs]; //put the values of rs_register into Rvals_rs
    (*rVals).R_rt = mips.registers[(*d).regs.r.rt]; //put the values of rt_register into Rvals_rt
    (*rVals).R_rd = mips.registers[(*d).regs.r.rd]; //put the values of rd_register into Rvals_rd
}
void i_decode(unsigned int instr, DecodedInstr* d, RegVals* rVals){
    int temp, i_rs, i_rt, i_addr_or_immed, temp2;
    //get rs.
    temp = instr >> 21;
    i_rs = temp & 0x1f;
    (*d).regs.i.rs = i_rs;
    //printf("I, rs is: %d\n", (*d).regs.i.rs);
    //get rt.
    temp = instr >> 16;
    i_rt = temp & 0x1f;
    (*d).regs.i.rt = i_rt;
    //printf("I, rt is: %d\n", (*d).regs.i.rt);
    //get addr_or_immed.
    i_addr_or_immed = instr & 0xffff;
        temp2 = i_addr_or_immed >> 15;
            if((temp2 & 0x1) > 0)
            {
                d->regs.i.addr_or_immed = i_addr_or_immed | 0xffff0000;
            }
            else
            {
                d->regs.i.addr_or_immed = i_addr_or_immed | 0x00000000;
            }
    //printf("I, addr or immed is: %d\n", (*d).regs.i.addr_or_immed);
    //write to register values
    (*rVals).R_rs = mips.registers[(*d).regs.i.rs]; //put the values of rs_register into Rvals_rs
    (*rVals).R_rt = mips.registers[(*d).regs.i.rt]; //put the values of rt_register into Rvals_rt
}
void j_decode(unsigned int instr, DecodedInstr* d, RegVals* rVals){
    //get target
    int temp;
    (*d).regs.j.target = instr & 0x3ffffff;
    (*d).regs.j.target = (*d).regs.j.target << 2; // add two left bits to make 28 bit
    temp = mips.pc & 0xf0000000; // take first 4 bits from pc
    (*d).regs.j.target = (*d).regs.j.target + temp; // add the 4 bits froom pc to the target address, which will become 32-bit
    //printf("J, target is: %d\n", (*d).regs.j.target);

}

/* Decode instr, returning decoded instruction. */
void Decode ( unsigned int instr, DecodedInstr* d, RegVals* rVals) {
    /* Your code goes here */
    int opcode;
    if(instr == 0) // if there is no instruction, terminate
    {
        exit(0);
    }
    (*d).op = instr >> 26; //shift right 26 times, to get the first 6 binary of opcode.
    opcode = (*d).op; //opcode variable now has the opcode of a given instruction.
    //printf("Opcode is: %d\n", opcode);
    // further decoding all different instruction cases.
    switch (opcode)
    {
    // VVVVVVVV I - FORMAT VVVVVVVV
        case 9: // addiu 
            (*d).type = I; 
            i_decode(instr, d, rVals);
        break;
        case 12: // andi
            (*d).type = I; 
            i_decode(instr, d, rVals);
        break;
        case 13: // ori
            (*d).type = I; 
            i_decode(instr, d, rVals);
        break;
        case 15: // lui
            (*d).type = I; 
            i_decode(instr, d, rVals);
        break;
        case 4: // beq
            (*d).type = I; 
            i_decode(instr, d, rVals);
        break;
        case 5: // bne
            (*d).type = I; 
            i_decode(instr, d, rVals);
        break;
        case 35: // lw
            (*d).type = I; 
            i_decode(instr, d, rVals);
        break;
        case 43: // sw
            (*d).type = I; 
            i_decode(instr, d, rVals);
        break;

    // VVVVVVVV R - FORMAT VVVVVVVV
        case 0: 
            (*d).type = R; 
            r_decode(instr, d, rVals);
        break; 

    // VVVVVVVV J - FORMAT VVVVVVVV
        case 2: //j
            (*d).type = J; 
            j_decode(instr, d, rVals);
        break;
        case 3: // jal
            (*d).type = J; 
            j_decode(instr, d, rVals);
        break;
        default:
            exit(0);
        break;
    }
}
/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
void PrintInstruction ( DecodedInstr* d) {
    switch ((*d).op)
    {   //printing the instruction name
        case 9: //addiu
            printf("addiu\t");
        break;
        case 2: // j        
            printf("j\t");
        break;
        case 3: // jal
            printf("jal\t");
        break;
        case 12: // andi
            printf("andi\t");
        break;
        case 13: // ori
            printf("ori\t");
        break;
        case 15: // lui
            printf("lui\t");
        break;
        case 4: //beq
            printf("beq\t");
        break;
        case 5: //bne
            printf("bne\t");
        break;
        case 35: // lw
            printf("lw\t");
        break;
        case 43: // sw
            printf("sw\t");
        break;
        case 0: switch((*d).regs.r.funct) // R-format
                {
                    case 33: // addu
                    printf("addu\t"); 
                    break;
                    case 35: // addiu
                    printf("subu\t");
                    break;
                    case 0: // sll
                    printf("sll\t");
                    break;
                    case 2: // srl
                    printf("srl\t");
                    break;
                    case 36: // and
                    printf("and\t");
                    break;
                    case 37: // or
                    printf("or\t");
                    break;
                    case 42: // slt
                    printf("slt\t");
                    break; 
                    case 8: // jr
                    printf("jr\t");
                    break;  
                }
    }
    // printing the register number and immed or address
    switch ((*d).type)
    {
        case I: // I format
            switch((*d).op)
            {   
                case 15: 
                    printf("$%d, 0x%x\n", (*d).regs.i.rt, (*d).regs.i.addr_or_immed);
                break;
                case 4:
                    printf("$%d, $%d, 0x%8.8x\n", (*d).regs.i.rs, (*d).regs.i.rt, ((*d).regs.i.addr_or_immed*4) + mips.pc + 4);
                break;
                case 5:
                    printf("$%d, $%d, 0x%8.8x\n", (*d).regs.i.rs, (*d).regs.i.rt, ((*d).regs.i.addr_or_immed*4) + mips.pc + 4);
                break;
                case 35:
                    printf("$%d, %d($%d)\n", (*d).regs.i.rt, (*d).regs.i.addr_or_immed, (*d).regs.i.rs);
                break;
                case 43:
                    printf("$%d, %d($%d)\n", (*d).regs.i.rt, (*d).regs.i.addr_or_immed, (*d).regs.i.rs);
                break;
                case 9:
                    printf("$%d, $%d, %d\n", (*d).regs.i.rt, (*d).regs.i.rs, (*d).regs.i.addr_or_immed);
                break;
                case 12:
                    printf("$%d, $%d, 0x%x\n", (*d).regs.i.rt, (*d).regs.i.rs, (*d).regs.i.addr_or_immed);
                break;
                case 13:
                    printf("$%d, $%d, 0x%x\n", (*d).regs.i.rt, (*d).regs.i.rs, (*d).regs.i.addr_or_immed);
                break;
            }
        break;
        case R: // R format
            switch((*d).regs.r.funct)
            {
                case 0:
                    printf("$%d, $%d, $%d\n", (*d).regs.r.rd, (*d).regs.r.rs, (*d).regs.r.shamt);
                break;
                case 2:
                    printf("$%d, $%d, $%d\n", (*d).regs.r.rd, (*d).regs.r.rs, (*d).regs.r.shamt);
                break;
                case 8:
                    printf("$%d\n", (*d).regs.r.rs);
                break;
                case 33:
                    printf("$%d, $%d, $%d\n", (*d).regs.r.rd, (*d).regs.r.rs, (*d).regs.r.rt);
                break;
                case 35:
                    printf("$%d, $%d, $%d\n", (*d).regs.r.rd, (*d).regs.r.rs, (*d).regs.r.rt);
                break;
                case 36:
                    printf("$%d, $%d, $%d\n", (*d).regs.r.rd, (*d).regs.r.rs, (*d).regs.r.rt);
                break;
                case 37:
                    printf("$%d, $%d, $%d\n", (*d).regs.r.rd, (*d).regs.r.rs, (*d).regs.r.rt);
                break;
                case 42:
                    printf("$%d, $%d, $%d\n", (*d).regs.r.rd, (*d).regs.r.rs, (*d).regs.r.rt);
                break;
            }
        break;
        case J: // J format
            printf("0x%8.8x\n", (*d).regs.j.target);
        break;
    }
}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {
    /* Your code goes here */
    int temp;
    switch((*d).type)
    {
        case I:
            switch(d->op)
            {
                case 9: //addiu
                    // rVals->R_rt = rVals->R_rs + d->regs.i.addr_or_immed;
                    temp = rVals->R_rs + d->regs.i.addr_or_immed;
                    //printf("execution output will be: %d\n", temp);
                    return temp;
                break; 
                case 12: //andi
                    temp = rVals->R_rs & d->regs.i.addr_or_immed;
                    //printf("execution output will be: %d\n", temp);
                    return temp;
                break;
                case 13: //ori
                    temp = rVals->R_rs | d->regs.i.addr_or_immed;
                    //printf("execution output will be: %d\n", temp);
                    return temp;
                break;
                case 15: //lui
                    temp = d->regs.i.addr_or_immed << 16;
                    //printf("execution output will be: %d\n", temp);
                    return temp;
                break;
                case 4: //beq
                    if((rVals->R_rs - rVals->R_rt) == 0)
                    {
                        temp = (d->regs.i.addr_or_immed << 2); //shift left by 2, since address is incremented by 4, so
                    }
                    else
                    {
                        temp = 0;
                    }
                    //printf("execution output will be: %x\n", temp);
                    return temp;
                case 5: //bne
                    if((rVals->R_rs - rVals->R_rt) == 0)
                    {
                        temp = 0;
                    }
                    else
                    {
                        temp = (d->regs.i.addr_or_immed << 2); //shift left by 2, since address is incremented by 4, so
                    }
                    //printf("execution output will be: %x\n", temp);
                    return temp;
                case 35: //lw
                    temp = rVals->R_rs + d->regs.i.addr_or_immed;
                    //printf("execution output will be: %d\n", temp);
                    return temp;
                case 43: // sw
                    temp = rVals->R_rs + d->regs.i.addr_or_immed;
                    //printf("execution output will be: %d\n", temp);
                    return temp; 
                
            }
        break;

        case J: 
            switch(d->op)
            {
                case 3: //jal
                    temp = mips.pc + 4;
                    return temp;
                break;
            }
        break;
        case R:
            switch(d->regs.r.funct)
            {
                case 33: //addu
                    temp = rVals->R_rs + rVals->R_rt;
                    return temp;
                break;
                case 35: //subu
                    temp = rVals->R_rs - rVals->R_rt;
                    return temp;
                break;
                case 0: //sll
                    temp = rVals->R_rt << d->regs.r.shamt;
                    return temp;
                break;
                case 2: // srl
                    temp = rVals->R_rt >> d->regs.r.shamt;
                    return temp;
                break;
                case 36: // and
                    temp = rVals->R_rs & rVals->R_rt;
                    return temp;
                break;
                case 37: // or
                    temp = rVals->R_rs | rVals->R_rt;
                    return temp;
                break;
                case 42: // slt
                    temp = (rVals->R_rs - rVals->R_rt) < 0;
                    return temp;
                break;
                case 8: // jr
                    return rVals->R_rs;
                break;
            }

        break;
            
    }
  return 0;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC ( DecodedInstr* d, int val) {
    mips.pc+=4;
    /* Your code goes here */
    switch(d->type)
    {
		case I: 
			switch (d->op){
        		case 4: // beq
                	mips.pc += val; //if beq passes, mips.pc will add to the val (12), then branch to new instruction
        		break;
        		case 5: // bne
                	mips.pc += val; //if bne passes, mips.pc will add to the val, then branch to new instruction
        		break;
			}
		break;
		case J:
			switch (d->op){
        		case 3: // jal
            		mips.registers[31] = mips.pc; //saves next pc onto $ra, which is $31
            		mips.pc = d->regs.j.target; // next jump, will be the target (32bit)
				break;
        		case 2: // j
            		//printf("J INSTRUCTION: ");
            		mips.pc = d->regs.j.target; // jumps to target (32bit)
				break;
			}
		break;
        case R: 
            switch(d->regs.r.funct)
            {
                case 8: // jr
                      mips.pc = val; 
                break;
            }
        break;
    }
}
/*
 * Perform memory load or store. Place the address of any updated memory 
 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value 
 * that is read, otherwise return -1. 
 *
 * Remember that we're mapping MIPS addresses to indices in the mips.memory 
 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1] 
 * with address 0x00400004, and so forth.
 *
 */
int Mem( DecodedInstr* d, int val, int *changedMem) {
    /* Your code goes here */
    *changedMem = -1;
    switch(d->op)
    {
        case 35: // lw
            if(val < 0x00401000 || val > 0x00404004 || val % 4 != 0){
                printf("Memory Access Exception at 0x%8.8x: address 0x%8.8x\n", mips.pc - 4, val);
                exit(0);
            }
            else{
                mips.registers[d->regs.i.rt] = mips.memory[(val-0x00400000)/4]; // load word from memory address to rt register, val will be the address.
                *changedMem = -1;
                val = mips.registers[d->regs.i.rt];
            }
        break;
        case 43: // sw
            if(val < 0x00401000 || val > 0x00404004 || val % 4 != 0){ // check if the address is outside of data memory and if it is al
                printf("Memory Access Exception at 0x%8.8x: address 0x%8.8x\n", mips.pc - 4, val);
                exit(0);
            }
            else{
                mips.memory[(val-0x00400000)/4] = mips.registers[d->regs.i.rt]; // store word from rt register to memory address.
                *changedMem = val;
                val = -1;
            }
        break;
    }
  return val;
}
/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg) {
    /* Your code goes here */
    *changedReg = -1;
	int ra = 31;
    switch (d->type)
    {
        case I:
            switch (d->op)
            {
            	case 35: //lw
                    mips.registers[d->regs.i.rt] = val;
                    *changedReg = d->regs.i.rt;
				break;
            	case 15: // lui
                    mips.registers[d->regs.i.rt] = val;
                    *changedReg = d->regs.i.rt;
				break;
             	case 12: // andi
                    mips.registers[d->regs.i.rt] = val;
                    *changedReg = d->regs.i.rt;
				break;
            	case 13: // ori
                    mips.registers[d->regs.i.rt] = val;
                    *changedReg = d->regs.i.rt;
				break;
            	case 9: // addiu
                    mips.registers[d->regs.i.rt] = val;
                    *changedReg = d->regs.i.rt;
            	break;
            }
        break;
        case J:
            switch (d->op)
            {
            	case 3: //jal
					mips.registers[ra] = val; // write the val (next pc address after jal) to save into register $ra.
					*changedReg = ra; // update register to 31
                break;
            }
        break;
        case R:
            switch (d->regs.r.funct)
            {
                case 33: // addu
                    mips.registers[d->regs.r.rd] = val;
                    *changedReg = d->regs.r.rd;
                break;
                case 35: // subu
                    mips.registers[d->regs.r.rd] = val;
                    *changedReg = d->regs.r.rd;
                break;
                case 36: // and
                    mips.registers[d->regs.r.rd] = val;
                    *changedReg = d->regs.r.rd;
                break;
                case 37: // or
                    mips.registers[d->regs.r.rd] = val;
                    *changedReg = d->regs.r.rd;
                break;
                case 42: //slt
                    mips.registers[d->regs.r.rd] = val;
                    *changedReg = d->regs.r.rd;
                break;
                case 0: // sll
                    mips.registers[d->regs.r.rd] = val;
                    *changedReg = d->regs.r.rd;
                break;
                case 2: // srl
                    mips.registers[d->regs.r.rd] = val;
                    *changedReg = d->regs.r.rd;
                break;
            }
        break; 
    }
}