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

#include <stdio.h>
#include <string.h>
#include "8085.h"

/* NOTE: WORKING! */
/* TODO: 
 * [**] incorporate step back, so that one can use the stop over and step back to see both the hex codes and decoded strings
 * [1]later incorporate tstate calculation 
 * [2]use return by reference and make array access for hl pair or whatever as funciton calls
 * [3 DONE]instead of multi funciton calls to set or reset or update flags, we can call one function
 * which will set all the flags. we can pass macros like SET, CLEAR, UPDATE, NO_CHANGE etc [IMPLEMENTED], we have
 * accessed the tm_update_flags function wherever possible.
 * [4] we should implement a seperate io space, just like the memory and work there
 * [5]think where to place the function pointer array
 * [6 DONE]doing make_val_16 (fetch_operand (), fetch_operand ()), passes the the returned elements in wrong order check why.
 * [7]shall we make the globals static ? so that it could be hidden from outside this file. Then we can implement the memory and their
 * access functions in one file and then construct the rest of the code in other files. so we can use access these variables from functions
 * outside this file.
 * [8]we can connect some of the io ports to the stdin and stdout files, or other files for io, think later
 * [9] messy code, unstructured data, plan and structre it .
 * [**10] update the auxiliary carry flag properly. think how it should be detected
 * [**11] DAA: if carry is 1 then do not modyfy, else if 0 then modify 
 * [**12] add the undocumented instructions
 */
/*
 * BUGFIXED:
 * [1] ral, rar, rlc, rrc shiftings had bugs, killed
 * [2] sub,sui,sbb subtraction had problems with zero extension when complementing, now masked with 0xff and carry complemented. Although previously the answer was correct, but the 8085 internal method was not followed. this was fixed.
 * [3] ral, rar, rlc, rrc shifting were interchanged and implemented, fixed
 * [4] cma was complimenting the whole internal 2byte Accumulator, generating carry because the
 *     upper byte was ff and then aci adi calculating wrong sum aS the carry was set
 * [5] inr a, fixed, zero flag was not set properly
 * [6] the conditional return famaly was popping from stack , and then checking condition.If cond not true, it was returning without change in PC , but as the stack was popped, everything is messed up. This is fixed.
 */
/* FIXME: CHECK THE INR AND DCR ROUTINES AND CONFIRM THAT THE HIGHER INTERNAL BYTE OF ACCUMULATOR IS NOT CAUSING ANY PROBLEM in flags. CONFIRM THAT NO REGISTER'S INTERNAL UPPER BYTE IS CONSIDERED AT ANY COST*/
/* FIXME: THE DAA COULD NOT BE USED AT THIS MOMENT, AS THE AUXCARRY FLAG IS NOT BEING SET BY ANY ROUTINE */

_8085_machine_t *tm;

static u16int_t xy_pair_get (u8int_t x, u8int_t y)
{
  u16int_t reg_pair = x;
  reg_pair <<= 8;
  reg_pair |= (y & 0xff);
  return reg_pair;
}

static void xy_pair_set (u8int_t *x, u8int_t *y, u16int_t val)
{
  *x = (val >> 8) & 0xff;
  *y = val & 0xff;
}

u16int_t tm_get_hl (void) { return xy_pair_get (tm->h, tm->l); }
u16int_t tm_get_bc (void) { return xy_pair_get (tm->b, tm->c); }
u16int_t tm_get_de (void) { return xy_pair_get (tm->d, tm->e); }

static void tm_set_hl (u16int_t val) { xy_pair_set (&tm->h, &tm->l, val); }
static void tm_set_bc (u16int_t val) { xy_pair_set (&tm->b, &tm->c, val); }
static void tm_set_de (u16int_t val) { xy_pair_set (&tm->d, &tm->e, val); }

static void tm_set_sign (void) { tm->flag |= 0x80; }
static void tm_clear_sign (void) { tm->flag &= 0x7f; }
_Bool tm_get_sign (void) { return ((tm->flag & 0x80) == 0x80); }

static void tm_set_zero (void) { tm->flag |= 0x40; }
static void tm_clear_zero (void) { tm->flag &= 0xbf; }
_Bool tm_get_zero (void) { return ((tm->flag & 0x40) == 0x40); }

static void tm_set_auxcarry (void) { tm->flag |= 0x10; }
static void tm_clear_auxcarry (void) { tm->flag &= 0xef; }
_Bool tm_get_auxcarry (void) { return ((tm->flag & 0x10) == 0x10); }

static void tm_set_parity (void) { tm->flag |= 0x04; }
static void tm_clear_parity (void) { tm->flag &= 0xfb; }
_Bool tm_get_parity (void) { return ((tm->flag & 0x04) == 0x04); }
      
static void tm_set_carry (void) { tm->flag |= 0x01; }
static void tm_clear_carry (void) { tm->flag &= 0xfe; }
_Bool tm_get_carry (void) { return ((tm->flag & 0x01) == 0x01); }


/* NOTE: we should also make a way to store the current operand in a global 
 * we need to udpate the functions to handle these variables. these funcitons
 * could be used outside of this file to know the current operands and operator.
 */




/* NOTE: won't these be unnecessary levels of function calls ?
static void stack_push_16 (u16int_t val)
{
  memory[--sp] = 
}

static void stack_push_8 (u8int_t val)
{
  
}

static void stack_pop_16 (void)
{
  
}

static void stack_pop_8 (void)
{
  
}
*/

static u16int_t make_val_16 (u8int_t high, u8int_t low)
{
  u32int_t addr = high;
  addr <<= 8;
  addr |= (low & 0xff);
//       printf ("\blow = %x\t high = %x \t addr = %x", low, high, addr);
  return addr;
}

static void tm_update_flags (u8int_t s, u8int_t z, u8int_t ac, u8int_t p, u8int_t cy)
{
  switch (s)
  {
    case UPDATE_SIGN:
      if ((tm->a & 0x80) == 0x80)
	tm_set_sign ();
      else
	tm_clear_sign ();
      break;
    
    case NO_CHANGE_SIGN:
      break;
    
    case SET_SIGN:
      tm_set_sign ();
      break;
    
    case CLEAR_SIGN:
      tm_clear_sign ();
      break;
  }
  
  switch (z)
  {
    case UPDATE_ZERO:
      if (tm->a == 0)
	tm_set_zero ();
      else
	tm_clear_zero ();
      break;
    
    case NO_CHANGE_ZERO:
      break;
    
    case SET_ZERO:
      tm_set_zero ();
      break;
    
    case CLEAR_ZERO:
      tm_clear_zero ();
      break;
  }
  
  switch (ac)
  {
    case UPDATE_AUXCARRY:
      //TODO
      break;

    case NO_CHANGE_AUXCARRY:
      break;
    
    case SET_AUXCARRY:
      tm_set_auxcarry ();
      break;
    
    case CLEAR_AUXCARRY:
      tm_clear_auxcarry ();
      break;
  }
  
  switch (p)
  {
    case UPDATE_PARITY:
      {
	u8int_t temp = tm->a;
	_Bool parity = 1;
// 	  temp = a;
// 	  parity = 1;
	//using loop, leaving optimizationon compiler
	//else write 8 sparate lines each one checking for each bit 
	//for faster execution (avoid shifting), as in hardware (parallal)
	while (temp)
	{
	  if (temp & 0x01)
	    parity = !parity;
	  temp >>= 1;
	}
	parity == 1 ? tm_set_parity () : tm_clear_parity ();
      } 
      break;
    
    case NO_CHANGE_PARITY:
      break;

    case SET_PARITY:
      tm_set_parity ();
      break;
    
    case CLEAR_PARITY:
      tm_clear_parity ();
      break;
  }
  
  switch (cy)
  {
    case UPDATE_CARRY:
      if ((tm->a & 0xff00) != 0)
	tm_set_carry ();
      else
	tm_clear_carry ();
      // Clear the upper byte of a
      tm->a &= 0xff;
      break;
    
    case NO_CHANGE_CARRY:
      break;
    
    case SET_CARRY:
      tm_set_carry ();
      break;
    
    case CLEAR_CARRY:
      tm_clear_carry ();
      break;
  }
}

