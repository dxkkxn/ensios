#ifndef ECRAN_H_
#define ECRAN_H_
#include <inttypes.h>

void efface_ecran(void);
void traite_car(char c);
void place_curseur(uint32_t lig, uint32_t col);
void console_putbytes(const char *s, int len);

#endif // ECRAN_H_
