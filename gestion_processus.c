#include "gestion_processus.h"
#include "segment.h"
#include <tinyalloc.h>
#include <debug.h>

#include <inttypes.h>
linked_list_t process_list = {NULL, NULL};
node_t *curr_node;
void ctx_sw(uint32_t *ctx1, uint32_t *ctx2);

void push_head(linked_list_t *pl, node_t *current) {
  current->next = pl->head;
  pl->head = current;
}

node_t *pop_head(linked_list_t *pl) {
  node_t *head = pl->head;
  pl->head = pl->head->next;
  assert(head != NULL);
  return head;
}

void push_queue(linked_list_t *pl, node_t *current) {
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
  curr_node = pop_head(&process_list);
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