/* We should do all of the below functions static */
u8int_t _invalid (void);
u8int_t nop      (void);
u8int_t hlt      (void);
u8int_t mov_b_b  (void);
u8int_t mov_b_c  (void);
u8int_t mov_b_d  (void);
u8int_t mov_b_e  (void);
u8int_t mov_b_h  (void);
u8int_t mov_b_l  (void);
u8int_t mov_b_m  (void);
u8int_t mov_b_a  (void);
u8int_t mov_c_b  (void);
u8int_t mov_c_c  (void);
u8int_t mov_c_d  (void);
u8int_t mov_c_e  (void);
u8int_t mov_c_h  (void);
u8int_t mov_c_l  (void);
u8int_t mov_c_m  (void);
u8int_t mov_c_a  (void);
u8int_t mov_d_b  (void);
u8int_t mov_d_c  (void);
u8int_t mov_d_d  (void);
u8int_t mov_d_e  (void);
u8int_t mov_d_h  (void);
u8int_t mov_d_l  (void);
u8int_t mov_d_m  (void);
u8int_t mov_d_a  (void);
u8int_t mov_e_b  (void);
u8int_t mov_e_c  (void);
u8int_t mov_e_d  (void);
u8int_t mov_e_e  (void);
u8int_t mov_e_h  (void);
u8int_t mov_e_l  (void);
u8int_t mov_e_m  (void);
u8int_t mov_e_a  (void);
u8int_t mov_h_b  (void);
u8int_t mov_h_c  (void);
u8int_t mov_h_d  (void);
u8int_t mov_h_e  (void);
u8int_t mov_h_h  (void);
u8int_t mov_h_l  (void);
u8int_t mov_h_m  (void);
u8int_t mov_h_a  (void);
u8int_t mov_l_b  (void);
u8int_t mov_l_c  (void);
u8int_t mov_l_d  (void);
u8int_t mov_l_e  (void);
u8int_t mov_l_h  (void);
u8int_t mov_l_l  (void);
u8int_t mov_l_m  (void);
u8int_t mov_l_a  (void);
u8int_t mov_m_b  (void);
u8int_t mov_m_c  (void);
u8int_t mov_m_d  (void);
u8int_t mov_m_e  (void);
u8int_t mov_m_h  (void);
u8int_t mov_m_l  (void);
u8int_t mov_m_a  (void);
u8int_t mov_a_b  (void);
u8int_t mov_a_c  (void);
u8int_t mov_a_d  (void);
u8int_t mov_a_e  (void);
u8int_t mov_a_h  (void);
u8int_t mov_a_l  (void);
u8int_t mov_a_m  (void);
u8int_t mov_a_a  (void);
u8int_t _lxi_b    (void);
u8int_t lxi_b   (u16int_t val);
u8int_t _lxi_d    (void);
u8int_t lxi_d   (u16int_t val);
u8int_t _lxi_h    (void);
u8int_t lxi_h    (u16int_t val);
u8int_t _lxi_sp   (void);
u8int_t lxi_sp   (u16int_t val);
u8int_t _mvi_b    (void);
u8int_t mvi_b    (u8int_t val);
u8int_t _mvi_c    (void);
u8int_t mvi_c    (u8int_t val);
u8int_t _mvi_d    (void);
u8int_t mvi_d    (u8int_t val);
u8int_t _mvi_e    (void);
u8int_t mvi_e    (u8int_t val);
u8int_t _mvi_h    (void);
u8int_t mvi_h    (u8int_t val);
u8int_t _mvi_l    (void);
u8int_t mvi_l    (u8int_t val);
u8int_t _mvi_m    (void);
u8int_t mvi_m    (u8int_t val);
u8int_t _mvi_a    (void);
u8int_t mvi_a    (u8int_t val);
u8int_t _lda      (void);
u8int_t lda      (u16int_t addr);
u8int_t _sta      (void);
u8int_t sta      (u16int_t addr);
u8int_t stax_b   (void);
u8int_t stax_d   (void);
u8int_t ldax_b   (void);
u8int_t ldax_d   (void);
u8int_t _shld     (void);
u8int_t shld     (u16int_t val);
u8int_t _lhld     (void);
u8int_t lhld     (u16int_t val);
u8int_t sphl     (void);
u8int_t pchl     (void);
u8int_t xchg     (void);
u8int_t xthl     (void);
u8int_t add_b    (void);
u8int_t add_c    (void);
u8int_t add_d    (void);
u8int_t add_e    (void);
u8int_t add_h    (void);
u8int_t add_l    (void);
u8int_t add_m    (void);
u8int_t add_a    (void);
u8int_t adc_b    (void);
u8int_t adc_c    (void);
u8int_t adc_d    (void);
u8int_t adc_e    (void);
u8int_t adc_h    (void);
u8int_t adc_l    (void);
u8int_t adc_m    (void);
u8int_t adc_a    (void);
u8int_t _adi      (void);
u8int_t adi      (u8int_t val);
u8int_t _aci      (void);
u8int_t aci      (u8int_t val);
u8int_t sub_b    (void);
u8int_t sub_c    (void);
u8int_t sub_d    (void);
u8int_t sub_e    (void);
u8int_t sub_h    (void);
u8int_t sub_l    (void);
u8int_t sub_m    (void);
u8int_t sub_a    (void);
u8int_t sbb_b    (void);
u8int_t sbb_c    (void);
u8int_t sbb_d    (void);
u8int_t sbb_e    (void);
u8int_t sbb_h    (void);
u8int_t sbb_l    (void);
u8int_t sbb_m    (void);
u8int_t sbb_a    (void);
u8int_t _sui      (void);
u8int_t sui      (u8int_t val);
u8int_t _sbi      (void);
u8int_t sbi      (u8int_t val);
u8int_t dad_b    (void);
u8int_t dad_d    (void);
u8int_t dad_h    (void);
u8int_t dad_sp   (void);
u8int_t inr_b    (void);
u8int_t inr_c    (void);
u8int_t inr_d    (void);
u8int_t inr_e    (void);
u8int_t inr_h    (void);
u8int_t inr_l    (void);
u8int_t inr_m    (void);
u8int_t inr_a    (void);
u8int_t dcr_b    (void);
u8int_t dcr_c    (void);
u8int_t dcr_d    (void);
u8int_t dcr_e    (void);
u8int_t dcr_h    (void);
u8int_t dcr_l    (void);
u8int_t dcr_m    (void);
u8int_t dcr_a    (void);
u8int_t inx_b    (void);
u8int_t inx_d    (void);
u8int_t inx_h    (void);
u8int_t inx_sp   (void);
u8int_t dcx_b    (void);
u8int_t dcx_d    (void);
u8int_t dcx_h    (void);
u8int_t dcx_sp   (void);
u8int_t daa      (void);
u8int_t ana_b    (void);
u8int_t ana_c    (void);
u8int_t ana_d    (void);
u8int_t ana_e    (void);
u8int_t ana_h    (void);
u8int_t ana_l    (void);
u8int_t ana_m    (void);
u8int_t ana_a    (void);
u8int_t _ani      (void);
u8int_t ani      (u8int_t val);
u8int_t xra_b    (void);
u8int_t xra_c    (void);
u8int_t xra_d    (void);
u8int_t xra_e    (void);
u8int_t xra_h    (void);
u8int_t xra_l    (void);
u8int_t xra_m    (void);
u8int_t xra_a    (void);
u8int_t _xri      (void);
u8int_t xri      (u8int_t val);
u8int_t ora_b    (void);
u8int_t ora_c    (void);
u8int_t ora_d    (void);
u8int_t ora_e    (void);
u8int_t ora_h    (void);
u8int_t ora_l    (void);
u8int_t ora_m    (void);
u8int_t ora_a    (void);
u8int_t cmp_b    (void);
u8int_t _cpi      (void);
u8int_t cpi      (u8int_t val);
u8int_t _ori      (void);
u8int_t ori      (u8int_t val);
u8int_t cmp_c    (void);
u8int_t cmp_d    (void);
u8int_t cmp_e    (void);
u8int_t cmp_h    (void);
u8int_t cmp_l    (void);
u8int_t cmp_m    (void);
u8int_t cmp_a    (void);
u8int_t cma      (void);
u8int_t cmc      (void);
u8int_t stc      (void);
u8int_t rlc      (void);
u8int_t rrc      (void);
u8int_t ral      (void);
u8int_t rar      (void);
u8int_t jmp      (void);
u8int_t jc       (void);
u8int_t jnc      (void);
u8int_t jp       (void);
u8int_t jm       (void);
u8int_t jpe      (void);
u8int_t jpo      (void);
u8int_t jz       (void);
u8int_t jnz      (void);
u8int_t push_b   (void);
u8int_t push_d   (void);
u8int_t push_h   (void);
u8int_t push_psw (void);
u8int_t pop_b    (void);
u8int_t pop_d    (void);
u8int_t pop_h    (void);
u8int_t pop_psw  (void);
u8int_t call     (void);
u8int_t cc       (void);
u8int_t cnc      (void);
u8int_t cp       (void);
u8int_t cm       (void);
u8int_t cpe      (void);
u8int_t cpo      (void);
u8int_t cz       (void);
u8int_t cnz      (void);
u8int_t ret      (void);
u8int_t rc       (void);
u8int_t rnc      (void);
u8int_t rp       (void);
u8int_t rm       (void);
u8int_t rpe      (void);
u8int_t rpo      (void);
u8int_t rz       (void);
u8int_t rnz      (void);
u8int_t rim      (void);
u8int_t sim      (void);
u8int_t in       (void);
u8int_t out      (void);
u8int_t di       (void);
u8int_t ei       (void);
u8int_t rst_0    (void);
u8int_t rst_1    (void);
u8int_t rst_2    (void);
u8int_t rst_3    (void);
u8int_t rst_4    (void);
u8int_t rst_5    (void);
u8int_t rst_6    (void);
u8int_t rst_7    (void);
//access function instruction set in function pointer array


