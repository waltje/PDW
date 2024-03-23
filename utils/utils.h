#ifndef PDW_UTILS_H
# define PDW_UTILS_H


extern char szCurrentDate[40];
extern char szCurrentTime[40];


#ifdef __cplusplus
extern "C" {
#endif

void GetFileName(const char *filepath, char *buffer, int buffersize);
void GetFilePath(const char *filepath, char *buffer, int buffersize);
void GetFileNameWithoutExtension(char *filepath);
char *lowercase(char *s);
int FileExists(const char *file);
void Get_Date_Time(void);
void ChangeFileExtension(char *filename, char *extension);
char *GetRightMostChars(char *string, char *right_string, int num);

#ifdef __cplusplus
}
#endif


#endif	/*PDW_UTILS_H*/
