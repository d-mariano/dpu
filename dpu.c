/*********************************************************
 *  Author:     Dave Mariano
 *  Date:       January 30, 2015
 *  Filename:   dpu.c
 *      
 *  Functions that make up the DPU - Dave Processing Unit
 *
 *********************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dpu.h"


/**
 *	DPU startup function that initializes memory and provides 
 *	an everlasting loop.  An input character  is taken and 
 *	handled according to the available options.
 */
int dpu_start(){
    unsigned char memory[MEM_SIZE];
    unsigned char choice[BUFF_SIZE];
    unsigned char flush[BUFF_SIZE];
    unsigned int offset, length, i;
    int bytes;

    /* Reset registers */
    dpu_reset();
    
    /*  Print title and command list */
    printf("    |=-=-=-=-=-=-=-=-=--->>DPU<<---=-=-=-=-=-=-=-=-=|\n"); 
    dpu_help();
    
    /*  Loop forever */
    forever{
        // Prompt
        printf("> ");

        // Obtain a choice from the user/stdin
        fgets(choice, BUFF_SIZE, stdin);
        
        // Ensure the choice is lower-case
        for(i = 0; i < strlen(choice); i++){
            choice[i] = tolower(choice[i]);    
        }    
        
        // Switch to execture correct function 
        switch(choice[0]){
            case 'd':
                printf("Enter offset in hex:\t");
                // Test for a valid intake
                if(scanf("%x", &offset) == 0){
                    printf("Not a valid offset.\n");
                    break;
                }    
                // Test for an offset in range
                if(offset >= MEM_SIZE){
                    printf("Not a valid offset.\n");
                    // Flush input buffer
                    fgets(flush, BUFF_SIZE, stdin);
                    break;
                }
                fgets(flush,BUFF_SIZE, stdin);
                printf("Enter length in hex:\t");
                if(scanf("%x", &length) == 0){
                    printf("Not a valid length.\n");
                    break;
                }    
                // Flush input 
                fgets(flush, BUFF_SIZE, stdin);
                
                dpu_dump(memory, offset, length);
                break;
            case 'g':
                
                 while(!flag_stop){
                    dpu_instCycle(memory);
                 }
                 
                break;
            case 'l':
                bytes = dpu_LoadFile(memory, MEM_SIZE);
                if(bytes >= 0){
                    printf("0x%x(%d) bytes have been loaded into memory from file.\n", (unsigned int)bytes, (unsigned int)bytes);        
                }    
                break;
            case 'm':
                printf("Enter offset in hex:\t");
                // Ensure proper value is taken in
                if(scanf("%x", &offset) == 0){
                    printf("Not a valid offset.\n");
                    break;
                }    
                // Flush input
                fgets(flush, BUFF_SIZE, stdin);
                // Test for an offset in range
                if(offset >= MEM_SIZE){
                    printf("Not a valid offset.\n");
                    break;
                }    
                       
                dpu_modify(memory, offset);
                break;
            case 'q':
                printf("Goodbye.\n");
                return dpu_quit();
            case 'r':
                dpu_reg();
                break;
            case 't':
                dpu_instCycle(memory);  
                dpu_reg();
                break;
            case 'w':
                dpu_WriteFile(memory);
                break;
            case 'z':
                dpu_reset();
                printf("Registers have been reset.\n");
                break;
            // If the user selects '?' then the case will fall into 'h'
            case '?':
            case 'h':
                dpu_help();
                break;
            default:
                printf("%c is not an option.  Enter H/h/? for help.\n", choice[0]);
        }
    }    
}

int dpu_go(){
    return 0;
}


/**
 *  Memory Dump:  Dump length amount of memory, beginning  at offset
 */
int dpu_dump(void * memptr, unsigned int offset, unsigned int length){
    unsigned int i;
    unsigned char line[LINE_LENGTH];
    unsigned int lineLength = LINE_LENGTH;
    unsigned int count = 0;

    while(count < length){
    
        if(offset == MEM_SIZE){
            break;
        } 

        /* Print the offset/block number */
        printf("%04X\t", offset);
        /* Create the line */
        for(i = 0; i < LINE_LENGTH; i++, offset++, count++){
            /* Ensure that the pointer does not go out of bounds */
            if(offset == MEM_SIZE){
                lineLength = i;
                break;
            }
            /* Ensure that the line length does not exceed desired length */
            if(count == length){
                lineLength = i;
                break;
            }      
            line[i] = *((char*)memptr + offset);
            printf("%02X ", line[i]);
        }
        /* Move to a newline and continue to ASCII representation of the 
         * line dumped.
         */   
        printf("\n\t");
        for(i = 0; i < lineLength; i++){
            if(isprint(line[i])){
                printf(" %c ", line[i]);
            }else{
                printf(" . ");
            }    
        }
        /* Newline for next block */
        printf("\n");
    }

    return 0;
}


