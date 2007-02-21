An entry point of the runNams, listMidlets, removeMidlet and fileInstaller executables
is defined in this library.
This entry point is platform-dependent, for the provided implementations it is:

default/
int main(int argc, char** commandlineArgs);

javacall/
void JavaTask(void);
