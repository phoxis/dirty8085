/* WARNING: very messy coode and goto */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "8085.h"

u16int_t label_count = 0;
struct _label_table {
  char label[32];
  u16int_t addr;
} label_table[128];


void remove_space (char *buffer)
{
  int i, k=0;
  char result[128];
  
  for (i=0; (buffer[i] != '\0') && (buffer[i] == ' '); i++)
    ;
  
  while (1)
  {
    i--;
    do {
      i++;
      if (buffer[i] == ';')
      {
	result[k++] = '\0';
	break;
      }
      result[k++] = buffer[i];
    } while ((buffer[i] != '\0') && (buffer[i] != ' '));
    
    if ((buffer[i] == '\0') || (buffer[i] == ';'))
      break;
    
    while ((buffer[i] != '\0') && (buffer[i] == ' '))
      i++;
  }
  strcpy (buffer, result);
}

int main (void)
{
  char buffer[128], opcode[5], operand_1[6], operand_2[6];
  u16int_t rel_addr = 0;
  
  while (1)
  {
    int i = 0, j, k, mark1, mark2, incr_addr = 1, is_label = 0;
    
    opcode[0] = '\0';
    operand_1[0] = '\0';
    operand_2[0] ='\0';
    
    printf ("\nEnter: ");
    scanf (" %[^\n]", buffer);
    remove_space (buffer);
//     printf ("\n\n%s", buffer);

    if (strcmp (buffer, "x") == 0)
      break;
    
    mark1 = i;
    while ((buffer[i] != '\0') && isalnum (buffer[i]))
      i++;
    mark2 = i;
    
    /* label */
    if ((buffer[i] == ':') || ((buffer[i] == ' ') && (buffer[i + 1] == ':')))
    {
      for (k = 0, j = mark1; j < mark2; j++, k++)
	label_table[label_count].label[k] = buffer[j];
      label_table[label_count].addr = rel_addr;
      label_count++;
      
      /* should not increase rel_addr if only the label is in the string */
      incr_addr = 0;
      is_label = 1;
      
      i++;
      if (buffer[i] == ':')
	i++;
      if (buffer[i] == ' ')
	i++;
      if (isalpha (buffer[i]))
      {
	incr_addr = 1;
	mark1 = i;
	while ((buffer[i] != '\0') && isalnum (buffer[i]))
	  i++;
	mark2 = i;
      }
    }
    /* instruction (can be in the same line) */
    if (!((buffer[i] == ':') || ((buffer[i] == ' ') && (buffer[i + 1] == ':'))))
    {
      for(k = 0, j = mark1; j < mark2; j++, k++)
	opcode[k] = buffer[j];
      opcode[k] = '\0';      
    }
    
    if (buffer[i] != '\0')
    {
      if (buffer[i] == ' ')
	i++;
      mark1 = i;
      while ((buffer[i] != '\0') && isalnum (buffer[i]))
	i++;
      mark2 = i;
      for(k = 0, j = mark1; j < mark2; j++, k++)
	operand_1[k] = buffer[j];
      operand_1[k] = '\0';
    }
    if (buffer[i] != '\0')
    {
      if (buffer[i] == ' ')
	i++;
      if (buffer[i] == ',')
      {
	i++;
	if (buffer[i] == ' ')
	  i++;
	mark1 = i;
	while ((buffer[i] != '\0') && isalnum (buffer[i]))
	  i++;
	mark2 = i;
	for(k = 0, j = mark1; j < mark2; j++, k++)
	  operand_2[k] = buffer[j];
	operand_2[k] = '\0';
      }
    }
    
    /* match label in the 1st operand in case of jump ins and replace address */
    /* NOTE: should we allow labels with names of regs ? */
    for (j = 0; j < label_count; j++)
    {
      if (strcmp (operand_1, label_table[label_count].label) == 0)
      {printf ("asdf");
	sprintf (operand_1, "%x", label_table[label_count].addr);
      }
    }
    if (is_label == 1)
      printf ("|%s| {%x} %s %s, %s", label_table[label_count].label, label_table[label_count].addr, opcode, operand_1, operand_2);
    else
      printf ("\n[%0x] %s %s, %s", rel_addr, opcode, operand_1, operand_2);
    if (incr_addr == 1)
      rel_addr++;
    is_label = 0;
  }
  printf ("\n");
  return 0;
}