/**
 *	Function to load data from a file into memory.
 */
int dpu_LoadFile(void * memory, unsigned int max){
    FILE* file;
    int nbytes;
    unsigned char filename[BUFF_SIZE];
    unsigned char buff[BUFF_SIZE];
    unsigned char error[BUFF_SIZE];
    unsigned char flush[BUFF_SIZE];
    unsigned long fsize;


    /* Prompt for filename */
    printf("\nEnter a filename: ");
    
    /* Obtain filename */
    fgets(filename, BUFF_SIZE, stdin);
    
    /* Nullify the last byte */
    filename[strlen(filename)-1] = '\0';

    /* Open the file */
    if((file = fopen(filename, "rb")) == NULL){
        sprintf(error, "load: fopen: %s", filename);
        perror(error);
        return -1;
    }

    printf("Loading %s to memory.\n", filename);
    
    /* Discover file size */
    if(fseek(file, 0, SEEK_END) == -1){
        perror("load: fseek");
        fclose(file);
        return -1;
    }
    
    /* Record file size as file position inidcator of ftell */
    if((fsize = ftell(file)) == -1){
        perror("load: ftell");
        fclose(file);
        return -1;
    }
    
    /* Set file position to beginning of file */
    rewind(file);

    /* Truncate file if it cannot fit in memory */
    if(max < fsize){
        fsize = max;
        printf("Data from file has been truncated to fit into memory.\n");
    }

    /* Read the file into memory */
    nbytes = (int)fread(memory, BYTE_SIZE, (size_t)fsize, file);
    
    if(ferror(file)){
        perror("load: fread");
        fclose(file);
        return -1;
    }

    fclose(file);
    
    return (int)nbytes;
}


/**
 *  Memory Modify:  
 *      Modify bytes of memory, in hex, beginning at offset.
 */
int dpu_modify(void * memptr, unsigned int offset){
    unsigned char input[BUFF_SIZE];
    unsigned char flush[BUFF_SIZE];
    unsigned int byte;
    unsigned int i;
    
    printf("*All byte values accepted in hex.*\nEnter '.' to stop.\n\n");

    forever{
        // Print current offset and value
        printf("%04X : %02X > ", offset, *((unsigned char*)memptr + offset));
        // Get input
        fgets(input, BUFF_SIZE, stdin);
        // Nullify newline element
        input[2] ='\0';

        // Check if input is quit
        if(input[0] == '.'){
            break;    
        }
        // Check if input is null
        if(strcmp(input, "") == 0){
            // Ignore invalid input and continue
            continue;
        }
        // Check if input is enter
        if(strcmp(input, "\n") == 0){
            // Ignore invalid input continue
            continue;
        }    

        // Ensure the string is lowercase
        for(i = 0; i < strlen(input); i++){
            input[i] = tolower(input[i]);
        }

        // Capture input as a hex-byte
        if(sscanf(input, "%x", &byte) == 0){
            continue;
        }

        // Assign offset with new byte value
        *((unsigned char*)memptr + offset) = byte;
        // Increment offset to next byte
        ++offset;
        
        if(offset == MEM_SIZE){
            printf("End of memory.\n");
            break;
        }    
    }
    return 0;
}    

int dpu_quit(){
    return 0;    
}


/**
 * Register dump:   
 *      Display all registers and flags with their current values.
 */
int dpu_reg(){
    unsigned int i;

    /* Print regsiter file */
    for(i = 0; i < RF_SIZE; i ++){
        if(i % LINE_BREAK == 0){
            printf("\n");
        }
        if(i == RF_SP){
            printf(" SP:%08X ", SP);
        }else if(i == RF_LR){
            printf(" LR:%08X ",  LR);
        }else if(i == RF_PC){
            printf(" PC:%08X ",  PC);
        }else{    
            printf("r%02d:%08X ", i,  regfile[i]);
        }    
    }    
    
    /* Print flags */
    printf("\t SZC:%d%d%d", flag_sign, flag_zero, flag_carry);

    /* Print non-visible registers */
    printf("\n   MAR:%08X   MBR:%08X   IR0:%04X   IR1:%04X   Stop:%0d   IR Flag:%01d\n",  mar,  mbr, IR0, IR1, flag_stop, flag_ir);

    return 0;
}


