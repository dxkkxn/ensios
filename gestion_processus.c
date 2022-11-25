#include "gestion_processus.h"
#include "segment.h"
#include <inttypes.h>
#include <tinyalloc.h>
#include <debug.h>

#include <inttypes.h>
extern int system_time;
linked_list_t process_list = {NULL, NULL};
linked_list_t slept_process = {NULL, NULL};
linked_list_t diying_process = {NULL, NULL};
node_t *curr_node;
void ctx_sw(uint32_t *ctx1, uint32_t *ctx2);

void push_head(linked_list_t *pl, node_t *current) {
  current->next = pl->head;
  pl->head = current;
}

node_t *pop_head(linked_list_t *pl) {
  node_t *head = pl->head;
  pl->head = pl->head->next;
  if (pl->head == NULL)
    pl->queue = NULL;
  assert(head != NULL);
  return head;
}

void push_queue(linked_list_t *pl, node_t *current) {

  if (pl->head == NULL) {
    assert(pl->queue == NULL);
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

void delete(linked_list_t* list, node_t * to_delete) {
  node_t* curr = list->head;
  node_t* prev = NULL;
  while(curr != NULL) {
    if (curr == to_delete) {
      if (prev == NULL) {
        list->head = list->head->next;
        if (list->head == NULL)
          list->queue =  NULL;
      }
      else {
        prev->next = curr->next;
        if (list->queue == curr)
          list->queue = prev;
      }
      break;
    }
    prev = curr;
    curr = curr->next;
  }
}

void dors(uint32_t secs) {
  curr_node->process->wake_time = system_time+secs;
  curr_node->process->state = SLEPT;
  push_queue(&slept_process, curr_node);
  ordonnance();
}

void fin_processus() {
  curr_node->process->state = DIYING;
  push_queue(&diying_process, curr_node);
  ordonnance();
}

void wake_slept_process(linked_list_t *sp) {
  node_t* curr = sp->head;
  while(curr != NULL) {
    if (curr->process->wake_time <= system_time) {
      delete(sp, curr);
      curr->process->state = ACTIVABLE;
      push_queue(&process_list, curr);
    }
    curr = curr->next;
  }
}

void free_diying_process() {
  node_t * curr = diying_process.head;
  node_t * next;
  while (curr != NULL) {
    next = curr->next;
    free(curr);
    curr = next;
  }
}

void ordonnance() {
  // int old_process = curr_process;
  // curr_process = (curr_process + 1) % 2;
  // process[old_process].state = ACTIVABLE;
  // process[curr_process].state = SELECTED;
  // ctx_sw(process[old_process].ctx, process[curr_process].ctx);

  wake_slept_process(&slept_process);
  if (process_list.head == NULL)
    return;
  node_t *old_node = curr_node;
  curr_node = pop_head(&process_list);
  curr_node->process->state = SELECTED;
  if (old_node->process->state == SELECTED) {
    old_node->process->state = ACTIVABLE;
    push_queue(&process_list, old_node);
  }
  ctx_sw(old_node->process->ctx, curr_node->process->ctx);
  free_diying_process();
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