/* Hex code is the index */
static struct _instruction_set instruction_set[0x100] =
{ 
  /* store the type of opnd , ie the addressing mode also, to help the assembler */
/* function  , asm_str , opnd1, opnd2, upper_nibble=1st operand size, lower_nibble=2nd operand size */
  {nop       , "nop"   ,  ""  , ""   , 0x00}, //0x00
  {_lxi_b    , "lxi"   ,  "b" , " "  , 0x02}, //0x01
  {stax_b    , "stax"  ,  "b" , ""   , 0x00}, //0x02
  {inx_b     , "inx"   ,  "b" , ""   , 0x00}, //0x03
  {inr_b     , "inr"   ,  "b" , ""   , 0x00}, //0x04
  {dcr_b     , "dcr"   ,  "b" , ""   , 0x00}, //0x05
  {_mvi_b    , "mvi"   ,  "b" , " "  , 0x01}, //0x06
  {rlc       , "rlc"   ,  ""  , ""   , 0x00}, //0x07
  {_invalid  , ""      ,  "x" , "x"  , 0xff}, //0x08
  {dad_b     , "dad"   ,  "b" , ""   , 0x00}, //0x09
  {ldax_b    , "ldax"  ,  "b" , ""   , 0x00}, //0x0a
  {dcx_b     , "dcx"   ,  "b" , ""   , 0x00}, //0x0b
  {inr_c     , "inr"   ,  "c" , ""   , 0x00}, //0x0c
  {dcr_c     , "dcr"   ,  "c" , ""   , 0x00}, //0x0d
  {_mvi_c    , "mvi"   ,  "c" , " "  , 0x01}, //0x0e
  {rrc       , "rrc"   ,  ""  , ""   , 0x00}, //0x0f
  {_invalid  , ""      ,  "x" , "x"  , 0xff}, //0x10
  {_lxi_d    , "lxi"   , "d"  , " "  , 0x02}, //0x11
  {stax_d    , "stax"  , "d"  , ""   , 0x00}, //0x12
  {inx_d     , "inx"   , "d"  , ""   , 0x00}, //0x13
  {inr_d     , "inr"   , "d"  , ""   , 0x00}, //0x14
  {dcr_d     , "dcr"   , "d"  , ""   , 0x00}, //0x15
  {_mvi_d    , "mvi"   , "d"  , " "  , 0x01}, //0x16
  {ral       , "ral"   , ""   , ""   , 0x00}, //0x17
  {_invalid  , ""      , "x"  , "x"  , 0xff}, //0x18
  {dad_d     , "dad"   , "d"  , ""   , 0x00}, //0x19
  {ldax_d    , "ldax"  , "d"  , ""   , 0x00}, //0x1a
  {dcx_d     , "dcx"   , "d"  , ""   , 0x00}, //0x1b
  {inr_e     , "inr"   , "e"  , ""   , 0x00}, //0x1c 
  {dcr_e     , "dcr"   , "e"  , ""   , 0x00}, //0x1d
  {_mvi_e    , "mvi"   , "e"  , " "  , 0x01}, //0x1e
  {rar       , "rar"   , ""   , ""   , 0x00}, //0x1f
  {rim       , "rim"   , ""   , ""   , 0x00}, //0x20
  {_lxi_h    , "lxi"   , "h"  , " "  , 0x02}, //0x21 
  {_shld     , "shld"  , ""   , " "  , 0x20}, //0x22
  {inx_h     , "inx"   , "h"  , ""   , 0x00}, //0x23
  {inr_h     , "inr"   , "h"  , ""   , 0x00}, //0x24
  {dcr_h     , "dcr"   , "h"  , ""   , 0x00}, //0x25
  {_mvi_h    , "mvi"   , "h"  , " "  , 0x01}, //0x26
  {daa       , "daa"   , ""   , ""   , 0x00}, //0x27
  {_invalid  , ""      , "x"  , "x"  , 0xff}, //0x28
  {dad_h     , "dad"   , "h"  , ""   , 0x00}, //0x29
  {_lhld     , "lhld"  , ""   , " "  , 0x20}, //0x2a
  {dcx_h     , "dcx"   , "h"  , ""   , 0x00}, //0x2b
  {inr_l     , "inr"   , "l"  , ""   , 0x00}, //0x2c
  {dcr_l     , "dcr"   , "l"  , ""   , 0x00}, //0x2d
  {_mvi_l    , "mvi"   , "l"  , " "  , 0x01}, //0x2e
  {cma       , "cma"   , ""   , ""   , 0x00}, //0x2f
  {sim       , "sim"   , ""   , ""   , 0x00}, //0x30
  {_lxi_sp   , "lxi"   , "sp" , " "  , 0x02}, //0x31
  {_sta      , "sta"   , ""   , " "  , 0x20}, //0x32
  {inx_sp    , "inx"   , "sp" , ""   , 0x00}, //0x33
  {inr_m     , "inr"   , "m"  , ""   , 0x00}, //0x34
  {dcr_m     , "dcr"   , "m"  , ""   , 0x00}, //0x35
  {_mvi_m    , "mvi"   , "m"  , " "  , 0x01}, //0x36
  {stc       , "stc"   , ""   , ""   , 0x00}, //0x37
  {_invalid  , ""      , "x"  , "x"  , 0xff}, //0x38
  {dad_sp    , "dad"   , "sp" , ""   , 0x00}, //0x39
  {_lda      , "lda"   , " "  , ""   , 0x20}, //0x3a
  {dcx_sp    , "dcx"   , "sp" , ""   , 0x00}, //0x3b
  {inr_a     , "inr"   , "a"  , ""   , 0x00}, //0x3c
  {dcr_a     , "dcr"   , "a"  , ""   , 0x00}, //0x3d
  {_mvi_a    , "mvi"   , "a"  , " "  , 0x01}, //0x3e
  {cmc       , "cmc"   , ""   , ""   , 0x00}, //0x3f
  {mov_b_b   , "mov"   , "b"  , "b"  , 0x00}, //0x40
  {mov_b_c   , "mov"   , "b"  , "c"  , 0x00}, //0x41
  {mov_b_d   , "mov"   , "b"  , "d"  , 0x00}, //0x42
  {mov_b_e   , "mov"   , "b"  , "e"  , 0x00}, //0x43
  {mov_b_h   , "mov"   , "b"  , "h"  , 0x00}, //0x44
  {mov_b_l   , "mov"   , "b"  , "l"  , 0x00}, //0x45
  {mov_b_m   , "mov"   , "b"  , "m"  , 0x00}, //0x46
  {mov_b_a   , "mov"   , "b"  , "a"  , 0x00}, //0x47
  {mov_c_b   , "mov"   , "c"  , "b"  , 0x00}, //0x48
  {mov_c_c   , "mov"   , "c"  , "c"  , 0x00}, //0x49
  {mov_c_d   , "mov"   , "c"  , "d"  , 0x00}, //0x4a
  {mov_c_e   , "mov"   , "c"  , "e"  , 0x00}, //0x4b
  {mov_c_h   , "mov"   , "c"  , "h"  , 0x00}, //0x4c
  {mov_c_l   , "mov"   , "c"  , "l"  , 0x00}, //0x4d
  {mov_c_m   , "mov"   , "c"  , "m"  , 0x00}, //0x4e
  {mov_c_a   , "mov"   , "c"  , "a"  , 0x00}, //0x4f
  {mov_d_b   , "mov"   , "d"  , "b"  , 0x00}, //0x50
  {mov_d_c   , "mov"   , "d"  , "c"  , 0x00}, //0x51
  {mov_d_d   , "mov"   , "d"  , "d"  , 0x00}, //0x52
  {mov_d_e   , "mov"   , "d"  , "e"  , 0x00}, //0x53
  {mov_d_h   , "mov"   , "d"  , "h"  , 0x00}, //0x54
  {mov_d_l   , "mov"   , "d"  , "l"  , 0x00}, //0x55
  {mov_d_m   , "mov"   , "d"  , "m"  , 0x00}, //0x56
  {mov_d_a   , "mov"   , "d"  , "a"  , 0x00}, //0x57
  {mov_e_b   , "mov"   , "e"  , "b"  , 0x00}, //0x58
  {mov_e_c   , "mov"   , "e"  , "c"  , 0x00}, //0x59
  {mov_e_d   , "mov"   , "e"  , "d"  , 0x00}, //0x5a
  {mov_e_e   , "mov"   , "e"  , "e"  , 0x00}, //0x5b
  {mov_e_h   , "mov"   , "e"  , "h"  , 0x00}, //0x5c
  {mov_e_l   , "mov"   , "e"  , "l"  , 0x00}, //0x5d
  {mov_e_m   , "mov"   , "e"  , "m"  , 0x00}, //0x5e
  {mov_e_a   , "mov"   , "e"  , "a"  , 0x00}, //0x5f
  {mov_h_b   , "mov"   , "h"  , "b"  , 0x00}, //0x60
  {mov_h_c   , "mov"   , "h"  , "c"  , 0x00}, //0x61
  {mov_h_d   , "mov"   , "h"  , "d"  , 0x00}, //0x62
  {mov_h_e   , "mov"   , "h"  , "e"  , 0x00}, //0x63
  {mov_h_h   , "mov"   , "h"  , "h"  , 0x00}, //0x64
  {mov_h_l   , "mov"   , "h"  , "l"  , 0x00}, //0x65
  {mov_h_m   , "mov"   , "h"  , "m"  , 0x00}, //0x66
  {mov_h_a   , "mov"   , "h"  , "a"  , 0x00}, //0x67
  {mov_l_b   , "mov"   , "l"  , "b"  , 0x00}, //0x68
  {mov_l_c   , "mov"   , "l"  , "c"  , 0x00}, //0x69
  {mov_l_d   , "mov"   , "l"  , "d"  , 0x00}, //0x6a
  {mov_l_e   , "mov"   , "l"  , "e"  , 0x00}, //0x6b
  {mov_l_h   , "mov"   , "l"  , "h"  , 0x00}, //0x6c
  {mov_l_l   , "mov"   , "l"  , "l"  , 0x00}, //0x6d
  {mov_l_m   , "mov"   , "l"  , "m"  , 0x00}, //0x6e
  {mov_l_a   , "mov"   , "l"  , "a"  , 0x00}, //0x6f
  {mov_m_b   , "mov"   , "m"  , "b"  , 0x00}, //0x70
  {mov_m_c   , "mov"   , "m"  , "c"  , 0x00}, //0x71
  {mov_m_d   , "mov"   , "m"  , "d"  , 0x00}, //0x72
  {mov_m_e   , "mov"   , "m"  , "e"  , 0x00}, //0x73
  {mov_m_h   , "mov"   , "m"  , "h"  , 0x00}, //0x74
  {mov_m_l   , "mov"   , "m"  , "l"  , 0x00}, //0x75
  {hlt       , "hlt"   , ""   , ""   , 0x00}, //0x76
  {mov_m_a   , "mov"   , "m"  , "a"  , 0x00}, //0x77
  {mov_a_b   , "mov"   , "a"  , "b"  , 0x00}, //0x78
  {mov_a_c   , "mov"   , "a"  , "c"  , 0x00}, //0x79
  {mov_a_d   , "mov"   , "a"  , "d"  , 0x00}, //0x7a
  {mov_a_e   , "mov"   , "a"  , "e"  , 0x00}, //0x7b
  {mov_a_h   , "mov"   , "a"  , "h"  , 0x00}, //0x7c
  {mov_a_l   , "mov"   , "a"  , "l"  , 0x00}, //0x7d
  {mov_a_m   , "mov"   , "a"  , "m"  , 0x00}, //0x7e
  {mov_a_a   , "mov"   , "a"  , "a"  , 0x00}, //0x7f
  {add_b     , "add"   , "b"  , ""   , 0x00}, //0x80
  {add_c     , "add"   , "c"  , ""   , 0x00}, //0x81
  {add_d     , "add"   , "d"  , ""   , 0x00}, //0x82
  {add_e     , "add"   , "e"  , ""   , 0x00}, //0x83
  {add_h     , "add"   , "h"  , ""   , 0x00}, //0x84
  {add_l     , "add"   , "l"  , ""   , 0x00}, //0x85
  {add_m     , "add"   , "m"  , ""   , 0x00}, //0x86
  {add_a     , "add"   , "a"  , ""   , 0x00}, //0x87
  {adc_b     , "adc"   , "b"  , ""   , 0x00}, //0x88
  {adc_c     , "adc"   , "c"  , ""   , 0x00}, //0x89
  {adc_d     , "adc"   , "d"  , ""   , 0x00}, //0x8a
  {adc_e     , "adc"   , "e"  , ""   , 0x00}, //0x8b
  {adc_h     , "adc"   , "h"  , ""   , 0x00}, //0x8c
  {adc_l     , "adc"   , "l"  , ""   , 0x00}, //0x8d
  {adc_m     , "adc"   , "m"  , ""   , 0x00}, //0x8e
  {adc_m     , "adc"   , "a"  , ""   , 0x00}, //0x8f
  {sub_b     , "sub"   , "b"  , ""   , 0x00}, //0x90
  {sub_c     , "sub"   , "c"  , ""   , 0x00}, //0x91
  {sub_d     , "sub"   , "d"  , ""   , 0x00}, //0x92
  {sub_e     , "sub"   , "e"  , ""   , 0x00}, //0x93
  {sub_h     , "sub"   , "h"  , ""   , 0x00}, //0x94
  {sub_l     , "sub"   , "l"  , ""   , 0x00}, //0x95
  {sub_m     , "sub"   , "m"  , ""   , 0x00}, //0x96
  {sub_a     , "sub"   , "a"  , ""   , 0x00}, //0x97
  {sbb_b     , "sbb"   , "b"  , ""   , 0x00}, //0x98
  {sbb_c     , "sbb"   , "c"  , ""   , 0x00}, //0x99
  {sbb_d     , "sbb"   , "d"  , ""   , 0x00}, //0x9a
  {sbb_e     , "sbb"   , "e"  , ""   , 0x00}, //0x9b
  {sbb_h     , "sbb"   , "h"  , ""   , 0x00}, //0x9c
  {sbb_l     , "sbb"   , "l"  , ""   , 0x00}, //0x9d
  {sbb_m     , "sbb"   , "m"  , ""   , 0x00}, //0x9e
  {sbb_a     , "sbb"   , "a"  , ""   , 0x00}, //0x9f
  {ana_b     , "ana"   , "b"  , ""   , 0x00}, //0xa0
  {ana_c     , "ana"   , "c"  , ""   , 0x00}, //0xa1
  {ana_d     , "ana"   , "d"  , ""   , 0x00}, //0xa2
  {ana_e     , "ana"   , "e"  , ""   , 0x00}, //0xa3
  {ana_h     , "ana"   , "h"  , ""   , 0x00}, //0xa4
  {ana_l     , "ana"   , "l"  , ""   , 0x00}, //0xa5
  {ana_m     , "ana"   , "m"  , ""   , 0x00}, //0xa6
  {ana_a     , "ana"   , "a"  , ""   , 0x00}, //0xa7
  {xra_b     , "xra"   , "b"  , ""   , 0x00}, //0xa8
  {xra_c     , "xra"   , "c"  , ""   , 0x00}, //0xa9
  {xra_d     , "xra"   , "d"  , ""   , 0x00}, //0xaa
  {xra_e     , "xra"   , "e"  , ""   , 0x00}, //0xab
  {xra_h     , "xra"   , "h"  , ""   , 0x00}, //0xac
  {xra_l     , "xra"   , "l"  , ""   , 0x00}, //0xad
  {xra_m     , "xra"   , "m"  , ""   , 0x00}, //0xae
  {xra_a     , "xra"   , "a"  , ""   , 0x00}, //0xaf
  {ora_b     , "ora"   , "b"  , ""   , 0x00}, //0xb0
  {ora_c     , "ora"   , "c"  , ""   , 0x00}, //0xb1
  {ora_d     , "ora"   , "d"  , ""   , 0x00}, //0xb2
  {ora_e     , "ora"   , "e"  , ""   , 0x00}, //0xb3
  {ora_h     , "ora"   , "h"  , ""   , 0x00}, //0xb4
  {ora_l     , "ora"   , "l"  , ""   , 0x00}, //0xb5
  {ora_m     , "ora"   , "m"  , ""   , 0x00}, //0xb6
  {ora_a     , "ora"   , "a"  , ""   , 0x00}, //0xb7
  {cmp_b     , "cmp"   , "b"  , ""   , 0x00}, //0xb8
  {cmp_c     , "cmp"   , "c"  , ""   , 0x00}, //0xb9
  {cmp_d     , "cmp"   , "d"  , ""   , 0x00}, //0xba
  {cmp_e     , "cmp"   , "e"  , ""   , 0x00}, //0xbb
  {cmp_h     , "cmp"   , "h"  , ""   , 0x00}, //0xbc
  {cmp_l     , "cmp"   , "l"  , ""   , 0x00}, //0xbd
  {cmp_m     , "cmp"   , "m"  , ""   , 0x00}, //0xbe
  {cmp_a     , "cmp"   , "a"  , ""   , 0x00}, //0xbf
  {rnz       , "rnz"   , ""   , ""   , 0x00}, //0xc0
  {pop_b     , "pop"   , "b"  , ""   , 0x00}, //0xc1
  {jnz       , "jnz"   , " "  , ""   , 0x20}, //0xc2
  {jmp       , "jmp"   , ""   , ""   , 0x20}, //0xc3
  {cnz       , "cnz"   , ""   , ""   , 0x20}, //0xc4
  {push_b    , "push"  , "b"  , ""   , 0x00}, //0xc5
  {_adi      , "adi"   , ""   , " "  , 0x10}, //0xc6
  {rst_0     , "rst"   , "0"  , ""   , 0x00}, //0xc7
  {rz        , "rz"    , " "  , ""   , 0x00}, //0xc8
  {ret       , "ret"   , ""   , ""   , 0x00}, //0xc9
  {jz        , "jz"    , ""   , ""   , 0x20}, //0xca
  {_invalid  , ""      , "x"  , "x"  , 0xff}, //0xcb
  {cz        , "cz"    , ""   , ""   , 0x20}, //0xcc
  {call      , "call"  , ""   , ""   , 0x20}, //0xcd
  {_aci      , "aci"   , ""   , " "  , 0x10}, //0xce
  {rst_1     , "rst"   , "1"  , ""   , 0x00}, //0xcf
  {rnc       , "rnc"   , ""   , ""   , 0x00}, //0xd0
  {pop_d     , "pop"   , "d"  , ""   , 0x00}, //0xd1
  {jnc       , "jnc"   , ""   , ""   , 0x20}, //0xd2
  {out       , "out"   , ""   , ""   , 0x10}, //0xd3
  {cnc       , "cnc"   , ""   , ""   , 0x20}, //0xd4
  {push_d    , "push"  , "d"  , ""   , 0x00}, //0xd5
  {_sui      , "sui"   , ""   , " "  , 0x20}, //0xd6
  {rst_2     , "rst"   , "2"  , ""   , 0x00}, //0xd7
  {rc        , "rc"    , ""   , ""   , 0x00}, //0xd8
  {_invalid  , ""      , "x"  , "x"  , 0xff}, //0xd9
  {jc        , "jc"    , " "  , ""   , 0x20}, //0xda
  {in        , "in"    , ""   , ""   , 0x10}, //0xdb
  {cc        , "cc"    , " "  , ""   , 0x20}, //0xdc
  {_invalid  , ""      , "x"  , "x"  , 0xff}, //0xdd
  {_sbi      , "sbi"   , ""   , " "  , 0x10}, //0xde
  {rst_3     , "rst"   , "3"  , ""   , 0x00}, //0xdf
  {rpo       , "rpo"   , ""   , ""   , 0x00}, //0xe0
  {pop_h     , "pop"   , "h"  , ""   , 0x00}, //0xe1
  {jpo       , "jpo"   , ""   , ""   , 0x20}, //0xe2
  {xthl      , "xthl"  , ""   , ""   , 0x00}, //0xe3
  {cpo       , "cpo"   , ""   , ""   , 0x20}, //0xe4
  {push_h    , "push"  , "h"  , ""   , 0x00}, //0xe5
  {_ani      , "ani"   , " "  , ""   , 0x10}, //0xe6
  {rst_4     , "rst"   , "4"  , ""   , 0x00}, //0xe7
  {rpe       , "rpe"   , ""   , ""   , 0x00}, //0xe8
  {pchl      , "pchl"  , ""   , ""   , 0x00}, //0xe9
  {jpe       , "jpe"   , ""   , ""   , 0x20}, //0xea
  {xchg      , "xchg"  , ""   , ""   , 0x00}, //0xeb
  {cpe       , "cpe"   , ""   , ""   , 0x20}, //0xec
  {_invalid  , ""      , "x"  , "x"  , 0xff}, //0xed
  {_xri      , "xri"   , ""   , " "  , 0x10}, //0xee
  {rst_5     , "rst"   , "5"  , ""   , 0x00}, //0xef
  {rp        , "rp"    , ""   , ""   , 0x00}, //0xf0
  {pop_psw   , "pop"   , "psw", ""   , 0x00}, //0xf1
  {jp        , "jp"    , ""   , ""   , 0x20}, //0xf2
  {di        , "di"    , ""   , ""   , 0x00}, //0xf3
  {cp        , "cp"    , ""   , ""   , 0x20}, //0xf4
  {push_psw  , "push"  , "psw", ""   , 0x00}, //0xf5
  {_ori      , "ori"   , ""   , " "  , 0x10}, //0xf6
  {rst_6     , "rst"   , "6"  , ""   , 0x00}, //0xf7
  {rm        , "rm"    , ""   , ""   , 0x00}, //0xf8
  {sphl      , "sphl"  , ""   , ""   , 0x00}, //0xf9
  {jm        , "jm"    , " "  , ""   , 0x20}, //0xfa
  {ei        , "ei"    , ""   , ""   , 0x00}, //0xfb
  {cm        , "cm"    , ""   , ""   , 0x20}, //0xfc
  {_invalid  , ""      , "x"  , "x"  , 0xff}, //0xfd
  {_cpi      , "cpi"   , ""   , " "  , 0x10}, //0xfe
  {rst_7     , "rst"   , "7"  , ""   , 0x00}, //0xff
}; 