/**
 *	Write to File:
 *	    Function to write bytes from memory to a file.
 */
void dpu_WriteFile(void * memory){
    FILE* file;
    int nbytes;
    int wbytes;
    unsigned char filename[BUFF_SIZE];
    unsigned char error[BUFF_SIZE];
    unsigned char flush[BUFF_SIZE];

    // Retrieve filename
    printf("\nEnter a filename: ");
    fgets(filename, BUFF_SIZE, stdin);
    
    // Nullify last byte
    filename[strlen(filename) -1] = '\0';

    // Retrieve number of bytes to write
    printf("\nEnter the amount of bytes to write: ");
    if(scanf("%d", &nbytes) == 0){
        printf("Invalid input.\n");
        return;
    }    
    // Flush input stream
    fgets(flush, BUFF_SIZE, stdin);
    
    /** Check if number of bytes specified is greater than memory, 
     *  less than 0, or 0.  
     */
    if(nbytes > MEM_SIZE){
        printf("File not written.  Cannot write more bytes than in memory.\n");
        return;
    
    }else if(nbytes <= 0){
        printf("0 bytes written.\n");
        return;
    }

    // Open the file
    if((file = fopen(filename, "wb")) == NULL){
        sprintf(error, "write: %s", filename);
        perror(error);
        return;
    }

    // Write memory to file
    if((wbytes = fwrite(memory, BYTE_SIZE, nbytes, file)) < 0){
        perror("dpu: write: ");
    }else{
        printf("%d bytes have been written to %s.\n", wbytes, filename);
    }
    
    // Close the file
    fclose(file);

    return;
}


/**
 *  Reset: Reset all registers to 0.
 */
int dpu_reset(){
    unsigned int i;
    
    // Reset visible registers
    for(i = 0; i < RF_SIZE; i++){
        regfile[i] = 0;
    }
    // Reset flags
    flag_sign = 0;
    flag_zero = 0;
    flag_carry = 0;
    flag_stop = 0;
    flag_ir = 0;
    // Non-visible registers
    mar = 0;
    mbr = 0;
    ir = 0;
    // Unofficial current instruction register
    cir = 0;
    
    return 0;
}


/**
 * Help:  Function to print DPU options to the user
 *	      in the form of a menu.
 */
void dpu_help(){
    printf("\td\tdump memory\n"
            "\tg\tgo - run the entire program\n"
            "\tl\tload a file into memory\n"
            "\tm\tmemory modify\n"
            "\tq\tquit\n"
            "\tr\tdisplay registers\n"
            "\tt\ttrace - execute one instruction\n"
            "\tw\twrite file\n"
            "\tz\treset all registers to zero\n"
            "\t?, h\tdisplay list of commands\n");
}


/********************************************************************
 * Instruction Cycle:   
 *      An instruction cycle consists of a fetch, which provides two 
 *      instructions, and an execute for each of said instructions.  
 *      An IR flag is uses to determine the which instruction is to be
 *      executed from either 16-bit IR0 or IR1 of the 32-bit 
 *      Instruction Register.  If the flag is high, IR1 will be executed,
 *      then the flag will be set low.  If the flag is low, a fetch is made,
 *      providing IR0 and IR1 with the new instructions, and the flag is 
 *      then set high.  
 *
 ***********************************************************************/
void dpu_instCycle(void * memory){
    /* Determine which IR to use via IR Active flag */
    if(flag_ir == 0){
        flag_ir = 1;
        /* Fetch new set of instructions */
        dpu_fetch(memory);
        /* Current instruction is now IR0 */
        cir = IR0;
        dpu_execute(memory);
    }else{
        flag_ir = 0;
        cir = IR1;
        dpu_execute(memory);
    }     
}


/********************************************************
 * Fetch:  Fetch an instruction from memory, at the address
 *         of the program counter.  Memory is 8 bits, and so
 *         4 bytes must be collected into the MBR, starting at the
 *         memory offset specified by MAR, as the Memory Buffer 
 *         Register is 32 bits.  This requires bit shifting 8 bits 
 *         to the left for each byte.  Doing this 4 times will equate 
 *         to 32 bits read.  The PC is incremented by the size of one
 *         register/instruction after the contents of MBR are stored in
 *         the Instruction Register.
 **************************************************************/         
