typedef struct _ProcessRec {
    char *procName;
    void (*proc)(int argc, char **argv);
} ProcessRec;

extern ProcessRec nativeProcessList[];