/* Instead of simulating overloading of the below functions, do two vartients like:
 * get_current_opcode_str  and get_opcode_str ()  , in the 2nd version a parameter
 * this_one would be required. bette
 */

char *tm_get_opcode_str (void)
{
  if (tm->current_instruction != NULL)
    return tm->current_instruction->ins_str;
  else
    return " ";
}

u8int_t tm_get_opcode_hex (void)   
{
  if (tm->current_instruction != NULL)
    return (tm->current_instruction - instruction_set);
  return 0x00;
}

char *tm_get_operand_1 (void)   
{
  if (tm->current_instruction != NULL)
    return tm->current_instruction->operand_1;
  else
    return " ";
}

char *tm_get_operand_2 (void) 
{
  if (tm->current_instruction != NULL)
    return tm->current_instruction->operand_2;
  else
    return " ";
}

/* TODO: implement memory bounds check in these functions, not needed unsigned type automatically wraps around */
static u8int_t tm_fetch_operand (void) 
{
    return tm->memory[tm->pc++];
}

/* fetch the instruction set array location pointer, also return it */
/* NOTE: this fetches the memory operands and makes the string, this is useless for the execution function is no 
 * feedback is used
 */
static struct _instruction_set *tm_fetch_instruction (void) 
{
  u16int_t program_counter;
  u8int_t opcode;
  u16int_t mem_operand_low, mem_operand_hi;
  u8int_t mem_operand_size;
  
  program_counter = tm->pc;
  opcode = tm->memory[program_counter++];
  tm->current_instruction = &instruction_set[opcode];
  
  /* First operand number of bytes in upper nibble */
  mem_operand_size = (instruction_set[opcode].mem_opnd & 0xf0) >> 4;
  if (mem_operand_size == 1)
  {
    mem_operand_low = tm_fetch_memory (program_counter);
    program_counter++;
    sprintf (instruction_set[opcode].operand_1, "%02xh", mem_operand_low);
  }
  else if (mem_operand_size == 2)
  {
    mem_operand_low = tm_fetch_memory (program_counter);
    program_counter++;
    mem_operand_hi = tm_fetch_memory (program_counter);
    program_counter++;
    sprintf (instruction_set[opcode].operand_1, "%04xh", make_val_16 (mem_operand_hi, mem_operand_low));
  }
  
  /* Second operand number of bytes in lower nibble */
  mem_operand_size = (instruction_set[opcode].mem_opnd & 0x0f);
  if (mem_operand_size == 1)
  {
    mem_operand_low = tm_fetch_memory (program_counter);
    program_counter++;
    sprintf (instruction_set[opcode].operand_2, "%02xh", mem_operand_low);
  }
  else if (mem_operand_size == 2)
  {
    mem_operand_low = tm_fetch_memory (program_counter);
    program_counter++;
    mem_operand_hi = tm_fetch_memory (program_counter);
    program_counter++;
    sprintf (instruction_set[opcode].operand_2, "%04xh", make_val_16 (mem_operand_hi, mem_operand_low));
  }
  
//   printf ("\n\npc = %x , memory[%x] = %x, ins = %s\n\n", pc-1, pc-1, memory[pc-1], tm->current_instruction->ins_str);
  return tm->current_instruction; 
}

/* Execute the current opcode */
static u8int_t tm_execute_instruction (void)
{ 
  if (tm->current_instruction != NULL)
  {
    /* Increment pc to 1 for the instruciton. The memory operand increment updates are still the responsibility
     * of the instruction emulating function, when it uses the fetch_operand function. Unfortunately when step
     * executing this will twice fetch the memory operands, one in the fetch instruction and another during when
     * the emulation funciton is executed
     */
    tm->prev_pc = tm->pc++;
//     tm->pc = tm->pc + (tm->current_instruction->mem_opnd & 0x0f) + ((tm->current_instruction->mem_opnd >> 4) & 0x0f);
    return tm->current_instruction->ins ();
  }
  else
    return 0;
}

static void tm_step_over_instruction (void)
{
  if (tm->current_instruction != NULL)
  {
    /* Increment pc to 1 for the instruciton and increment proper number of bytes depending on the number of
     * memory operands. Do not execute.
     */
    tm->prev_pc = tm->pc++;
    tm->pc = tm->pc + (tm->current_instruction->mem_opnd & 0x0f) + ((tm->current_instruction->mem_opnd >> 4) & 0x0f);
  }
}

u8int_t tm_fetch_memory (u16int_t loc) 
{ 
  return tm->memory[loc];
}

void tm_set_memory (u16int_t loc, u8int_t val)
{
    tm->memory[loc] = val;
//     printf ("\n\tmemory[%x] = %x  (%x)\n", loc, memory[loc], val); 
}

/* NOTE: Only get funcitons, to be used ouside the file (not static), setting or getting
 * inside this file is dome directly
 */
u8int_t  tm_get_reg_b  (void) { return  tm->b; }
u8int_t  tm_get_reg_c  (void) { return  tm->c; }
u8int_t  tm_get_reg_d  (void) { return  tm->d; }
u8int_t  tm_get_reg_e  (void) { return  tm->e; }
u8int_t  tm_get_reg_h  (void) { return  tm->h; }
u8int_t  tm_get_reg_l  (void) { return  tm->l; }
u8int_t  tm_get_reg_a  (void) { return  tm->a; }
u16int_t tm_get_reg_sp (void) { return tm->sp; }
u16int_t tm_get_reg_pc (void) { return tm->pc; }

u16int_t tm_get_reg_prev_pc (void) { return tm->prev_pc; }

void tm_set_start_address (u16int_t addr) { tm->pc = addr; }

void tm_clear_flags (void)
{
  tm->flag = 0;
}

void tm_clear_regs (void)
{
  tm->b = 0;
  tm->c = 0;
  tm->d = 0;
  tm->e = 0;
  tm->h = 0;
  tm->l = 0;
  tm->a = 0;
  tm->pc = 0;
  tm->sp = 0;
}

//TEST  Functions like show flags etc, have to make these also in the GUI

// 0x08, 0x10, 0x18, 0x28, 0x38, 0xcb, 0xd9, 0xdd, 0xed, 0xfd
u8int_t _invalid (void)
{
  return EXECUTE_ERR;
}

//0x00
u8int_t nop (void)
{
  tm->a = tm->a & tm->a;
  return 0;
  // No operation
}

// 0x76
u8int_t hlt (void)
{
//   infinite loop here, probably later accept interrupts, or  wait for reset
//   we can simulate these with posix signals?
//   TODO
  return EXECUTE_END;
}

// 0x40
u8int_t mov_b_b (void)
{
  tm->b = tm->b;
  return 0;
}

// 0x41
u8int_t mov_b_c (void)
{
  tm->b = tm->c;
  return 0;
}

// 0x42
u8int_t mov_b_d (void)
{
  tm->b = tm->d;
  return 0;
}

// 0x43
u8int_t mov_b_e (void)
{
  tm->b = tm->e;
  return 0;
}


// 0x44
u8int_t mov_b_h (void)
{
  tm->b = tm->h;
  return 0;
}


// 0x45
u8int_t mov_b_l (void)
{
  tm->b = tm->l;
  return 0;
}


// 0x46
u8int_t mov_b_m (void)
{
  tm->b = tm->memory[tm_get_hl ()];
  return 0;
}


// 0x47
u8int_t mov_b_a (void)
{
  tm->b = tm->a;
  return 0;
}


// 0x48
u8int_t mov_c_b (void)
{
  tm->c = tm->b;
  return 0;
}


// 0x49
u8int_t mov_c_c (void)
{
  tm->c = tm->c;
  return 0;
}


// 0x4a
u8int_t mov_c_d (void)
{
  tm->c = tm->d;
  return 0;
}


// 0x4b
u8int_t mov_c_e (void)
{
  tm->c = tm->e;
  return 0;
}


// 0x4c
u8int_t mov_c_h (void)
{
  tm->c = tm->h;
  return 0;
}


// 0x4d
u8int_t mov_c_l (void)
{
  tm->c = tm->l;
  return 0;
}


// 0x4e
u8int_t mov_c_m (void)
{
  tm->c = tm->memory[tm_get_hl ()];
  return 0;
}


// 0x4f
u8int_t mov_c_a (void)
{
  tm->c = tm->a;
  return 0;
}


// 0x50
u8int_t mov_d_b (void)
{
  tm->d = tm->b;
  return 0;
}

// 0x51
u8int_t mov_d_c (void)
{
  tm->d = tm->c;
  return 0;
}

// 0x52
u8int_t mov_d_d (void)
{
  tm->d = tm->d;
  return 0;
}

// 0x53
u8int_t mov_d_e (void)
{
  tm->d = tm->e;
  return 0;
}

// 0x54
u8int_t mov_d_h (void)
{
  tm->d = tm->h;
  return 0;
}

// 0x55
u8int_t mov_d_l (void)
{
  tm->d = tm->l;
  return 0;
}

// 0x56
u8int_t mov_d_m (void)
{
  tm->d = tm->memory[tm_get_hl ()];
  return 0;
}

// 0x57
u8int_t mov_d_a (void)
{
  tm->d = tm->a;
  return 0;
}

// 0x58
u8int_t mov_e_b (void)
{
  tm->e = tm->b;
  return 0;
}

// 0x59
u8int_t mov_e_c (void)
{
  tm->e = tm->c;
  return 0;
}

// 0x5a
u8int_t mov_e_d (void)
{
  tm->e = tm->d;
  return 0;
}

// 0x5b
u8int_t mov_e_e (void)
{
  tm->e = tm->e;
  return 0;
}

// 0x5c
u8int_t mov_e_h (void)
{
  tm->e = tm->h;
  return 0;
}

