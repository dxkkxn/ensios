#ifndef HORLOGE_H_
#define HORLOGE_H_

#include <inttypes.h>
#include <stdbool.h>

void gestion_horloge(void);
void masque_IRQ(uint32_t num_IRQ, bool masque);
void write_hour(void);

void init_traitant_IT(uint32_t num_IT, void (*traitant)(void));
#endif // HORLOGE_H_
