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

/* TODO: create proper types header file and clear mess, CLEAR THE BELOW SHIT HOLE 
 * BY TAKING GLOBAL PTRS ANDS OPERATING ON THEM. SIMULATE CLASSES and the GUI object
 * make better file handling system with prompt
 * a help screen, or window
 * reset machine, memory, registers, flags separately
 * the _8085_machine_t need some more states to hold in order for better interface i think.
 */
/* BUG: when the start address is set the mloc is reset to 0 can't find why */
/* NOTE:
 * we are using the memory access functions to access the memory in this file, to much function call but use of abstraction.
 */

#include <stdio.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>


#include "8085.h"

#define REGS_WIN_H        20
#define REGS_WIN_W        52
#define REGS_WIN_XPOS     50
#define REGS_WIN_YPOS     5

#define LCD_WIN_H         2
#define LCD_WIN_W         15
#define LCD_WIN_XPOS      10
#define LCD_WIN_YPOS      10

#define PROMPT_WIN_H      10
#define PROMPT_WIN_W      30
#define PROMPT_WIN_XPOS   10
#define PROMPT_WIN_YPOS   13

#define HELP_WIN_H        0
#define HELP_WIN_W        0
#define HELP_WIN_XPOS     0
#define HELP_WIN_YPOS     5

void show_name (void)
{
  move (2, 5);
  printw ("[[:Dirty 8085a simulator:]] testing  [GPLv3] ");
  refresh();
}

void help (void)
{
  WINDOW *helpwin = newwin (HELP_WIN_H, HELP_WIN_W, HELP_WIN_YPOS, HELP_WIN_XPOS);
  
  wmove (helpwin, 10, 0);
  wprintw (helpwin, "\n     Memory Up/Down                :  Up/Down key , Enter/Backspace");
  wprintw (helpwin, "\n     Memory Jump 16 places Up/Down :  Page Up/Down");
  wprintw (helpwin, "\n     Goto Memory Location          :  g , F3");
  wprintw (helpwin, "\n     Set Start Address             :  s");
  wprintw (helpwin, "\n     Start Execution               :  x , F5");
  wprintw (helpwin, "\n     Step Execution                :  z , F7");
  wprintw (helpwin, "\n     Step Over                     :      F8 ");
  wprintw (helpwin, "\n     End Step Execution            :  q");
  wprintw (helpwin, "\n     Read machine state from file  :      F9");
  wprintw (helpwin, "\n     Write machine state to file   :      F6");
  wprintw (helpwin, "\n     Reset Flags and Registers     :  r");
  wprintw (helpwin, "\n     Help                          :  h  , F1");
  wprintw (helpwin, "\n     Quit                          :  ESC, F12");

  wprintw (helpwin, "\n\nUse 0-9 and a-f to enter values at address");
  wrefresh (helpwin);
  wgetch (helpwin);
  wclear (helpwin);
  wrefresh (helpwin);
  delwin (helpwin);
  return;
}

/* Messy code, passing start addr here to print with this */
void update_regs (WINDOW *regs, u16int_t start_addr)
{
  int i=0;

  mvwprintw (regs, i++, 12, "+-----------+");
  mvwprintw (regs, i++, 12, "| regiters  |  start address : %04x", start_addr);
  mvwprintw (regs, i++, 12, "+----+------+-----------------------+");
  mvwprintw (regs, i++, 12, "| a  |          [%02x]                |", tm_get_reg_a ());
  mvwprintw (regs, i++, 12, "+----+------------+------------+----+");
  mvwprintw (regs, i++, 12, "| b  |    [%02x]    |    [%02x]    |  c |", tm_get_reg_b (), tm_get_reg_c ());
  mvwprintw (regs, i++, 12, "+----+------------+------------+----+");
  mvwprintw (regs, i++, 12, "| d  |    [%02x]    |    [%02x]    |  e |", tm_get_reg_d (), tm_get_reg_e ());
  mvwprintw (regs, i++, 12, "+----+------------+------------+----+");
  mvwprintw (regs, i++, 12, "| h  |    [%02x]    |    [%02x]    |  l |", tm_get_reg_h (), tm_get_reg_l ());;
  mvwprintw (regs, i++, 12, "+----+------------+------------+----+");
  mvwprintw (regs, i++, 12, "| pc |           [%04x]             |", tm_get_reg_pc ());
  mvwprintw (regs, i++, 12, "+-----------------------------------+");
  mvwprintw (regs, i++, 12, "| sp |           [%04x]             |", tm_get_reg_sp ());
  mvwprintw (regs, i++, 12, "+-----------------------------------+");
  
  i=0;
  wmove (regs, i++, 0);
  mvwprintw (regs, i++, 0, "+-------+");
  mvwprintw (regs, i++, 0,"|flags  |");
  mvwprintw (regs, i++, 0,"+---+---+");
  mvwprintw (regs, i++, 0,"| s |%2d |", tm_get_sign ());
  mvwprintw (regs, i++, 0,"+---+---+");
  mvwprintw (regs, i++, 0,"| z |%2d |", tm_get_zero ());
  mvwprintw (regs, i++, 0,"+---+---+");
  mvwprintw (regs, i++, 0,"|ac |%2d |", tm_get_auxcarry ());
  mvwprintw (regs, i++, 0,"+---+---+");
  mvwprintw (regs, i++, 0,"| p |%2d |", tm_get_parity ());
  mvwprintw (regs, i++, 0,"+---+---+");
  mvwprintw (regs, i++, 0,"|cy |%2d |", tm_get_carry ());
  mvwprintw (regs, i++, 0,"+---+---+");
  
  wrefresh (regs);
//   wmove (regs, i, 0);
}

