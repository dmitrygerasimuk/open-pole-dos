/* globals.h */
#ifndef GLOBALS_H
#define GLOBALS_H


#include "typedefs.h"   

/* GLOBAL SETTINGS */
 
#define UI_HOF_REC_SIZE    ((u16)13u)
 

typedef enum  {
    GAME_STATUS_HUMAN_WON  = 0u,   
    GAME_STATUS_HUMAN_LOST = 1u,   
} UiGameStatus;

typedef enum PlayerId {
    PLAYER_NONE  = 0u,
    PLAYER_1     = 1u,
    PLAYER_2     = 2u,
    PLAYER_HUMAN = 3u
} PlayerId;



extern PlayerId g_active_player;    // кто сейчас ходит
extern s16 g_score_table[4];   // очки игроков, нулевой элемент - буфер (сколько очков выпало на барабане или шкатулка)

extern u16 g_score_x[4];        // таблица с координатами X для спрайтов с очками
extern u16 g_score_y[4];
extern u16 g_player_bubble_x[4]; // таблица X для пузырей с буквами над игроками

typedef struct WordState {
    s16 match_count;        // сколько совпадений у текущей буквы в слова
    u16 opened[20];        // массив флагов открытых букв (0..len-1) - 1-based
    u16 found[20];        // позиции найденных букв (1-based)
} WordState;
extern WordState g_word_state;


extern u8 g_current_letter; 
extern u8  g_mPressedKey;
extern u16 g_space_pressed;
extern  s16 g_mouse_present;
extern  u16 g_mouse_buttons;


extern u16 g_soundONOF_soundMultiplier;   // это и умножитель для эффектов, но и ипользуется для mute (=0)


extern u16 g_used_letter[35]; // какие буквы уже использованы (0/1)

extern u16 g_round_wins;    // номер раунда    
extern u16 g_used_word_idx[3]; // Таблица уже использованных индексов слов по раундам: [0],[1],[2]
                               // (в дизасме это  ds: 06AA,06AC,06AE). 

extern u8  g_buffer_string[64];       // временный буфер для строк (обычно таv слово)
extern u8  g_buffer_string_ovl[64];    // а тут - тема 

/* Куда кладём результат */
extern char g_current_game_word[64];   // загаданное слово

#define WORD_MAX_LEN ((size_t)20u)  

#define G_WORDLEN strlen(g_current_game_word) > WORD_MAX_LEN ? WORD_MAX_LEN : (u16)strlen(g_current_game_word)
 


 
 
extern char g_answer_entered[21];

extern u16 g_wheel_anim_counter;       //  ds:0970  
extern u16 g_wheelSector;            //  ds:0972  
 
 
extern u16 g_player_shuffle[3*2+1];  // массив для перемешивания игроков (1-based)

extern const u8 kAlphabet32Pas[33];
 extern u16 g_guessed_letters_right;  // если три подряд - шкатулка
 extern u16 g_game_status_zeroifwon;  
 // 0 — обычный ход продолжаетс /  человек выиграл раунд
 // 1 — проигрыш/вылет (не человек выиграл) или введен неправильный ответ
 // 2 — выбираем взять приз
 // 3 - приз или деньги


extern u8  g_baraban_lut_table_xy_inited; // статус таблицы XY для барабана

typedef struct OpponentProfile {
    u16 sprite_id_word;   // 0x0015,0x0016,0x0030... */
    const char *name;     // CP866 C-string  
} OpponentProfile; 


extern OpponentProfile g_opponents[]; // массив оппонентов - имя и номер спрайта 
u16 g_opponents_count(void); // размер этой таблицы



 #define UI_KEYMAP_LEN   34u
extern const u8 g_ui_keymap_src[UI_KEYMAP_LEN];   // таблицы для маппинга клавиш
extern  const u8 g_ui_keymap_dst[UI_KEYMAP_LEN];  // при любом вводе с клавиатуры
 
void dbg_dump_globals(void);





// my addons
extern u8 g_addon_mono_palette;
 
extern u16 g_seed;
extern u16 g_run_tests;
 



// pole.c 
 


#endif

