#include "segment.h"
#include <cpu.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>

#include <tinyalloc.h>

#define QUARTZ 0x1234DD
#define CLOCKFREQ 50
#define MAX_PROCESS_NAME 32
#define STACK_SIZE 512

void traitant_IT_32(void);
// typedef enum { false, true } bool;

void ctx_sw(uint32_t *ctx1, uint32_t *ctx2);
// on peut s'entrainer a utiliser GDB avec ce code de base
// par exemple afficher les valeurs de x, n et res avec la commande display

// une fonction bien connue
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

uint32_t fact(uint32_t n) {
  uint32_t res;
  if (n <= 1) {
    res = 1;
  } else {
    res = fact(n - 1) * n;
  }
  return res;
}

uint16_t *ptr_mem(uint32_t lig, uint32_t col) {
  return (uint16_t *)(0xB8000 + 80 * 2 * lig + col * 2);
}

uint32_t LIG, COL;
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

int next_mult8(int x) {
  int i = 0;
  while (8 * i < COL) {
    i++;
  }
  return 8 * i;
}

void traite_car(char c) {
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

void defilement(void) {
  unsigned int *new_line = (unsigned int *)(0xB8000 + 80 * 2);
  memmove((unsigned int *)0xB8000, new_line, 80 * 24 * 2);
  place_curseur(LIG - 1, COL);
}
void defilement_respect_hour(void) {
  unsigned int *new_start = (unsigned int *)(0xB8000 + 80 * 2);
  unsigned int *new_line = (unsigned int *)(0xB8000 + 80 * 2 * 2);
  memmove(new_start, new_line, 80 * 23 * 2);
  place_curseur(LIG - 1, COL);
}

void console_putbytes(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    traite_car(s[i]);
  }
}

uint8_t hour = 0;
uint8_t min = 0;
uint8_t sec = 0;

void write_hour(void) {
  place_curseur(0, 80 - 8);
  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", hour, min, sec);
  console_putbytes(buffer, 8);
  place_curseur(1, 0);
}
void increment_hour() {
  sec++;
  if (sec == 60) {
    min++;
    sec = 0;
  }
  if (min == 60) {
    hour++;
    min = 0;
  }
  if (hour == 24) {
    hour = 0;
  }
}

uint8_t tics = 0;
void tic_PIT(void) {
  outb(0x20, 0x20);
  tics++;
  if (tics == 50) {
    tics = 0;
    increment_hour();
    write_hour();
  }
  return;
}

typedef enum { SELECTED, ACTIVABLE } state_t;
typedef struct process {
  int pid;
  char name[MAX_PROCESS_NAME];
  state_t state;
  uint32_t ctx[5];
  uint32_t stack[STACK_SIZE];
} process_t;

typedef struct node_t {
  process_t *process;
  struct node_t *next;
} node_t;

typedef struct LinkedList {
  node_t *head;
  node_t *queue;
} LinkedList;
LinkedList process_list = {NULL, NULL};
// process_t process[2];
node_t *curr_node;

// char *mon_nom() {
//   if (process[0].state == SELECTED)
//     return process[0].name;
//   return process[1].name;
// }
// int mon_pid() {
//   if (process[0].state == SELECTED)
//     return process[0].pid;
//   return process[1].pid;
// }

void push_head(LinkedList *pl, node_t *current) {
  current->next = pl->head;
  pl->head = current;
}

node_t *pop_head(LinkedList *pl) {
  node_t *head = pl->head;
  pl->head = pl->head->next;
  assert(head != NULL);
  return head;
}

void push_queue(LinkedList *pl, node_t *current) {
  if (pl->queue == NULL && pl->queue == NULL) {
    // empty list
    pl->head = current;
    pl->queue = current;
    current->next = NULL;
    return;
  }
  pl->queue->next = current;
  current->next = NULL;
  pl->queue = current;
}

