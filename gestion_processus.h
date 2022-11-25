#ifndef GESTION_PROCESSUS_H
#define GESTION_PROCESSUS_H

#define MAX_PROCESS_NAME 32
#define STACK_SIZE 512
#include <inttypes.h>

typedef enum { SELECTED, ACTIVABLE, SLEPT, DIYING} state_t;
typedef struct process {
  int pid;
  char name[MAX_PROCESS_NAME];
  state_t state;
  uint32_t ctx[5];
  uint32_t stack[STACK_SIZE];
  uint32_t wake_time;
} process_t;

typedef struct node_t {
  process_t *process;
  struct node_t *next;
} node_t;

typedef struct linked_list_t {
  node_t *head;
  node_t *queue;
} linked_list_t;

void push_head(linked_list_t *pl, node_t *current);
node_t *pop_head(linked_list_t *pl);
void push_queue(linked_list_t *pl, node_t *current);
void ordonnance();
void dors(uint32_t secs);
void fin_processus();
#endif // GESTION_PROCESSUS_H
