/* pole_dialog.c */
// THIS FILE SHOULD ALWAYS BE ENCODED IN CP866
// ВАЖНО! ДОЛЖЕН БЫТЬ В CP866 (DOS-кодировке)


/*
это помогает не перетереть жтот файл utf-8
.vscode/settings.json --> 
        {
        8
        "files.encoding": "utf8",

        
        "files.autoGuessEncoding": true,

        
        "files.encodingOverride": {
            "src/pole_dialog.c": "cp866"
        }
         }% 
*/

#include "pole_dialog.h"


const char cKAPITALSHOW[] = 
    " КАПИТАЛШОУ";

const char sdelalDimaBashurov[] =
    "Сделал Дима Башуров из Арзамаса-16 (E-Mail: 0669@RFNC.NNOV.SU )";

const char end_autorDimaBashurov[] =
    "Автор Дима Башуров из Российского Федерального Ядерного Центра";

const char end_telephonArzamas[] =
    "Телефон в Арзамасе-16 : (831-30) 5-92-73   E-mail: 0669 @ RFNC. NNOV. SU";

const char telephonString[] =
    "Телефон в Арзамасе-16: (83130) 5-92-73";

const char posvDruzyam[] =
    "Посвящается друзьям";

const char spravka1[] =
    "СПРАВКА: Для перемещения своей руки использйте клавиши со стрелками или";

const char spravka2[] =
    "\"мышку\". Ввод осуществляется нажатием на клавишу ПРОБЕЛ или на";

const char spravka3[] =
    "левую кнопку\"мышки\". Нажатие <Ctrl+S> включает/выключает звук,";

const char spravka4[] =
    "если пришел начальник, нажми клавишу TAB, ESC - выход из игры!";

/* CP866 C-strings */
const char STR_PERVIY[]             = "Первый";
const char STR_VTOROI[]             = "Второй";
const char STR_TRETII[]             = "Третий";
const char STR_IGROK_EXCL[]         = " игрок!";
const char STR_IGROK[]              = " игрок";

const char STR_VRASHCHAYTE[]        = "Вращайте барабан!";
const char STR_U_VAS[]              = "У вас ";
const char STR_OCHKOV_EXCL[]        = " очков!";
const char STR_NAZOVITE[]           = "Назовите букву!";
const char STR_U_VAS_0[]            = "У вас 0 очков..";
const char STR_UVY_PEREHOD[]        = "Увы! Переход хода..";
const char STR_VSE_DENGI_SGORELI[]  = "Все деньги сгорели!";
const char STR_DENGI_UDVAIV[]       = "Деньги удваиваются!";
const char STR_SEKTOR_PLUS[]        = "Сектор ПЛЮС!";
const char STR_OTKROY_LYUBUYU[]     = "Открой любую букву!";
const char STR_SEKTOR_PRIZ[]        = "Сектор ПРИЗ!";
const char STR_PRIZ_ILI_IGRAEM[]    = "Приз или играем?";

const char STR_EST_TAKAYA[]         = "Есть такая буква!";
const char STR_BRAVO2[]             = "Браво!!";
const char STR_NET_TAKOI[]          = "Нет такой буквы!";
const char STR_PEREHOD_HODA[]       = "Переход хода..";

const char STR_VYIGRAL_RAUND[]      = "выиграл раунд!";
const char STR_ZA_3_BUKVY_PREMIYA[] = "За 3 буквы премия!";
const char STR_VNESITE_SHKATULKI[]  = "Внесите шкатулки!";
const char STR_GDE_DENGI[]          = "Где деньги?";
const char STR_VYBIRAYTE[]          = "Выбирайте!";
const char STR_BRAVO3[]             = "Браво!!!";
const char STR_VY_OTGADALI[]        = "Вы отгадали!";
const char STR_UVY_ETA[]            = "Увы! Эта";
const char STR_SHKATULKA_PUSTA[]    = "шкатулка пуста!";
const char STR_ZAGADANO_SLOVO[]     = "Загадано слово:";
const char STR_NACHINAEM_IGRU[]     = "Начинаем игру!";
const char STR_TEMA[]               = "Тема:";

const char STR_VY_SOVERSHENNO[]     = "Вы совершенно";
const char STR_PRAVY[]              = "правы!!";
const char STR_NEPRAVILNO_VY[]      = "Неправильно! Вы";
const char STR_POKIDAETE[]          = "покидаете игру!";
const char STR_POZDRAVLYAYU[]       = "Поздравляю! Вы";
const char STR_VYIGRALI_FINAL[]     = "выиграли финал!";
const char STR_K_SOZHALENIYU[]      = "К сожалению Вы";
const char STR_POKIDAETE2[]         = "покидаете игру..";
const char STR_ESLI_TAK_TO[]        = "Если так, то";
const char STR_NAZOVITE_BUKVU_DOT[] = "назовите букву.";