void ordonnance() {
  // int old_process = curr_process;
  // curr_process = (curr_process + 1) % 2;
  // process[old_process].state = ACTIVABLE;
  // process[curr_process].state = SELECTED;
  // ctx_sw(process[old_process].ctx, process[curr_process].ctx);
  //
  node_t *old_node = curr_node;
  node_t *curr_node = pop_head(&process_list);
  push_queue(&process_list, old_node);
  old_node->process->state = ACTIVABLE;
  curr_node->process->state = SELECTED;
  ctx_sw(old_node->process->ctx, curr_node->process->ctx);
}
// void idle_func(void) {
//   // for (int i = 0; i < 3; i++) {
//   //   printf("[idle] je tente de passer la main a proc1...\n");
//   //   ctx_sw(process[0].ctx, process[1].ctx);
//   // }
//   // printf("[idle] je bloque le systeme\n");
//   // hlt();
//   for (;;) {
//     printf("[%s] pid = %i\n", curr_node->process->name,
//     curr_node->process->pid); ordonnance();
//   }
// }

int process_count = 0;
int32_t create_process(void (*func)(void), char *name) {
  process_t *new = malloc(sizeof(process_t));
  new->pid = process_count;
  strcpy(new->name, name);
  new->state = ACTIVABLE;
  process_count++;
  new->stack[511] = (uint32_t)*func;
  new->ctx[1] = (uint32_t) & new->stack[511];

  node_t *new_node = malloc(sizeof(node_t));
  new_node->process = new;
  new_node->next = NULL;
  push_queue(&process_list, new_node);
  return process_count - 1;
}

void process_func(void) {
  process_t *process = curr_node->process;
  for (;;) {
    printf("[%s] pid = %i\n", process->name, process->pid);
    ordonnance();
  }
}
// void proc1_func(void) {
//   // while (1) {
//   //   printf("[proc1] idle m'a donne la main\n");
//   //   printf("[proc1] je tente de lui la redonner\n");
//   //   ctx_sw(process[1].ctx, process[0].ctx);
//   // }
//   for (;;) {
//     printf("[%s] pid = %i\n", mon_nom(), mon_pid());
//     ordonnance();
//   }
// }

void kernel_start(void) {
  // process_t idle = {0, "idle", SELECTED};
  // process_t proc1 = {1, "proc1", ACTIVABLE};
  // process[0] = idle;
  // process[1] = proc1;
  //(void)(idle);
  //(void)(proc1);
  // process[1].stack[511] = (uint32_t)proc1_func;
  // process[1].ctx[1] = (uint32_t)&process[1].stack[511];
  create_process(&process_func, "idle");
  create_process(&process_func, "proc1");
  create_process(&process_func, "proc2");
  create_process(&process_func, "proc3");
  create_process(&process_func, "proc4");
  create_process(&process_func, "proc5");
  create_process(&process_func, "proc6");
  create_process(&process_func, "proc7");
  create_process(&process_func, "proc8");
  curr_node = pop_head(&process_list);
  curr_node->process->state = ACTIVABLE;
  efface_ecran();
  process_func();
  //  idle_func();

  // process[0].pid = 0;
  //// process[0].name = "idle";
  // process[0].state = SELECTED;
  // process[1].pid = 1;
  //// process[1].name = "proc1";
  // process[1].state = ACTIVABLE;
  // process[1].stack[511] = (uint32_t)proc1_func;
  // process[1].ctx[1] = (uint32_t)&process[1].stack[511];
  // idle_func();
  //  uint32_t x = fact(5);
  //   quand on saura gerer l'ecran, on pourra afficher x
  //   uint16_t *res = ptr_mem(0, 0);
  //   uint16_t *res2 = ptr_mem(0, 1);
  //(void)res;
  //(void)res2;
  //(void)x;
  //  ecrit_car(0, 0, 'a');
  //  ecrit_car(0, 1, 'b');
  //  init_traitant_IT(32, traitant_IT_32);
  //  gestion_horloge();
  //  masque_IRQ(0, 0);
  //  efface_ecran();
  //  write_hour();
  //  write_hour();
  //  sti();
  //  traite_car('1');
  //  traite_car('2');
  //  traite_car('3');
  //  traite_car('4');
  //  traite_car('\n');
  //  traite_car('5');
  //  traite_car('6');
  //  traite_car('7');
  //  defilement();
  //  printf("xxxx");
  //  defilement_respect_hour();
  //   place_curseur(0, 0);
  //      on ne doit jamais sortir de kernel_start
  //  while (1) {
  //   // cette fonction arrete le processeur
  //   hlt();
  // }
}
