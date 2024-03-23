#ifndef PDW_LANG_H
# define PDW_LANG_H


#ifdef __cplusplus
extern "C" {
#endif

extern char lang_name[10][40];
extern int lang_rev[10];
extern int lang_cnt;


void read_language_tables(void);
void free_lang_tables(void);
char remap_ch(char c);
int in_lang_tbl(char c);

#ifdef __cplusplus
}
#endif


#endif	/*PDW_LANGUAGE_H*/

