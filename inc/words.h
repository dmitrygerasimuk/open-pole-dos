#ifndef WORDS_H
#define WORDS_H

#include "typedefs.h"

 // Загружает случайное слово+тему из pole.ovl, запоминает выбранный индекс для g_round_wins,
 //  и приводит слово/тему к uppercase
int words_load_word_and_randomize(void);

#endif