// 0x5d
u8int_t mov_e_l (void)
{
  tm->e = tm->l;
  return 0;
}

// 0x5e
u8int_t mov_e_m (void)
{
  tm->e = tm->memory[tm_get_hl ()];
  return 0;
}

// 0x5f
u8int_t mov_e_a (void)
{
  tm->e = tm->a;
  return 0;
}

// 0x60
u8int_t mov_h_b (void)
{
  tm->h = tm->b;
  return 0;
}

// 0x61
u8int_t mov_h_c (void)
{
  tm->h = tm->c;
  return 0;
}

// 0x62
u8int_t mov_h_d (void)
{
  tm->h = tm->d;
  return 0;
}

// 0x63
u8int_t mov_h_e (void)
{
  tm->h = tm->e;
  return 0;
}

// 0x64
u8int_t mov_h_h (void)
{
  tm->h = tm->h;
  return 0;
}

// 0x65
u8int_t mov_h_l (void)
{
  tm->h = tm->l;
  return 0;
}

// 0x66
u8int_t mov_h_m (void)
{
  tm->h = tm->memory[tm_get_hl ()];
  return 0;
}

// 0x67
u8int_t mov_h_a (void)
{
  tm->h = tm->a;
  return 0;
}

// 0x68
u8int_t mov_l_b (void)
{
  tm->l = tm->b;
  return 0;
}

// 0x69
u8int_t mov_l_c (void)
{
  tm->l = tm->c;
  return 0;
}

// 0x6a
u8int_t mov_l_d (void)
{
  tm->l = tm->d;
  return 0;
}

// 0x6b
u8int_t mov_l_e (void)
{
  tm->l = tm->e;
  return 0;
}

// 0x6c
u8int_t mov_l_h (void)
{
  tm->l = tm->h;
  return 0;
}

// 0x6d
u8int_t mov_l_l (void)
{
  tm->l = tm->l;
  return 0;
}

// 0x6e
u8int_t mov_l_m (void)
{
  tm->l = tm->memory[tm_get_hl ()];
  return 0;
}

// 0x6f
u8int_t mov_l_a (void)
{
  tm->l = tm->a;
  return 0;
}

// 0x70
u8int_t mov_m_b (void)
{
  tm->memory[tm_get_hl ()] = tm->b;
  return 0;
}

// 0x71
u8int_t mov_m_c (void)
{
  tm->memory[tm_get_hl ()] = tm->c;
  return 0;
}

// 0x72
u8int_t mov_m_d (void)
{
  tm->memory[tm_get_hl ()] = tm->d;
  return 0;
}

// 0x73
u8int_t mov_m_e (void)
{
  tm->memory[tm_get_hl ()] = tm->e;
  return 0;
}

// 0x74
u8int_t mov_m_h (void)
{
  tm->memory[tm_get_hl ()] = tm->h;
  return 0;
}

// 0x75
u8int_t mov_m_l (void)
{
  tm->memory[tm_get_hl ()] = tm->l;
  return 0;
}

// 0x77
u8int_t mov_m_a (void)
{
  tm->memory[tm_get_hl ()] = tm->a;
  return 0;
}


// 0x78
u8int_t mov_a_b (void)
{
  tm->a = tm->b & 0xff;
  return 0;
}

// 0x79
u8int_t mov_a_c (void)
{
  tm->a = tm->c & 0xff;
  return 0;
}

// 0x7a
u8int_t mov_a_d (void)
{
  tm->a = tm->d & 0xff;
  return 0;
}

// 0x7b
u8int_t mov_a_e (void)
{
  tm->a = tm->e & 0xff;
  return 0;
}

// 0x7c
u8int_t mov_a_h (void)
{
  tm->a = tm->h & 0xff;
  return 0;
}

// 0x7d
u8int_t mov_a_l (void)
{
  tm->a = tm->l & 0xff;
  return 0;
}

// 0x7e
u8int_t mov_a_m (void)
{
  tm->a = tm->memory[tm_get_hl ()] & 0xff;
  return 0;
}

// 0x7f
u8int_t mov_a_a (void)
{
  tm->a = tm->a & 0xff;
  return 0;
}

//0x01
u8int_t _lxi_b (void)
{
  tm->c = tm_fetch_operand();
  tm->b = tm_fetch_operand();
//   sprintf (tm->current_instruction->operand_2, "%04xh", make_val_16 (tm->b, tm->c));
  return 2;
}

//0x01 overloaded
u8int_t lxi_b (u16int_t val)
{
  tm->c = (val & 0xff);
  tm->b = (val >> 8) & 0xff;
  return 2;
}

//0x11
u8int_t _lxi_d (void)
{
  tm->e = tm_fetch_operand();
  tm->d = tm_fetch_operand();
//   sprintf (tm->current_instruction->operand_2, "%04xh", make_val_16 (tm->d, tm->e));
  return 2;
}

//0x11 overloaded
u8int_t lxi_d (u16int_t val)
{
  tm->e = (val & 0xff);
  tm->d = ((val >> 8) & 0x0f);
  return 2;
}


//0x21
u8int_t _lxi_h (void)
{
  tm->l = tm_fetch_operand();
  tm->h = tm_fetch_operand();
//   sprintf (tm->current_instruction->operand_2, "%04xh", make_val_16 (tm->h, tm->l));
  return 2;
}

//0x21 overloaded
u8int_t lxi_h (u16int_t val)
{
  tm->l = (val & 0xff);
  tm->h = ((val >> 8) & 0xff);
  return 2;
}


//0x31
u8int_t _lxi_sp (void)
{
  u8int_t low, high;
  
  low = tm_fetch_operand();
  high = tm_fetch_operand();
  tm->sp = make_val_16 (high, low);
 
//   sprintf (tm->current_instruction->operand_2, "%04xh", tm->sp);
  return 2;
}

//0x31 overloaded
u8int_t lxi_sp (u16int_t val)
{
  tm->sp = val;
  return 2;
}


//0x06
u8int_t _mvi_b (void)
{
  tm->b = tm_fetch_operand();
//   sprintf (tm->current_instruction->operand_2, "%04xh", tm->b);
  return 1;
}

//0x06 overloaded
u8int_t mvi_b (u8int_t val)
{
  tm->b = val;
  return 1;
}

//0x0e
u8int_t _mvi_c (void)
{
  tm->c = tm_fetch_operand();
//   sprintf (tm->current_instruction->operand_2, "%04xh", tm->c);
  return 1;
}

//0x0e overloaded
u8int_t mvi_c (u8int_t val)
{
  tm->c = val;
  return 1;
}

//0x16
u8int_t _mvi_d (void)
{
  tm->d = tm_fetch_operand();
//   sprintf (tm->current_instruction->operand_2, "%04xh", tm->d);
  return 1;
}

//0x16 overloaded
u8int_t mvi_d (u8int_t val)
{
  tm->d = val;
  return 1;
}

//0x1e
u8int_t _mvi_e (void)
{
  tm->e = tm_fetch_operand();
//   sprintf (tm->current_instruction->operand_2, "%04xh", tm->e);
  return 1;
}

//0x1e overloaded
u8int_t mvi_e (u8int_t val)
{
  tm->e = val;
  return 1;
}

//0x26
u8int_t _mvi_h (void)
{
  tm->h = tm_fetch_operand();
//   sprintf (tm->current_instruction->operand_2, "%04xh", tm->h);
  return 1;
}

//0x26 overloaded
u8int_t mvi_h (u8int_t val)
{
  tm->h = val;
  return 1;
}

//0x2e
u8int_t _mvi_l (void)
{
  tm->l = tm_fetch_operand();
//   sprintf (tm->current_instruction->operand_2, "%04xh", tm->l);
  return 1;
}

//0x2e overloaded
u8int_t mvi_l (u8int_t val)
{
  tm->l = val;
  return 1;
}

//0x36
u8int_t _mvi_m (void)
{
  u8int_t val;
  val = tm_fetch_operand();
  tm->memory[tm_get_hl ()] = val;
//   sprintf (tm->current_instruction->operand_2, "%04xh", val);
  return 1;
}

//0x36 overloaded
u8int_t mvi_m (u8int_t val)
{
  tm->memory[tm_get_hl ()] = val;
  return 1;
}

//0x3e
u8int_t _mvi_a (void)
{
  tm->a = tm_fetch_operand() & 0xff;
//   sprintf (tm->current_instruction->operand_2, "%04xh", tm->a);
  return 1;
}

//0x3e overloaded
u8int_t mvi_a (u8int_t val)
{
  tm->a = val & 0xff;
  return 1;
}

//0x3a
u8int_t _lda (void)
{
  u8int_t low, high;
  u16int_t addr;
  
  low = tm_fetch_operand();
  high = tm_fetch_operand();
  addr = make_val_16 (high, low);
  tm->a = tm->memory[addr] & 0xff;
  
//   sprintf (tm->current_instruction->operand_2, "%04xh", addr);
  return 2;
}

//0x3a overloaded
u8int_t lda (u16int_t addr)
{  
  tm->a = tm->memory[addr] & 0xff;
  return 2;
}


//0x32
u8int_t _sta (void)
{
  u8int_t low, high;
  u16int_t addr;
  
  low = tm_fetch_operand();
  high = tm_fetch_operand();
  addr  = make_val_16 (high, low);
  tm->memory[addr] = tm->a;
  
//   sprintf (tm->current_instruction->operand_2, "%04xh", addr);
  return 2;
}

//0x32 overloaded
u8int_t sta (u16int_t addr)
{ 
  tm->memory[addr] = tm->a;
  return 2;
}

//0x02
u8int_t stax_b (void)
{
  tm->memory[tm_get_bc ()] = tm->a;
  return 0;
}

//0x12
u8int_t stax_d (void)
{
  tm->memory[tm_get_de ()] = tm->a;
  return 0;
}

//0x0a
u8int_t ldax_b (void)
{
  tm->a = tm->memory[tm_get_bc ()] & 0xff;
  return 0;
}

//0x1a
u8int_t ldax_d (void)
{
  tm->a = tm->memory[tm_get_de ()] & 0xff;
  return 0;
}

//0x22
u8int_t _shld (void)
{
  u8int_t low, high;
  u16int_t addr;
  
  low = tm_fetch_operand();
  high = tm_fetch_operand();
  addr = make_val_16 (high, low);
  
  tm->memory[addr] = tm->l;
  tm->memory[addr + 1] = tm->h;
  
//   sprintf (tm->current_instruction->operand_2, "%04xh", addr);
  return 2;
}

//0x22 overloaded
u8int_t shld (u16int_t addr)
{
  tm->memory[addr] = tm->l;
  tm->memory[addr + 1] = tm->h;
  return 2;
}


//0x2a
u8int_t _lhld (void)
{
  u8int_t low, high;
  u16int_t addr;
  
  low = tm_fetch_operand();
  high = tm_fetch_operand();
  addr = make_val_16 (high, low);
  
  tm->l = tm->memory[addr];
  tm->h = tm->memory[addr + 1];
  
//   sprintf (tm->current_instruction->operand_2, "%04xh", addr);
  return 2;
}
 
//0x2a overloaded
u8int_t lhld (u16int_t addr)
{
  tm->l = tm->memory[addr];
  tm->h = tm->memory[addr + 1];
  return 2;
}

//0xf9
u8int_t sphl (void)
{
  tm->sp = tm_get_hl ();
  return 0;
}

//0xe9
u8int_t pchl (void)
{
  tm->pc = tm_get_hl ();
  return 0;
}

//0xeb
u8int_t xchg (void)
{
  u8int_t low_t, high_t;
  
  low_t = tm->l;
  tm->l =tm-> e;
  tm->e = low_t;
  
  high_t = tm->h;
  tm->h = tm->d;
  tm->d = high_t;
  return 0;
}

//FIXME: please check this if okay. why not use a better stack abstraction ?
//0xe3
u8int_t xthl (void)
{
  u8int_t low_t, high_t;
  
  low_t = tm->l;
  tm->l = tm->memory[tm->sp];
  tm->memory[tm->sp] = low_t;
  
  high_t =tm-> h;
  tm->h = tm->memory[tm->sp + 1];
  tm->memory[tm->sp + 1] = high_t;
  return 0;
}