/* // currently this is inside the main 
void update_lcd (WINDOW *lcd, u16int_t mloc)
{
}
*/

#define ERROR     0
#define WRITE     1
#define OVERWRITE 2
#define EXISTS    3
#define SAVED     4
#define OPENED    5

u8int_t machine_read (_8085_machine_t *machine, char *file_name)
{
  int fd;
  
  fd = open (file_name, O_RDONLY);
  if (fd == -1)
    return ERROR;
  if (read (fd, machine, sizeof (_8085_machine_t)) == sizeof (_8085_machine_t))
    return OPENED;
  
  return ERROR;
  /* we should will not reach here */
}

u8int_t machine_write (_8085_machine_t *machine, char *file_name, u8int_t mode)
{
  int fd;
 
  if (mode == OVERWRITE)
    fd = open (file_name, O_WRONLY, O_TRUNC, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
  else if (mode == WRITE)
  {
    fd = open (file_name, O_RDONLY);
    if (fd == -1)
    {
      fd = open (file_name, O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    }
    else 
    {
      close (fd);
      return EXISTS;
    }
  }
  
  if (fd == -1)
    return ERROR;
  if (write (fd, machine, sizeof (_8085_machine_t)) == sizeof (_8085_machine_t))
    return SAVED;
  
  /* we should will not reach here */
  return ERROR;
}


/* TESTING: signal handler function */
jmp_buf interface;

void signal_handler (int sig)
{
  int x=sig;x++;
  longjmp (interface, 1);
}

int main (void)
{
  _8085_machine_t machine;
  char file_name[FILENAME_MAX];
  u16int_t mloc = 0;
  u8int_t retval;
  u16int_t start_addr = 0x0000;
  int val, flag = 1;
  char high_low = 1;
  WINDOW *lcd, *prompt_area, *regs;
  
  register_machine (&machine);
  
  
  /* TESTING: install signal handler to terminate current execution when ctrl + c is pressed 
   * helpful to break out from infinite loop
   */
  signal (SIGINT, signal_handler);
  
  
  initscr ();

  lcd = newwin (LCD_WIN_H, LCD_WIN_W, LCD_WIN_YPOS, LCD_WIN_XPOS);
  prompt_area = newwin (PROMPT_WIN_H, PROMPT_WIN_W, PROMPT_WIN_YPOS, PROMPT_WIN_XPOS);
  regs = newwin (REGS_WIN_H, REGS_WIN_W, REGS_WIN_YPOS, REGS_WIN_XPOS);

  crmode ();
  cbreak ();
  noecho ();
  keypad (lcd, TRUE);
  
  show_name ();

//   printf ("\n\n");
//   wmove (prompt_area, 3, 0);
//   wprintw (prompt_area, "current start address: %x", start_addr);
  
  while (1)
  {
    setjmp (interface);
    update_regs (regs, start_addr);
    wrefresh (prompt_area);

    if (flag)
    {
      wclear (lcd); 
      mvwprintw (lcd, 0, 0, " [ mem | val]");
      wattron (lcd, A_BOLD);
      mvwprintw (lcd, 1, 0, " [%04x |  %02x]", mloc, tm_fetch_memory (mloc));
      wattroff (lcd, A_BOLD);
      wrefresh (lcd);
    }
    flag = 1;
    val = tolower (wgetch (lcd));
    switch (val)
    {
      case '\n':
      case KEY_UP:
	mloc++;
	break;
	
      case KEY_BACKSPACE:
      case KEY_DOWN:
	mloc--;
	break;
	
      case KEY_PPAGE:
	mloc += 0x10;
	break;
	
      case KEY_NPAGE:
	mloc -= 0x10;
	break;
	
      case KEY_F(3):
      case 'g':
	wclear (prompt_area);
	mvwprintw (prompt_area, 0, 0, "goto: ");
	echo ();
	wscanw (prompt_area, "%x", &mloc);
	noecho ();
	wclear (prompt_area);
	break;
	
      case 's': // input 'begin' start address, or simply take the start address as the location where 'x' was pressed
	wclear (prompt_area);
	mvwprintw (prompt_area, 0, 0, "start address: ");
	echo ();
	wscanw (prompt_area, "%x", &start_addr);
	noecho ();
	wmove (prompt_area, 0, 0);
	wclear (prompt_area);
	update_regs (regs, start_addr);
	break;
	
      case 'x':
      case KEY_F(5): //full execute
	wclear (prompt_area);
	mvwprintw (prompt_area, 0, 0, "executing ...");
	retval = execute (start_addr);
	wmove (prompt_area, 0, 0);
	wclrtoeol (prompt_area);
	update_regs (regs, start_addr);
	if (retval == EXECUTE_END)
	  mvwprintw (prompt_area, 0, 0, "execution complete");
	else
	{
	  mloc = tm_get_reg_prev_pc ();
	  mvwprintw (prompt_area, 0, 0, "execution error @ mem");
	}
	execute_step (start_addr, EXECUTE_END);  // if step execute was running stop it
	break;
	
      case KEY_F(7): //step execute
	wclear (prompt_area);
	wattron (prompt_area, A_STANDOUT);
	mvwprintw (prompt_area, 1, 0, "now : %-5s %5s , %5s", tm_get_opcode_str (), tm_get_operand_1 (), tm_get_operand_2 ());
	wattroff (prompt_area, A_STANDOUT);
	retval = execute_step (start_addr, EXECUTE_STEP);
	wattron (prompt_area, A_BOLD);
	mvwprintw (prompt_area, 2, 0, "next: %-5s %5s , %5s", tm_get_opcode_str (), tm_get_operand_1 (), tm_get_operand_2 ());
	wattroff (prompt_area, A_BOLD);
	update_regs (regs, start_addr);
	mloc = tm_get_reg_pc ();  //NOTE: direct access in object
	if (retval == EXECUTE_STEP)
	  mvwprintw (prompt_area, 0, 0, "step executing ...");
	else if (retval == EXECUTE_END)
	  mvwprintw (prompt_area, 0, 0, "execution end");
	else
	{
	  mloc = tm_get_reg_prev_pc ();
	  mvwprintw (prompt_area, 0, 0, "execution error @ mem");
	}
	break;
      
      case KEY_F(8): //step over
	wclear (prompt_area);
	wattron (prompt_area, A_STANDOUT);
	mvwprintw (prompt_area, 1, 0, "over: %-4s %2s, %5s", tm_get_opcode_str (), tm_get_operand_1 (), tm_get_operand_2 ());
	wattroff (prompt_area, A_STANDOUT);
	retval = execute_step (start_addr, EXECUTE_OVER);
	wattron (prompt_area, A_BOLD);
	mvwprintw (prompt_area, 2, 0, "next: %-4s %2s, %5s", tm_get_opcode_str (), tm_get_operand_1 (), tm_get_operand_2 ());
	wattroff (prompt_area, A_BOLD);
	update_regs (regs, start_addr);
	mloc = machine.prev_pc; //NOTE: direct access in object
	if (retval == EXECUTE_OVER)
	  mvwprintw (prompt_area, 0, 0, "stepping over ...");
	else if (retval == EXECUTE_END)
	  mvwprintw (prompt_area, 0, 0, "execution end");
	else
	{
	  mloc = tm_get_reg_prev_pc ();
	  mvwprintw (prompt_area, 0, 0, "execution error @ mem");
	}
	break;
	
      case 'q': //end step execute
	wclear (prompt_area);
	execute_step (start_addr, EXECUTE_END);
	mvwprintw (prompt_area, 0, 0, "step execute end");
	update_regs (regs, start_addr);
	break;
	
	/* NOTE: after loading a file, the current_instruction field contains an invalid pointer we need to reset it
	 * when loading or saving. WE ARE CLEARING IT HERE MANUALLY WHILE SOME BETTER ABSTRACTION IS NOT IMPLEMENTED
	 */
      case KEY_F(9): //load machine from file
	wclear (prompt_area);
	mvwprintw (prompt_area, 0, 0, "load : enter file: ");
	echo ();
	mvwscanw (prompt_area, 1, 0, " %[^\n]", file_name);
	noecho ();
	wclear (prompt_area);
	if (machine_read (&machine, file_name) == OPENED)
	{
	  mvwprintw (prompt_area, 0, 0, "file \"%s\" loaded", file_name);
	  start_addr = tm_get_reg_pc (); // make the start address the last stored address in the machine in pc
	  update_regs (regs, start_addr);
	  machine.current_instruction = NULL; //WARNING: direct access as noted above
	}
	else
	  mvwprintw (prompt_area, 0, 0, "cannot load \"%s\"", file_name);
	file_name[0] = '\0';
	break;
	
      case KEY_F(6): //save machine to file
	wclear (prompt_area);
	mvwprintw (prompt_area, 0, 0, "save : enter file: ");
	wmove (prompt_area, 1, 0);
	echo ();
	mvwscanw (prompt_area, 1, 0, " %[^\n]", file_name);
	noecho ();
	wclear (prompt_area);
	retval = machine_write(&machine, file_name, WRITE);
	if (retval == SAVED)
	  mvwprintw (prompt_area, 0, 0, "file \"%s\" saved", file_name);
	else if (retval == EXISTS)
	{
	  wclear (prompt_area);
	  mvwprintw (prompt_area, 0, 0, "file \"%s\" exists overwrite? (y/n): ", file_name);
	  echo ();
	  do {
	  retval = tolower (wgetch (prompt_area));
	  if ((retval == 'y') || (retval == 'n'))
	    break;
	  } while (1);
	  noecho ();
	  wclear (prompt_area);
	  if (retval == 'y')
	  {
	    retval = machine_write (&machine, file_name, OVERWRITE);
	    if (retval == SAVED)
	    {
	      mvwprintw (prompt_area, 0, 0, "file \"%s\" saved", file_name);
	    }
	    else if (retval == ERROR)
	    {
	      mvwprintw (prompt_area, 0, 0, "cannot write \"%s\"", file_name);
	    }
	  }
	  else
	  {
	    mvwprintw (prompt_area, 0, 0, "file \"%s\" unmodified", file_name);
	  }
	}
	file_name[0] = '\0';
	break;
	
      case 'r':
	wclear (prompt_area);
	tm_clear_flags ();
	tm_clear_regs ();
	mvwprintw (prompt_area, 0, 0, "regs and flags cleared");
	update_regs (regs, start_addr);
	break;
	
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	val = val - '0';
	flag = 2;
	break;
	
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
	val = val - 'a' + 10;
	flag = 2;
	break;
	
      case KEY_F(1):
      case 'h':
	help ();
	
	break;
	
      case KEY_F(12):
      case 27://esc
	wclear (prompt_area);
	mvwprintw (prompt_area, 0, 0, "exit ? (y/n): ");
	echo ();
	do {
	 retval = tolower (wgetch (prompt_area));
	 if ((retval == 'y') || (retval == 'n'))
	   break;
	} while (1);
	noecho ();
	if (retval == 'y')
	  goto outside;
	else
	  wclear (prompt_area);
	  break;
	
      default:
	flag = 0;
    }
    
    if (flag == 2)
    {
      if (high_low)
	    tm_set_memory (mloc, (tm_fetch_memory (mloc) & 0x0f) | ((val & 0xff) << 4));
      else
	    tm_set_memory (mloc, (tm_fetch_memory (mloc) & 0xf0) | (val & 0x0f));
      high_low = !high_low;
      flag = 1;
    }
    else 
      high_low = 1;
    
    /* Not needed because mloc is u16int_t the val automatically wraps around */
    /*if (mloc >= MAX_MEM)
	  mloc = 0;
    else if (mloc < 0)
      mloc = MAX_MEM - 1;
    */

  }
  outside:
  
  echo ();
  nocbreak ();
  endwin ();
  return 0;
}
