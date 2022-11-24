#include "ecran.h"

#include <cpu.h> // for outb
#include <inttypes.h>
#include <string.h> // for memmove


uint32_t LIG, COL;
uint16_t *ptr_mem(uint32_t lig, uint32_t col) {
  return (uint16_t *)(0xB8000 + 80 * 2 * lig + col * 2);
}
int next_mult8(int x) {
  int i = 0;
  while (8 * i < COL) {
    i++;
  }
  return 8 * i;
}
void ecrit_car(uint32_t lig, uint32_t col, char c) {
  uint16_t *ptr = ptr_mem(lig, col);
  uint16_t temp = c;
  temp |= 15 << 8;
  *ptr = temp;
}

void efface_ecran(void) {
  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 80; j++) {
      ecrit_car(i, j, ' ');
    }
  }
}

void place_curseur(uint32_t lig, uint32_t col) {
  // D4 port de commande
  // D5 port de donees
  // uint16_t *ptr = ptr_mem(lig, col);
  uint16_t pos = col + lig * 80;
  uint8_t partie_basse = (uint8_t)(pos & 0b11111111UL);
  uint8_t partie_haute = (uint8_t)(pos >> 8);
  outb(0x0F, 0x3D4);
  outb(partie_basse, 0x3D5);
  outb(0x0E, 0x3D4);
  outb(partie_haute, 0x3D5);
  LIG = lig;
  COL = col;
}

void clean_last_line() {
  for (int j = 0; j < 80; j++) { // delete the chars of the last line
    ecrit_car(24, j, ' ');
  }
}
void defilement(void) {
  unsigned int *new_line = (unsigned int *)(0xB8000 + 80 * 2);
  memmove((unsigned int *)0xB8000, new_line, 80 * 24 * 2);
  clean_last_line();
  /* place_curseur(LIG, COL); */
}
void defilement_respect_hour(void) {
  unsigned int *new_start = (unsigned int *)(0xB8000 + 80 * 2);
  unsigned int *new_line = (unsigned int *)(0xB8000 + 80 * 2 * 2);
  memmove(new_start, new_line, 80 * 23 * 2);
  clean_last_line();
}

void traite_car(char c) {
  if (LIG >= 25) {
    defilement_respect_hour();
    place_curseur(24, 0);
  }
  switch (c) {
  case '\b':
    if (COL != 0)
      place_curseur(LIG, COL - 1);
    break;
  case '\t':
    place_curseur(LIG, next_mult8(COL));
    break;
  case '\n':
    place_curseur(LIG + 1, 0);
    break;
  case '\f':
    efface_ecran();
    place_curseur(0, 0);
    break;
  case '\r':
    place_curseur(LIG, 0);
    break;
  default:
    ecrit_car(LIG, COL, c);
    place_curseur(LIG, COL + 1);
  }
}

void console_putbytes(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    traite_car(s[i]);
  }
}