const char STR_REKLAMNAYA[]         = "РЕКЛАМНАЯ";
const char STR_PAUZA[]              = "ПАУЗА!";
const char STR_PRIZ_ILI[]           = "ПРИЗ или";
const char STR_RUBLEI_Q[]           = " рублей?";
const char STR_ZABIRAYTE[]          = "Забирайте";
const char STR_SVOI_DENGI[]         = "свои деньги!";
const char STR_PREDSTAVLYAYU[]      = "Представляю";
const char STR_UCHASTNIKOV[]        = "участников!";
const char STR_PRIZ_EXCL[]          = "свой ПРИЗ!";

const char STR_8_LUCHSHIH_IGROKOV[] = "8 лучших игроков,";
const char STR_VIIGR_FINAL[]        = "выигравших ФИНАЛ!";

const char STR_AYA_BUKVA[]          = "-я буква";



const char *STR_STAGE_TITLE_CHETVERTFINAL  = "ЧЕТВЕРТЬФИНАЛ";
const char *STR_STAGE_TITLE_POLUFINAL  = "  ПОЛУФИНАЛ";
const char *STR_STAGE_TITLE_FINAL   = "    ФИНАЛ";


 
// strings for prize.c                                                
 
 
 const char kMoneySto[]       = "СТО";
 const char kMoneyTyshcha[]   = "ТЫЩА";
 const char kMoneyStoTyshch[] = "СТО ТЫЩ";
 const char kMoneyMillion[]   = "МИЛЛИОН";

 const char kPrizeLine1[]  = "Вы выбрали ПРИЗ и мы Вас поздравляем!";
 const char kPrizeLine2[]  = "Фирма ИНТЕРМОДА и ПОЛЕ ЧУДЕС дарит Вам";
 const char kPrizeSuffix[] = " компании PROCTER & GAMBLE!";
 const char kPrizeAddrHdr[] = "За ПРИЗОМ обращайтесь по адресу:";
 const char kPrizeAddr[]    = "101000-Ц, Москва, проезд Серова, 11";
 const char kPrizeNote[]    = "На конверте сделайте пометку КОМПЬЮТЕРНЫЙ ПРИЗ";

 
 const char kPrizeItem0[] = "ЗУБНУЮ ЩЕТКУ";
 const char kPrizeItem1[] = "Порошок ARIEL";
 const char kPrizeItem2[] = "Расчестку для усов";
 const char kPrizeItem3[] = "Зубочистку";
 const char kPrizeItem4[] = "Круг для унитаза";
 const char kPrizeItem5[] = "Рулон мягкой бумаги";
 const char kPrizeItem6[] = "Шнурки для калош";
 const char kPrizeItem7[] = "Подтяжки для носков";
 const char kPrizeItem8[] = "Беруши для ушей";
 const char kPrizeItem9[] = "Пивную открывашку";



 const char kVB_SkazhuKruchu[] = "Скажу   Кручу";
 const char kVB_SlovoBaraban[] = "СЛОВО  БАРАБАН";
 const char kVB_BeruBudu[]     = "Беру   Буду";
 const char kVB_PrizIgrat[]    = "ПРИЗ  ИГРАТЬ";
 const char kVB_BeruBeru[]     = "Беру    Беру";
 const char kVB_PrizDengi[]    = "ПРИЗ   ДЕНЬГИ";


const char str_Perviy_Igrok[]  = "Первый игрок";  
const char str_Vtoroy_Igrok[]  = "Второй игрок";  
const char str_TretiyIgrok[]   = "Третий игрок";  
const char str_Eto_Vy[]        = "это ВЫ!";    

 

const char STR_PYATOCHOK[]  = "ПЯТАЧОК";  
const char STR_VINNIPUH[]   = "ВИННИ-ПУХ";  
const char STR_KROLIK[]     = "КРОЛИК";  
const char STR_IAIA[]       = "ИА-ИА";  
const char STR_KARLSON[]    = "КАРЛСОН";  
const char STR_SOVA[]       = "СОВА";  

 

const char STR_BUKVA[]       = "Буква ";  
const char STR_BUKVA_EXCLAMATION[]       = "!";  



 const char S_COMP_GAME[]   = "Компьютерная игра";
 const char S_SOLD_AT[]     = "продается по адресу";
 const char S_ADDR1[]       = "101000-Ц, МОСКВА,";
 const char S_ADDR2[]       = "проезд Серова, 11.";
 const char S_FIRST25[]     = "25 самых первых";
 const char S_BUYERS[]      = "покупателей будут";
 const char S_INVITED[]     = "приглашены со";
 const char S_FAMILIES[]    = "своими семьями";
 const char S_SHOOTING[]    = "на съемки телеигры";
 const char S_LAST_LINE[]   = "ПОЛЕ ЧУДЕС!";

 const char S_LISTIEV_PIC[]    = "ЛИСТЬЕВ";
  const char S_YAKUBOVICH_PIC[]    = "ЯКУБОВИЧ";
