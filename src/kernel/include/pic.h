#ifndef _PIC_H_
#define _PIC_H_

#include "types.h"

void pic_remap(void);
void pic_send_eoi(uint8_t irq);

#endif