// 0x80
u8int_t add_b (void)
{
  tm->a = (tm->a & 0xff) + tm->b;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

// 0x81
u8int_t add_c (void)
{
  tm->a = (tm->a & 0xff) + tm->c;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

// 0x82
u8int_t add_d (void)
{
  tm->a = (tm->a & 0xff) + tm->d;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

// 0x83
u8int_t add_e (void)
{
  tm->a = (tm->a & 0xff) + tm->e;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

// 0x84
u8int_t add_h (void)
{
  tm->a = (tm->a & 0xff)+ tm->h;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

// 0x85
u8int_t add_l (void)
{
  tm->a = (tm->a & 0xff) + tm->l;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

// 0x86
u8int_t add_m (void)
{
  tm->a = (tm->a & 0xff) + tm->memory[tm_get_hl ()];
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

// 0x87
u8int_t add_a (void)
{
  tm->a = (tm->a & 0xff) + tm->a;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

// 0x88
u8int_t adc_b (void)
{
  tm->a = (tm->a & 0xff) + tm->b + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}


// 0x89
u8int_t adc_c (void)
{
  tm->a = (tm->a & 0xff) + tm->c + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}


// 0x8a
u8int_t adc_d (void)
{
  tm->a = (tm->a & 0xff) + tm->d + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

// 0x8b
u8int_t adc_e (void)
{
  tm->a = (tm->a & 0xff) + tm->e + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}


// 0x8c
u8int_t adc_h (void)
{
  tm->a = (tm->a & 0xff) + tm->h + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}


// 0x8d
u8int_t adc_l (void)
{
  tm->a = (tm->a & 0xff) + tm->l + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}


// 0x8e
u8int_t adc_m (void)
{
  tm->a = (tm->a & 0xff) + tm->memory[tm_get_hl ()] + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}


// 0x8f
u8int_t adc_a (void)
{
  tm->a = (tm->a & 0xff) + tm->a + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 0;
}

//0xc6
u8int_t _adi (void)
{
  u8int_t operand;

  operand = tm_fetch_operand();

  tm->a = (tm->a & 0xff) + operand;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  
  return 1;
}

//0xc6 overloaded
u8int_t adi (u8int_t val)
{
  tm->a = (tm->a & 0xff) + val;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 1;
}

//0xce
u8int_t _aci (void)
{
  u8int_t operand;
  
  operand = tm_fetch_operand();
  tm->a = (tm->a & 0xff) + operand + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
//   sprintf (tm->current_instruction->operand_2, "%04xh", operand);
  return 1;
}

//0xce overloaded
u8int_t aci (u8int_t val)
{
  tm->a = (tm->a & 0xff) + val + tm_get_carry ();
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  return 1;
}

// 0x90
u8int_t sub_b (void)
{
  tm->a = (tm->a & 0xff) + ((~tm->b + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

// 0x91
u8int_t sub_c (void)
{
  tm->a = (tm->a & 0xff) + ((~tm->c + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

// 0x92
u8int_t sub_d (void)
{
  tm->a = (tm->a & 0xff) + ((~tm->d + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

// 0x93
u8int_t sub_e (void)
{
  tm->a = (tm->a & 0xff) + ((~tm->e + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

// 0x94
u8int_t sub_h (void)
{
  tm->a = (tm->a & 0xff) + ((~tm->h + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

// 0x95
u8int_t sub_l (void)
{
  tm->a = (tm->a & 0xff) + ((~tm->l + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

// 0x96
u8int_t sub_m (void)
{
  tm->a = (tm->a & 0xff) + ((~tm->memory[tm_get_hl ()] + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

// 0x97
u8int_t sub_a (void)
{
  tm->a = (tm->a & 0xff) + ((~tm->a + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}


//FIXME: what to do if overflow occurs when adding (b+1) borrow ro reg ?

//0x98
u8int_t sbb_b (void)
{
  tm->a = (tm->a & 0xff) + ((~(tm->b + tm_get_carry ()) + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

//0x99
u8int_t sbb_c (void)
{
  tm->a = (tm->a & 0xff) + ((~(tm->c + tm_get_carry ()) + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

//0x9a
u8int_t sbb_d (void)
{
  tm->a = (tm->a & 0xff) + ((~(tm->d + tm_get_carry ()) + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

//0x9b
u8int_t sbb_e (void)
{
  tm->a = (tm->a & 0xff) + ((~(tm->e + tm_get_carry ()) + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

//0x9c
u8int_t sbb_h (void)
{
  tm->a = (tm->a & 0xff) + ((~(tm->h + tm_get_carry ()) + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

//0x9d
u8int_t sbb_l (void)
{
  tm->a = (tm->a & 0xff)+ ((~(tm->l + tm_get_carry ()) + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

//0x9e
u8int_t sbb_m (void)
{
  tm->a = (tm->a & 0xff) + ((~(tm->memory[tm_get_hl ()] + tm_get_carry ()) + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

//0x9f
u8int_t sbb_a (void)
{
  tm->a = (tm->a & 0xff) + ((~(tm->a + tm_get_carry ()) + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 0;
}

//0xd6
u8int_t _sui (void)
{
  u8int_t operand;
  
  operand = tm_fetch_operand();
  tm->a = (tm->a & 0xff) + ((~operand + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
//   sprintf (tm->current_instruction->operand_2, "%04xh", operand);
  return 1;
}

//0xd6 overloaded
u8int_t sui (u8int_t val)
{
  tm->a = (tm->a & 0xff) + ((~val + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 1;
}

//0xde
u8int_t _sbi (void)
{
  u8int_t operand;
  
  operand = tm_fetch_operand();
  tm->a = (tm->a & 0xff) + ((~(operand + tm_get_carry ()) + 1) & 0xff);
    
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
//   sprintf (tm->current_instruction->operand_2, "%04xh", operand);
  return 1;
}

//0xde overloaded
u8int_t sbi (u8int_t val)
{
  tm->a = (tm->a & 0xff) + ((~(val + tm_get_carry ()) + 1) & 0xff);
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
  tm_get_carry () ? tm_clear_carry () : tm_set_carry ();
  return 1;
}

//0x09
u8int_t dad_b (void)
{
  //32 bits to detect the carry
  u32int_t temp = tm_get_hl ();
  temp = temp + tm_get_bc ();
  if ((temp & 0xffff0000) != 0)
    tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, SET_CARRY);
  tm_set_hl (temp & 0xffff);
  return 0;
  //other flags unaffected
}

//0x19
u8int_t dad_d (void)
{
  //32 bits to detect the carry
  u32int_t temp = tm_get_hl ();
  temp = temp + tm_get_de ();
  if ((temp & 0xffff0000) != 0)
    tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, SET_CARRY);
  tm_set_hl (temp & 0xffff);
  return 0;
  //other flags unaffected
}

//0x29
u8int_t dad_h (void)
{
  //32 bits to detect the carry
  u32int_t temp = tm_get_hl ();
  temp = temp + temp;
  if ((temp & 0xffff0000) != 0)
    tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, SET_CARRY);
  tm_set_hl (temp & 0xffff);
  return 0;
  //other flags unaffected
}

//0x39
u8int_t dad_sp (void)
{
  //32 bits to detect the carry
  u32int_t temp = tm_get_hl ();
  temp = temp + tm->sp;
  if ((temp & 0xffff0000) != 0)
    tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, SET_CARRY);
  tm_set_hl (temp & 0xffff);
  return 0;

  //other flags unaffected
}


//0x04
u8int_t inr_b (void)
{
  u8int_t temp = tm->a;
  
  tm->b =tm-> b + 1;
  tm->a = tm->b & 0xff;
      
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x0c
u8int_t inr_c (void)
{
  u8int_t temp = tm->a;
  
  tm->c = tm->c + 1;
  tm->a = tm->c & 0xff;
      
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x14
u8int_t inr_d (void)
{
  u8int_t temp = tm->a;
  
  tm->d = tm->d + 1;
  tm->a = tm->d & 0xff;
      
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x1c
u8int_t inr_e (void)
{
  u8int_t temp = tm->a;
  
  tm->e = tm->e + 1;
  tm->a = tm->e & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x24
u8int_t inr_h (void)
{
  u8int_t temp = tm->a;
  
  tm->h = tm->h + 1;
  tm->a = tm->h & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x2c
u8int_t inr_l (void)
{
  u8int_t temp = tm->a;
  
  tm->l = tm->l + 1;
  tm->a = tm->l & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x34
u8int_t inr_m (void)
{
  u8int_t temp = tm->a;
  
  tm->memory[tm_get_hl ()] = tm->memory[tm_get_hl ()] + 1;
  tm->a = tm->memory[tm_get_hl ()] & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x3c
u8int_t inr_a (void)
{
  tm->a = (tm->a & 0xff) + 1;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //do not update carry
}

//0x05
u8int_t dcr_b (void)
{
  u8int_t temp = tm->a;
  
  tm->b = (tm->b + 0xff) & 0xff;
  tm->a = tm->b & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x0d
u8int_t dcr_c (void)
{
  u8int_t temp = tm->a;
  
  tm->c = (tm->c + 0xff) & 0xff;
  tm->a = tm->c & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x15
u8int_t dcr_d (void)
{
  u8int_t temp = tm->a;
  
  tm->d = (tm->d + 0xff) & 0xff;
  tm->a = tm->d & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x1d
u8int_t dcr_e (void)
{
  u8int_t temp = tm->a;
  
  tm->e = (tm->e + 0xff) & 0xff;
  tm->a = tm->e & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp;
  return 0;
}

//0x25
u8int_t dcr_h (void)
{
  u8int_t temp = tm->a;
  
  tm->h = (tm->h + 0xff) & 0xff;
  tm->a = tm->h;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x2d
u8int_t dcr_l (void)
{
  u8int_t temp = tm->a;
  
  tm->l = (tm->l + 0xff) & 0xff;
  tm->a = tm->l & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x35
u8int_t dcr_m (void)
{
  u8int_t temp = tm->a;
  
  tm->memory[tm_get_hl ()] = tm->memory[tm_get_hl ()] + 0xff;
  tm->a = tm->memory[tm_get_hl ()] & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  //do not update carry
  
  tm->a = temp & 0xff;
  return 0;
}

//0x3d
u8int_t dcr_a (void)
{
  tm->a = (tm->a + 0xff) & 0xff;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //do not update carry
}

//0x03
u8int_t inx_b (void)
{
  u16int_t temp = tm_get_bc ();
  temp = temp + 1;
  tm_set_bc (temp);
        
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //no flags affected
}

//0x13
u8int_t inx_d (void)
{
  u16int_t temp = tm_get_de ();
  temp = temp + 1;
  tm_set_de (temp);
          
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  //no flags affected
  return 0;
}

//0x23
u8int_t inx_h (void)
{
  u16int_t temp = tm_get_hl ();
  temp = temp + 1;
  tm_set_hl (temp);
          
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //no flags affected
}

//0x03
u8int_t inx_sp (void)
{
  tm->sp = tm->sp + 1;
          
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //no flags affected
}


//0x0b
u8int_t dcx_b (void)
{
  u16int_t temp = tm_get_bc ();
  temp = temp - 1;
  tm_set_bc (temp);
          
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //no flags affected
}

//0x1b
u8int_t dcx_d (void)
{
  u16int_t temp = tm_get_de ();
  temp = temp - 1;
  tm_set_de (temp);
          
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //no flags affected
}

//0x2b
u8int_t dcx_h (void)
{
  u16int_t temp = tm_get_hl ();
  temp = temp - 1;
  tm_set_hl (temp);
          
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //no flags affected
}

//0x3b
u8int_t dcx_sp (void)
{
  tm->sp = tm->sp - 1;
          
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //no flags affected
}

//FIXME: needs total rebuild, check what should be the carry, aux carry etc
//0x27
u8int_t daa (void)
{
  u8int_t old_carry = tm_get_carry ();
  
  if (((tm->a & 0x0f) > 0x09) || (tm_get_auxcarry () == true))
  {
    adi (0x06);
  }
  
  if ((((tm->a >> 4) & 0x0f) > 0x09) || (tm_get_carry () == true))
  {
    adi (0x60);
  }
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, (old_carry ? SET_CARRY : CLEAR_CARRY));
  return 0;
}


//0xa0
u8int_t ana_b (void)
{
  tm->a = (tm->a & 0xff) &tm-> b;
        
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xa1
u8int_t ana_c (void)
{
  tm->a = (tm->a & 0xff) & tm->c;
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xa2
u8int_t ana_d (void)
{
  tm->a = (tm->a & 0xff) & tm->d;
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xa3
u8int_t ana_e (void)
{
  tm->a = (tm->a & 0xff) & tm->e;
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xa4
u8int_t ana_h (void)
{
  tm->a = (tm->a & 0xff) & tm->h;
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xa5
u8int_t ana_l (void)
{
  tm->a = (tm->a & 0xff) & tm->l;
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xa6
u8int_t ana_m (void)
{
  tm->a = (tm->a & 0xff) & tm->memory[tm_get_hl ()];
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xa7
u8int_t ana_a (void)
{
  tm->a = (tm->a & 0xff) & tm->a;
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xe6
u8int_t _ani (void)
{
  u8int_t operand;
  
  operand = tm_fetch_operand();
  tm->a = (tm->a & 0xff) & operand;
  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
//   sprintf (tm->current_instruction->operand_2, "%04xh", operand);
  return 1;
}

//0xe6 overloaded
u8int_t ani (u8int_t val)
{
  tm->a = (tm->a & 0xff) & val;
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, SET_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 1;
}

//0xa8
u8int_t xra_b (void)
{
  tm->a = (tm->a & 0xff) ^ tm->b;
          
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xa9
u8int_t xra_c (void)
{
  tm->a = (tm->a & 0xff) ^ tm->c;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xaa
u8int_t xra_d (void)
{
  tm->a = (tm->a & 0xff) ^ tm->d;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xab
u8int_t xra_e (void)
{
  tm->a = (tm->a & 0xff) ^ tm->e;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xac
u8int_t xra_h (void)
{
  tm->a = (tm->a & 0xff) ^ tm->h;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xad
u8int_t xra_l (void)
{
  tm->a = (tm->a & 0xff) ^ tm->l;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xae
u8int_t xra_m (void)
{
  tm->a = (tm->a & 0xff) ^ tm->memory[tm_get_hl ()];
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xaf
u8int_t xra_a (void)
{
  tm->a = (tm->a & 0xff) ^ tm->a;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xee
u8int_t _xri (void)
{
  u8int_t operand;
  
  operand = tm_fetch_operand();
  tm->a = (tm->a & 0xff) ^ operand;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
//   sprintf (tm->current_instruction->operand_2, "%04xh", operand);
  return 1;
}

//0xee overloaded
u8int_t xri (u8int_t val)
{
  tm->a = (tm->a & 0xff) ^ val;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 1;
}

//0xb0
u8int_t ora_b (void)
{
  tm->a = (tm->a & 0xff) | tm->b;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xb1
u8int_t ora_c (void)
{
  tm->a = (tm->a & 0xff) | tm->c;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xb2
u8int_t ora_d (void)
{
  tm->a = (tm->a & 0xff) | tm->d;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xb3
u8int_t ora_e (void)
{
  tm->a = (tm->a & 0xff) | tm->e;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xb4
u8int_t ora_h (void)
{
  tm->a = (tm->a & 0xff) | tm->h;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xb5
u8int_t ora_l (void)
{
  tm->a = (tm->a & 0xff) | tm->l;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xb6
u8int_t ora_m (void)
{
  tm->a = (tm->a & 0xff) | tm->memory[tm_get_hl ()];
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xb7
u8int_t ora_a (void)
{
  tm->a = (tm->a & 0xff) | tm->a;
            
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 0;
}

//0xb8
u8int_t cmp_b (void)
{
  u8int_t temp = tm->a;

  tm->a = (tm->a & 0xff) + ~tm->b + 1;

  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
 
  tm->a = temp;
  return 0;
}

//0xfe
u8int_t _cpi (void)
{
  u8int_t temp = tm->a, operand;
  
  operand = tm_fetch_operand();
  tm->a = (tm->a & 0xff) + ~operand + 1;
              
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);

  tm->a = temp;
//   sprintf (tm->current_instruction->operand_2, "%04xh", operand);
  return 1;
}

//0xfe overloaded
u8int_t cpi (u8int_t val)
{
  u8int_t temp = tm->a;
  
  tm->a = (tm->a & 0xff) + ~val + 1;
              
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);

  tm->a = temp;
  return 0;
}

//0xf6
u8int_t _ori (void)
{
  u8int_t operand;
  
  operand = tm_fetch_operand();
  tm->a = (tm->a & 0xff) | operand;
              
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
//   sprintf (tm->current_instruction->operand_2, "%04xh", operand);
  return 1;
}

//0xf6 overloaded
u8int_t ori (u8int_t val)
{
  tm->a = (tm->a & 0xff) | val;
                
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, CLEAR_AUXCARRY, UPDATE_PARITY, CLEAR_CARRY);
  return 1;
}

//0xb9
u8int_t cmp_c (void)
{
  u8int_t temp = tm->a;

  tm->a = (tm->a & 0xff) + ~tm->c + 1;
                
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);

  tm->a = temp & 0xff;
  return 0;
}

//0xba
u8int_t cmp_d (void)
{
  u8int_t temp = tm->a;

  tm->a = (tm->a & 0xff) + ~tm->d + 1;
                  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);

  tm->a = temp & 0xff;
  return 0;
}

//0xbb
u8int_t cmp_e (void)
{
  u8int_t temp = tm->a;

  tm->a = (tm->a & 0xff) + ~tm->e + 1;
                  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);

  tm->a = temp & 0xff;
  return 0;
}

//0xbc
u8int_t cmp_h (void)
{
  u8int_t temp = tm->a;

  tm->a = (tm->a & 0xff) + ~tm->h + 1;
                  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);

  tm->a = temp & 0xff;
  return 0;
}

//0xbd
u8int_t cmp_l (void)
{
  u8int_t temp = tm->a;

  tm->a = (tm->a & 0xff) + ~tm->l + 1;
                  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);

  tm->a = temp & 0xff;
  return 0;
}

//0xbe
u8int_t cmp_m (void)
{
  u8int_t temp = tm->a;

  tm->a = (tm->a & 0xff) + ~tm->memory[tm_get_hl ()] + 1;
                  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);

  tm->a = temp & 0xff;
  return 0;
}

//0xbf
u8int_t cmp_a (void)
{
  u8int_t temp = tm->a;

  tm->a = (tm->a & 0xff) + ~tm->a + 1;
                  
  tm_update_flags (UPDATE_SIGN, UPDATE_ZERO, UPDATE_AUXCARRY, UPDATE_PARITY, UPDATE_CARRY);
 
  tm->a = temp & 0xff;
  return 0;
}

//0x2f
u8int_t cma (void)
{
  tm->a = ~tm->a & 0xff;
  
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //no flags affected
}

//0x3f
u8int_t cmc (void)
{
  tm_get_carry () == 1 ? tm_clear_carry () : tm_set_carry ();      
  
  tm_update_flags (NO_CHANGE_SIGN, NO_CHANGE_ZERO, NO_CHANGE_AUXCARRY, NO_CHANGE_PARITY, NO_CHANGE_CARRY);
  return 0;
  //no other flags are affected
}

//0x37
u8int_t stc (void)
{
  tm_set_carry ();
  return 0;
}

//0x07
u8int_t rlc (void)
{
  _Bool msb = ((tm->a & 0x80) == 0x80);
  tm->a <<= 1;
  tm->a |= msb;
  (msb) ? tm_set_carry () : tm_clear_carry ();
  return 0;
  //other flags not affected
}

//0x0f
u8int_t rrc (void)
{
  _Bool lsb = ((tm->a & 0x01) == 0x01);
  tm->a >>= 1;
  tm->a |= (lsb << 7);
  (lsb == 1) ? tm_set_carry () : tm_clear_carry ();
  return 0;
  //other flags not affected
}

//0x17
u8int_t ral (void)
{
  _Bool msb = ((tm->a & 0x80) == 0x80);
  tm->a <<= 1;
  tm->a |= (tm_get_carry ());
  
  (msb) ? tm_set_carry () : tm_clear_carry ();
  
  return 0;
  //other flags not affected
}

//0x1f
u8int_t rar (void)
{
    _Bool lsb = ((tm->a & 0x01) == 0x01);
  tm->a >>= 1;
  tm->a |= (tm_get_carry () << 7);
  
  (lsb) ? tm_set_carry () : tm_clear_carry ();
  
  return 0;

  //other flags not affected
}

//0xc3
u8int_t jmp (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t jump_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", jump_addr);
//   u16int_t addr = make_val_16 (tm_fetch_operand (), tm_fetch_operand ());
//   addr = tm_fetch_operand();
//   addr <<= 8;
//   addr |= tm_fetch_operand();
  
  tm->pc = jump_addr;
  return 2;
  
//   printf ("\n\t\tdebug : jmp addr : %x", pc);
}


//0xda
u8int_t jc (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t jump_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", jump_addr);

  if (tm_get_carry ())
  {
    tm->pc = jump_addr;
//     printf ("\n\t\tdebug : jc addr : %x", pc);
  }
  return 2;
}

//0xd2
u8int_t jnc (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t jump_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", jump_addr);
  
  if (!tm_get_carry ())
  {
    tm->pc = jump_addr;
//     printf ("\n\t\tdebug : jnc addr : %x", pc);
  }
  return 2;
}

//0xf2
u8int_t jp (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t jump_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", jump_addr);
  
  if (!tm_get_sign ())
  {
    tm->pc = jump_addr;
//     printf ("\n\t\tdebug : jp addr : %x", pc);
  }
  return 2;
}

//0xfa
u8int_t jm (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t jump_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", jump_addr);

  if (tm_get_sign())
  {
    tm->pc = jump_addr;
//     printf ("\n\t\tdebug : jm addr : %x", pc);
  }
  return 2;
}

//0xea
u8int_t jpe (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t jump_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", jump_addr);

  if (tm_get_parity ())
  {
    tm->pc = jump_addr;
//     printf ("\n\t\tdebug : jpe addr : %x", pc);
  }
  return 2;
}

//0xe2
u8int_t jpo (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t jump_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", jump_addr);

  if (!tm_get_parity ())
  {
    tm->pc = jump_addr;
//     printf ("\n\t\tdebug : jpo addr : %x", pc);
  }
  return 2;
}

//0xca
u8int_t jz (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t jump_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", jump_addr);
  
  if (tm_get_zero ())
  {
    tm->pc = jump_addr;
//     printf ("\n\t\tdebug : jz addr : %x", pc);
  }
  return 2;
}

//0xc2
u8int_t jnz (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t jump_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", jump_addr);
  
  if (!tm_get_zero ())
  {
    tm->pc = jump_addr;
//     printf ("\n\t\tdebug : jnz addr : %x", pc);
  }
  return 2;
}

//0xc5
u8int_t push_b (void)
{
  tm->memory[--tm->sp] = tm->b;
  tm->memory[--tm->sp] = tm->c;
  return 0;
  //no flags affected
}

//0xd5
u8int_t push_d (void)
{
  tm->memory[--tm->sp] = tm->d;
  tm->memory[--tm->sp] = tm->e;
  return 0;
  //no flags affected
}

//0xe5
u8int_t push_h (void)
{
  tm->memory[--tm->sp] = tm->h;
  tm->memory[--tm->sp] = tm->l;
  return 0;
  //no flags affected
}

//0xf5
u8int_t push_psw (void)
{
  tm->memory[--tm->sp] = tm->a;
  tm->memory[--tm->sp] = tm->flag;
  return 0;
  //no flags affected
}

//0xc1
u8int_t pop_b (void)
{
  tm->c = tm->memory[tm->sp++];
  tm->b = tm->memory[tm->sp++];
  return 0;
  //no flags affected
}

//0xd1
u8int_t pop_d (void)
{
  tm->e = tm->memory[tm->sp++];
  tm->d = tm->memory[tm->sp++];
  return 0;
  //no flags affected
}

//0xe1
u8int_t pop_h (void)
{
  tm->l = tm->memory[tm->sp++];
  tm->h = tm->memory[tm->sp++];
  return 0;
  //no flags affected
}

//0xf1
u8int_t pop_psw (void)
{
  tm->flag = tm->memory[tm->sp++];
  tm->a = tm->memory[tm->sp++] & 0xff;
  return 0;
  //no flags affected
}

//0xcd
u8int_t call (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t call_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", call_addr);
  
  //push pc
  tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
  tm->memory[--tm->sp] = (tm->pc & 0xff);
  
  //alter execution sequence as per 16 bit address
  tm->pc = call_addr;
  return 2;
}

//0xdc
u8int_t cc (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t call_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", call_addr);
  
  if (tm_get_carry ())
  {
    tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
    tm->memory[--tm->sp] = (tm->pc & 0xff);
    tm->pc = call_addr;
  }
  return 2;
}

//0xd4
u8int_t cnc (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t call_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", call_addr);

  if (!tm_get_carry ())
  {
    tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
    tm->memory[--tm->sp] = (tm->pc & 0xff);
    tm->pc = call_addr;
  }
  return 2;
}

//0xf4
u8int_t cp (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t call_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", call_addr);
  
  if (!tm_get_sign ())
  {
    tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
    tm->memory[--tm->sp] = (tm->pc & 0xff);
    tm->pc = call_addr;
  }
  return 2;
}

//0xfc
u8int_t cm (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t call_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", call_addr);
  
  if (tm_get_sign ())
  {
    tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
    tm->memory[--tm->sp] = (tm->pc & 0xff);
    tm->pc = call_addr;
  }
  return 2;
}

//0xec
u8int_t cpe (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t call_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", call_addr);
  
  if (tm_get_parity ())
  {
    tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
    tm->memory[--tm->sp] = (tm->pc & 0xff);
    tm->pc = call_addr;
  }
  return 2;
}

//0xe4
u8int_t cpo (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t call_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", call_addr);
  
  if (!tm_get_parity ())
  {
    tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
    tm->memory[--tm->sp] = (tm->pc & 0xff);
    tm->pc = call_addr;
  }
  return 2;
}

//0xcc
u8int_t cz (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t call_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", call_addr);
  
  if (tm_get_zero ())
  {
    tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
    tm->memory[--tm->sp] = (tm->pc & 0xff);
    tm->pc = call_addr;
  }
  return 2;
}

//0xc4
u8int_t cnz (void)
{
  u8int_t byte_low = tm_fetch_operand (), byte_high = tm_fetch_operand();
  u16int_t call_addr = make_val_16 (byte_high, byte_low);
//   sprintf (tm->current_instruction->operand_2, "%04xh", call_addr);
  
  if (!tm_get_zero ())
  {
    tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
    tm->memory[--tm->sp] = (tm->pc & 0xff);
    tm->pc = call_addr;
  }
  return 2;
}

//0xc9
u8int_t ret (void)
{
  //pop stack and get 16 bit jump address into pc
  u8int_t byte_low = tm->memory[tm->sp++], byte_high = tm->memory[tm->sp++];
  u16int_t ret_addr = make_val_16 (byte_high, byte_low);
  
  tm->pc = ret_addr;
  return 0;
}

//0xd8
u8int_t rc (void)
{
  u8int_t byte_low, byte_high;
  u16int_t ret_addr;
  
  if (tm_get_carry ())
  {
    byte_low = tm->memory[tm->sp++];
    byte_high = tm->memory[tm->sp++];
    ret_addr = make_val_16 (byte_high, byte_low);
    tm->pc = ret_addr;
  }
  return 0;
}

//0xd0
u8int_t rnc (void)
{
  u8int_t byte_low, byte_high;
  u16int_t ret_addr;
  
  if (!tm_get_carry ())
  {
    byte_low = tm->memory[tm->sp++];
    byte_high = tm->memory[tm->sp++];
    ret_addr = make_val_16 (byte_high, byte_low);
    tm->pc = ret_addr;
  }
  return 0;
}

//0xf0
u8int_t rp (void)
{
  u8int_t byte_low, byte_high;
  u16int_t ret_addr;
  
  if (!tm_get_sign ())
  {
    byte_low = tm->memory[tm->sp++];
    byte_high = tm->memory[tm->sp++];
    ret_addr = make_val_16 (byte_high, byte_low);
    tm->pc = ret_addr;
  }
  return 0;
}

//0xf8
u8int_t rm (void)
{
  u8int_t byte_low, byte_high;
  u16int_t ret_addr;
  
  if (tm_get_sign ())
  {
    byte_low = tm->memory[tm->sp++];
    byte_high = tm->memory[tm->sp++];
    ret_addr = make_val_16 (byte_high, byte_low);
    tm->pc = ret_addr;
  }
  return 0;
}

u8int_t rpe (void)
{
  u8int_t byte_low, byte_high;
  u16int_t ret_addr;
  
  if (tm_get_parity ())
  {
     byte_low = tm->memory[tm->sp++];
     byte_high = tm->memory[tm->sp++];
     ret_addr = make_val_16 (byte_high, byte_low);
    tm->pc = ret_addr;
  }
  return 0;
}

//0xe0
u8int_t rpo (void)
{
  u8int_t byte_low, byte_high;
  u16int_t ret_addr;
  
  if (!tm_get_parity ())
  {
    byte_low = tm->memory[tm->sp++];
    byte_high = tm->memory[tm->sp++];
    ret_addr = make_val_16 (byte_high, byte_low);
    tm->pc = ret_addr;
  }
  return 0;
}

//0xc8
u8int_t rz (void)
{
  u8int_t byte_low, byte_high;
  u16int_t ret_addr;
  
  if (tm_get_zero ())
  {
    byte_low = tm->memory[tm->sp++];
    byte_high = tm->memory[tm->sp++];
    ret_addr = make_val_16 (byte_high, byte_low);
    tm->pc = ret_addr;
  }
  return 0;
}

//0xc0
u8int_t rnz (void)
{
  u8int_t byte_low, byte_high;
  u16int_t ret_addr;
  
  if (!tm_get_zero ())
  {
    byte_low = tm->memory[tm->sp++];
    byte_high = tm->memory[tm->sp++];
    ret_addr = make_val_16 (byte_high, byte_low);
    tm->pc = ret_addr;
  }
  return 0;
}

// TODO: hlt, rst, sim, rim, di, ei, in, out
// TODO: interrupt
// TODO: code cleanup
// TODO: testing, and bug removal
//BELOW ARE THE TODO functions
/* should implement an interrupt mask register in the main datastructure 
 * should modify the rst instruction instruction depending upon the mask
 * should implement the vectored interrupts in some way like some special key is pressed, 
 * or later we might take interrupt from the parallal port or else
 * should fix some terminal output or some other port (LPT) output for the serial data outs
 * if we really are going to read and write data fromt he LPT or else then we need to implement 
 * good timings.
 */
//0x20
u8int_t rim (void)
{
  return 0;
}

//0x30
u8int_t sim (void)
{
  return 0;
}

//0xdb
u8int_t in (void)
{
  return 0;
}

//0xd3
u8int_t out (void)
{
  return 0;
}

//0xf3
u8int_t di (void)
{
  return 0;
}

//0xfb
u8int_t ei (void)
{
  return 0;
}

//0xc7
u8int_t rst_0 (void)
{
  tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
  tm->memory[--tm->sp] = (tm->pc & 0xff);
  tm->pc = 0x0000;
  return 0;
}

//0xcf
u8int_t rst_1 (void)
{
  tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
  tm->memory[--tm->sp] = (tm->pc & 0xff);
  tm->pc = 0x0001 << 3;
  return 0;
}

//0xd7
u8int_t rst_2 (void)
{
  tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
  tm->memory[--tm->sp] = (tm->pc & 0xff);
  tm->pc = 0x0002 << 3;
  return 0;
}

//x0df
u8int_t rst_3 (void)
{
  tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
  tm->memory[--tm->sp] = (tm->pc & 0xff);
  tm->pc = 0x0003 << 3;
  return 0;
}

//0xe7
u8int_t rst_4 (void)
{
  tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
  tm->memory[--tm->sp] = (tm->pc & 0xff);
  tm->pc = 0x0004 << 3;
  return 0;
}

//0xef
u8int_t rst_5 (void)
{
  tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
  tm->memory[--tm->sp] = (tm->pc & 0xff);
  tm->pc = 0x0005 << 3;
  return 0;
}

//0xf7
u8int_t rst_6 (void)
{
  tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
  tm->memory[--tm->sp] = (tm->pc & 0xff);
  tm->pc = 0x0006 << 3;
  return 0;
}

//0xff
u8int_t rst_7 (void)
{
  tm->memory[--tm->sp] = ((tm->pc >> 8) & 0xff);
  tm->memory[--tm->sp] = (tm->pc & 0xff);
  tm->pc = 0x0007 << 3;
  return 0;
}

/* should we use the abstracted functions in the execute module, or keep these funcitons in this file and access
 * the data structures directly, this will avoid unnecessary function calls. The abstracted access functions would
 * help using the datastructures from the outside.
 */

/* TODO: do not complicate, first do the global current instruction read and execute cycles */
/* INTROCUDE NEW STRUCTURE, MACHINE STATE, this will contain the regs, mem, and the current instruction pointer
 * then this could be used to pass to some other GUI functions, and we can also create multiple machines etc
 * think about this, should we create a struct with multuple machine instances, or simply rely on the access function
 * and the one and only global machine
 */
void register_machine (_8085_machine_t *machine)
{
  tm = machine;
  tm->current_instruction = NULL;
  tm_clear_regs ();
  tm_clear_flags ();
  memset (tm->memory, 0, MAX_MEM);
}

u8int_t execute (u16int_t start_addr)
{
  u8int_t retval;
  
  tm_set_start_address (start_addr);

  do {
    tm_fetch_instruction ();
    retval = tm_execute_instruction ();
    
//     printf ("\n[%04x] : [%02x %5s]\t%-4s %4s, %6s", tm->prev_pc, opcode, get_current_operand_2 (), get_current_opcode_str (), get_current_operand_1 (), get_current_operand_2 ());
    
//     printf ("\t\t{s: %d\tz: %d\tac: %d\tp: %d\tcy: %d}", tm_get_sign (), tm_get_zero (), tm_get_auxcarry (), tm_get_parity (), tm_get_carry ());
    
  } while ((retval != EXECUTE_END) && (retval != EXECUTE_ERR));

  tm->current_instruction = NULL;
  return retval;
}

u8int_t execute_step (u16int_t start_addr, u8int_t mode)
{
  u8int_t retval;
  static int first_time = 1;
  
  if (((mode == EXECUTE_STEP) || (mode == EXECUTE_OVER)) && (first_time == 1))
  {
    /* at first cycle fetch and return */
    tm_set_start_address (start_addr);
    tm_fetch_instruction ();
    first_time = 0;
    return mode;
  }
  else if (mode == EXECUTE_END)
  {
    first_time = 1;
    tm->current_instruction = NULL;
    return EXECUTE_END;
  }
   
  if (mode == EXECUTE_STEP)
    retval = tm_execute_instruction ();
  else if (mode == EXECUTE_OVER)
  {
    tm_step_over_instruction ();
    retval = EXECUTE_STEP;
  }
//     printf ("\n[%04x] : [%02x %5s]\t%-4s %4s, %6s", tm->prev_pc, opcode, get_current_operand_2 (), get_current_opcode_str (), get_current_operand_1 (), get_current_operand_2 ());
    
//     printf ("\t\t{s: %d\tz: %d\tac: %d\tp: %d\tcy: %d}", tm_get_sign (), tm_get_zero (), tm_get_auxcarry (), tm_get_parity (), tm_get_carry ());
    
  if ((retval == EXECUTE_END) || (retval== EXECUTE_ERR))
  {
    first_time = 1;
    tm->current_instruction = NULL;
    return retval;
  }
  else 
  {
    tm_fetch_instruction ();
    return mode;
  }
}

/* TEST: CODE FUNCTION
void input_code (void)
{
  u16int_t start_addr;
  u32int_t code;
  int i;
  
  printf ("\nEnter start address : ");
  scanf ("%x", &start_addr);
  
  i = start_addr;
  
  do {
    printf ("%x : ", i);
    scanf ("%x", &code);
    tm_set_memory (i, code);
    i++;
  } while (code != 0x76);
  
  printf ("\nInput complete\n");
  execute (start_addr);
}
*/

// int main (void)
// {
//   _8085_machine_t machine;
//   
//   register_machine (&machine);
//   machine.memory [0x00] = 10;
//   machine.memory [0x01] = 7;
//   machine.memory [0x02] = 8;
//   machine.memory [0x03] = 6;
//   machine.memory [0x04] = 1;
//   machine.memory [0x05] = 0;
//   machine.memory [0x06] = 2;
//   machine.memory [0x07] = 3;
//   machine.memory [0x08] = 4;
//   machine.memory [0x09] = 9;
//   machine.memory [0x0a] = 5;
//   
// //   memory[0x4200] = 0x06;
// //   memory[0x4201] = 0x09;
// //   memory[0x4202] = 0x3e;
// //   memory[0x4203] = 0x08;
// //   memory[0x4204] = 0x80;
// //   memory[0x4205] = 0x76;
//   input_code ();
// //   execute (0x4200);
// //   for (int i=0x4200; i<=0x4205; i++)
// //     printf ("\nmemory[%d] = %d", i, memory[i]);
// //   execute (0x4200);
//   for (int i=0x07d0; i<=0x07da; i++)
//     printf ("\nmemory[%d] = %d", i, machine.memory[i]);
//   printf ("\n");
//   return 0;
// }
