#include "horloge.h"

#include <cpu.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "ecran.h"
#include "segment.h"
#include "gestion_processus.h"

#define QUARTZ 0x1234DD
#define CLOCKFREQ 50



void gestion_horloge(void) {
  outb(0x43, 0x43);
  uint16_t to_send = (QUARTZ / CLOCKFREQ) % 256;
  outb(to_send, 0x40);
  outb((QUARTZ / CLOCKFREQ) >> 8, 0x40);
}

void masque_IRQ(uint32_t num_IRQ, bool masque) {
  uint8_t res = inb(0x21);
  if (masque) {
    res |= 1 << num_IRQ;
  } else {
    res &= ~(1 << num_IRQ);
  }
  outb(res, 0x21);
}

int system_time= 3600;
extern uint32_t LIG, COL;
void write_hour(void) {
  uint32_t old_lig = LIG;
  uint32_t old_col = COL;
  place_curseur(0, 80 - 8);
  uint8_t hour = system_time/3600;
  uint8_t min = (system_time%3600)/60;
  uint8_t sec_to_display = (system_time%3600)%60;
  char buffer[13];
  sprintf(buffer, "%02u:%02u:%02u", hour, min, sec_to_display);
  console_putbytes(buffer, 8);
  place_curseur(old_lig, old_col);
}

void init_traitant_IT(uint32_t num_IT, void (*traitant)(void)) {
  // initialisation de la table des vecteurs
  uint32_t *addr = (uint32_t *)(0x1000 + 8 * num_IT);
  uint32_t premier_mot = 0;
  premier_mot |= KERNEL_CS << 16;
  premier_mot |= (uint32_t)traitant & 0xffff; //((1UL << 16UL) - 1UL);
  uint32_t deuxieme_mot = 0;
  deuxieme_mot |= (uint32_t)traitant & 0xffff0000; //~((1UL << 16UL) - 1UL);
  deuxieme_mot |= 0x8E00;
  *addr = premier_mot;
  addr++;
  *addr = deuxieme_mot;
}
uint8_t tics = 0;
void tic_PIT(void) {
  outb(0x20, 0x20);
  tics++;
  if (tics == 50) {
    tics = 0;
    system_time++;
    write_hour();
  }
  ordonnance();
  return;
}