void dpu_fetch(void * memory){
    /* MAR <- PC */
   // mar = PC;
    
    ir = dpu_loadReg(PC, memory);
    
    /* PC + 1 instruction */
    PC += REG_SIZE;
}

/***************************************************************
 * Load Register: Load a register with memory at location of MAR.
 *                MAR must be set before this function is called.
 ******************************************************************/
uint32_t dpu_loadReg(uint32_t marValue, void * memory){
    unsigned int i;

    mar = marValue;

    /* MBR <- memory[MAR] */        /* PC <- + 1 instruction */
    for(i = 0; i < CYCLES; i++, mar++){
        mbr = mbr << SHIFT_BYTE;
        /* Add memory at mar to mbr */
        mbr += *((unsigned char*)memory + mar);
    }     

    /* Register <- MBR */
    return mbr;    
}

/***************************************************************
 * Store Register: Store an entire register into memory at MAR.
 *                 
 ******************************************************************/
void dpu_storeReg(uint32_t marValue, uint32_t mbrValue, void * memory){
    
    mar = marValue;
    mbr = mbrValue;

    *((unsigned char*)memory + mar++) = (unsigned char)(mbr >> SHIFT_3BYTE & BYTE_MASK);
    *((unsigned char*)memory + mar++) = (unsigned char)(mbr >> SHIFT_2BYTE & BYTE_MASK);
    *((unsigned char*)memory + mar++) = (unsigned char)(mbr >> SHIFT_BYTE & BYTE_MASK);
    *((unsigned char*)memory + mar) = (unsigned char)mbr & BYTE_MASK;
}

/***************************************************************
 * Execute: Recognize instruction type, acknowledge instruction 
 *          fields, execute instruction based on instruction field values.
 *          Instruction field values are determined in the header.
 ******************************************************************/
