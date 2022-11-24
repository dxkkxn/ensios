#include "segment.h"
#include <cpu.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>

#include "horloge.h"
#include "ecran.h"
#include "gestion_processus.h"

#include <tinyalloc.h>


// typedef enum { false, true } bool;



// process_t process[2];

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

extern linked_list_t process_list;
extern node_t * curr_node;

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
    sti();
    hlt();
    cli();

    //ordonnance();
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

void traitant_IT_32(void);
void kernel_start(void) {
  efface_ecran();
  init_traitant_IT(32, traitant_IT_32);
  masque_IRQ(0, 0);
  gestion_horloge();
  write_hour();
  sti();
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
   while (1) {
    // cette fonction arrete le processeur
    hlt();
  }
}
