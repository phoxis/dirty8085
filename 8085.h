/**  This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 **
 ** The GNU General Public License version 2 is included below for your reference.
 **/

typedef unsigned char u8int_t;
typedef unsigned short int u16int_t;
typedef unsigned int u32int_t;

#define MAX_MEM 0x00010000

#define UPDATE_SIGN         0x00
#define SET_SIGN            0x01
#define CLEAR_SIGN          0x02
#define NO_CHANGE_SIGN      0x03

#define UPDATE_ZERO         0x04
#define SET_ZERO            0x05
#define CLEAR_ZERO          0x06
#define NO_CHANGE_ZERO      0x07

#define UPDATE_AUXCARRY     0x08
#define SET_AUXCARRY        0x09
#define CLEAR_AUXCARRY      0x0a
#define NO_CHANGE_AUXCARRY  0x0b

#define UPDATE_PARITY       0x0c
#define SET_PARITY          0x0d
#define CLEAR_PARITY        0x0e
#define NO_CHANGE_PARITY    0x0f

#define UPDATE_CARRY        0x10
#define SET_CARRY           0x11
#define CLEAR_CARRY         0x12
#define NO_CHANGE_CARRY     0x13

#define MAX_MEM 0x00010000

#define EXECUTE_FULL        0x01
#define EXECUTE_STEP        0x02
#define EXECUTE_OVER        0x03
#define EXECUTE_END         0xff
#define EXECUTE_ERR         0x55

#define true  1
#define false 0

#define ERANGE 1

struct _instruction_set {
  u8int_t (*ins)(void);
  char ins_str[10];
  char operand_1[10];
  char operand_2[10];
  u8int_t mem_opnd; // the upper nibble represents the no of mem operations of 1st opnd, the lower nibble tells the no of mem opnds in 2nd operand
//   u8int_t bytes; //the instruction length //TODO: initilize the array with this info
};

typedef struct __8085_machine_t {
  /* Registers */
  u16int_t a;
  u8int_t flag;
  u8int_t b;
  u8int_t c;
  u8int_t d;
  u8int_t e;
  u8int_t h;
  u8int_t l;
  u16int_t sp;
  u16int_t pc;
  
  /* Last copy of register A previous state */
  u16int_t a_last;

  /* Memory */
  u8int_t memory[MAX_MEM];
  
  /* Last pc val */
  u16int_t prev_pc;

  /* Current states */
  struct _instruction_set *current_instruction;
} _8085_machine_t;

u16int_t tm_get_hl (void);
u16int_t tm_get_bc (void);
u16int_t tm_get_de (void);

_Bool tm_get_sign (void);
_Bool tm_get_zero (void);
_Bool tm_get_auxcarry (void);
_Bool tm_get_parity (void);
_Bool tm_get_carry (void);

char *tm_get_opcode_str (void);
u8int_t tm_get_opcode_hex (void); 
char *tm_get_operand_1 (void);
char *tm_get_operand_2 (void);

u8int_t tm_fetch_memory (u16int_t loc);
void tm_set_memory (u16int_t loc, u8int_t val);

u8int_t  tm_get_reg_b  (void);
u8int_t  tm_get_reg_c  (void);
u8int_t  tm_get_reg_d  (void);
u8int_t  tm_get_reg_e  (void);
u8int_t  tm_get_reg_h  (void);
u8int_t  tm_get_reg_l  (void);
u8int_t  tm_get_reg_a  (void);
u16int_t tm_get_reg_sp (void);
u16int_t tm_get_reg_pc (void);

void tm_clear_regs (void);
void tm_clear_flags (void);

u16int_t tm_get_reg_prev_pc (void);

void register_machine (_8085_machine_t *machine);

u8int_t execute (u16int_t start_addr);
u8int_t execute_step (u16int_t start_addr, u8int_t mode);