void dpu_execute(void * memory){
    int i;

    /* Recognize instruction type */
    
    /* 
     * Data Processing 
     */
    if(DATA_PROC){
        /* Acknowledge Operation field */
        if(DATA_AND){
            alu = regfile[RD] & regfile[RN];
            dpu_flags(alu);
            regfile[RD] = alu;
        }else if(DATA_EOR){
            alu = regfile[RD] ^ regfile[RN];
            dpu_flags(alu);
            regfile[RD] = alu;
        }else if(DATA_SUB){
            alu = regfile[RD] + ~regfile[RN] + 1;
            dpu_flags(alu);
            flag_carry = iscarry(regfile[RD], ~regfile[RN], 1);
            regfile[RD] = alu;
        }else if(DATA_SXB){
            alu = regfile[RN];
            if((alu & MSB8_MASK) == 1){
                alu += SEX8TO32;
            }
            dpu_flags(alu);
            regfile[RD] = alu;
        }else if(DATA_ADD){
            alu = regfile[RD] + regfile[RN];
            dpu_flags(alu);
            flag_carry = iscarry(regfile[RD], ~regfile[RN], 0);
            regfile[RD] = alu;
        }else if(DATA_ADC){
            alu = regfile[RD] + regfile[RN] + flag_carry; 
            dpu_flags(alu);
            flag_carry = iscarry(regfile[RD], regfile[RN], flag_carry);
            regfile[RD] = alu;
        }else if(DATA_LSR){
            for(i = 0; i < regfile[RN]; i++){
                flag_carry = regfile[RN] & LSB_MASK;
                alu = regfile[RD] >> 1;
            }
            dpu_flags(alu);
            regfile[RD] = alu;
        }else if(DATA_LSL){
            for(i = 0; i < regfile[RN]; i++){
                flag_carry = regfile[RN] & LSB_MASK;
                alu = regfile[RD] << 1;
            }
            dpu_flags(alu);
            regfile[RD] = alu;
        }else if(DATA_TST){
            alu = regfile[RD] & regfile[RN];
            dpu_flags(alu);
        }else if(DATA_TEQ){
            alu = regfile[RD] ^ regfile[RN];
            dpu_flags(alu);
        }else if(DATA_CMP){
            alu = regfile[RD] + ~regfile[RN] + 1;
            dpu_flags(alu);
            flag_carry = iscarry(regfile[RD], ~regfile[RN], 1);
        }else if(DATA_ROR){
            for(i = 0; i < regfile[RN]; i++){
                flag_carry = regfile[RD] & LSB_MASK;
                alu = regfile[RD] >> 1;
                /* Set the MSB of the alu to the value shifted left */
                if(flag_carry){
                    alu |= MSB32_MASK;
                }
            }
            dpu_flags(alu);
            regfile[RD] = alu;
        }else if(DATA_ORR){
            alu = regfile[RD] | regfile[RN];
            dpu_flags(alu);
            regfile[RD] = alu;
        }else if(DATA_MOV){
            regfile[RD] = regfile[RN];
            dpu_flags(regfile[RD]);
        }else if(DATA_BIC){
            alu = regfile[RD] & ~regfile[RN];
            dpu_flags(alu);
            regfile[RD] = alu;
        }else if(DATA_MVN){
            alu = ~regfile[RN];
            dpu_flags(alu);
            regfile[RD] = alu;
        }
    }
    /* 
     * Load/Store 
     */
    else if(LOAD_STORE){
        /* MAR <- regfile[RN] */
        
        if(LOAD_BIT){
            /*Load Byte*/
            if(BYTE_BIT){
                regfile[RD] = dpu_loadReg(regfile[RN], memory);
                regfile[RD] = regfile[RD] & BYTE_MASK;
            }
            /*Load Double Word*/
            else{
                regfile[RD] = dpu_loadReg(regfile[RN], memory);
            }
        }else{
            mbr = regfile[RD];
            /* Store one byte of the register into memory */
            if(BYTE_BIT){
                mar = regfile[RN];
                mbr = regfile[RD];
                *((unsigned char*)memory + mar) = (unsigned char)mbr & BYTE_MASK;
            }
            /*Store double word*/
            else{
                dpu_storeReg(regfile[RN], regfile[RD], memory);
            }
        } 
    /* 
     * Immediate Operations 
     */
    }else if(IMMEDIATE){
        /* Move immediate value into regfile at RD */
        if(MOV){
            regfile[RD] = IMM_VALUE;    
            dpu_flags(regfile[RD]);
        }else if(CMP){
            alu = regfile[RD] + ~IMM_VALUE + 1;
            dpu_flags(alu);
            flag_carry = iscarry(regfile[RD], ~IMM_VALUE, 0);
        }else if(ADD){
            alu = regfile[RD] + IMM_VALUE;
            dpu_flags(alu);
            flag_carry = iscarry(regfile[RD], IMM_VALUE, 0);
            regfile[RD] = alu;
        }else if(SUB){
            alu = regfile[RD] + ~IMM_VALUE + 1;
            dpu_flags(alu);
            flag_carry = iscarry(regfile[RD], ~IMM_VALUE, 1);
            regfile[RD] = alu;
        }    
    /* 
     * Conditonal Branch 
     */
    }else if(COND_BRANCH){
        /* Check condition codes and flags */
        if(dpu_chkbra()){
            /* Add relative address as a signed 8-bit */
            alu = PC + (int8_t)COND_ADDR;

            /* If IR1 is going to be executed next, the IR flag must be
             * set to 0.  If this is not done, IR1's instruction will be
             * executed as it has not been changed due to not fetching new 
             * instructions right after changing the PC.  Also, if the IR flag is 
             * high, then the PC was pointing to two instructions after the one 
             * that is in IR0, being the branch that just executed. We now have to 
             * decrement the ALU value by 2 to compensate for this before commiting
             * to the PC.
             */
            if(flag_ir != 0){
                flag_ir = 0;
                alu = alu + ~THUMB_SIZE + 1;
            }
            PC = alu;
        }        
    /* 
     * PUSH / PULL
     */
    }else if(PUSH_PULL){
        /* PULL */
        if(LOAD_BIT){
            /* High Registers */
            if(HIGH_BIT){
                /* Registers 8 - 15 */
                for(i = HI_REG; i < RF_SIZE; i++){
                    /* Registers must be represented by what bit number
                       they occupy.  HIGH reg's subtract half the list size.*/
                    if(dpu_chkRList( i - HALF_RF )){
                        /*If the current index is set on the register list: */

                        /* Set MAR to be the stack pointer */
                        regfile[i] = dpu_loadReg(SP & SP_MASK, memory);
                        /* Post increment */
                        alu = SP + REG_SIZE;
                        SP = alu;
                    }
                }
            }
            /* Low Registers */
            else{
                /* Registers 0 - 7 */
                for(i = 0; i <= LOW_LIMIT; i++){
                    if(dpu_chkRList(i)){
                        regfile[i] = dpu_loadReg(SP & SP_MASK, memory);
                        alu = SP + REG_SIZE;
                        SP = alu;
                    }
                }
            }

            /* Check if PC is to be pulled for return. */
            if(RET_BIT){
                 /* If the IR flag is 1, change it to 0 so the next thumb 
                    instruction is not executed. */
                PC = dpu_loadReg(SP & SP_MASK, memory);
                if(flag_ir !=0){
                    flag_ir = 0;
                }
                alu = SP + REG_SIZE;
                SP = alu;
            }

        }
        /* PUSH */
        else{
            if(RET_BIT){
                 /* Pre-decrement */
                alu = SP + ~REG_SIZE + 1;
                SP = alu;
                /* Store the Link Register/return address for jump-returns */
                dpu_storeReg(SP & SP_MASK, LR, memory);
            }
            if(HIGH_BIT){
                for(i = (RF_SIZE - 1); i >= HI_REG; i--){
                    if(dpu_chkRList( i - HALF_RF )){
                        alu = SP + ~REG_SIZE + 1;
                        SP = alu;
                        dpu_storeReg(SP & SP_MASK, regfile[i], memory);
                    }
                }
            }else{
                for(i = LOW_LIMIT; i >= 0; --i){
                    if(dpu_chkRList(i)){
                        alu = SP + ~REG_SIZE + 1;    
                        SP = alu;
                        //SP_DEC;
                        dpu_storeReg(SP & SP_MASK, regfile[i], memory);
                    }
                }
            }
        }
    }
    /* 
     * Unonditional Branch 
     */
    else if(BRANCH){
        if(LINK_BIT){
            LR = PC;
        }    
        PC = OFFSET12;
        /* Make sure the IR flag is not still HI after the PC has changed.
         * If it is, IR1 will execute before a fetch is made to reach the 
         * instruction being branched to.
         */
        flag_ir = 0;
    /* 
     * Stop 
     */
    }else if(STOP){
        flag_stop = 1;
    }    

}    

/*************************************************************
 *  dpu_chkbra() - Check condition code and flags, if a branch
 *                 is to be made, then return 1; else 0.
 *  
 *  
 ********************************************************/
int dpu_chkbra(){
    if(EQ){
        if(flag_zero){
            return 1;
        }    
    }else if(NE){
        if(flag_zero == 0){
            return 1;
        }
    }else if(CS){
        if(flag_carry){
            return 1;
        }
    }else if(CC){
        if(!flag_carry){
            return 1;
        }
    }else if(MI){
        if(flag_sign){
            return 1;      
        }    
    }else if(PL){
        if(!flag_sign){
            return 1;
        }
    }else if(HI){
        if(flag_carry && flag_zero == 0){
            return 1;   
        }
    }else if(LS){
        if(flag_carry == 0 || flag_zero){
            return 1;
        }
    }else if(AL){
        return 1;
    }
    
    return 0;
}


/*************************************************************************
 * dpu_chkRList() - Checks the register list field of the current PUSH/PULL  
 *           instruction to determine if the register file index passed
 *           to this function is to be PUSH/PULLed; returns 0 if it won't.
 *           For HIGH register PUSH/PULLs, the passed index must be 
 *           subtracted by half the size of the register file.
 ********************************************************************/
int dpu_chkRList(int index){
    switch(index){
        case 0:
            return REG_LIST & R0;
        case 1:
            return REG_LIST & R1;
        case 2:
            return REG_LIST & R2;
        case 3:
            return REG_LIST & R3;
        case 4:
            return REG_LIST & R4;
        case 5:
            return REG_LIST & R5;
        case 6:
            return REG_LIST & R6;
        case 7:
            return REG_LIST & R7;
    }
}

/****************************************************************
 * dpu_flags() - This routine checks for flags zero and sign, 
 *              regarding the result in the ALU register.
 *************************************************************/          
void dpu_flags(uint32_t alu){        
    if(alu == 0){
        flag_zero = 1;
    }else{
        flag_zero = 0;
    }    
    
    flag_sign = (alu & MSB32_MASK) >> MSBTOLSB;

}


/**********************************************************
 *   iscarry()- determine if carry is generated by addition: op1+op2+C
 *     C can only have value of 1 or 0.
 ************************************************************/
int iscarry(uint32_t op1,uint32_t op2, uint8_t c){
    if ((op2== MAX32)&&(c==1)) 
        return(1); // special case where op2 is at MAX32
    return((op1 > (MAX32 - op2 - c))?1:0);
}



