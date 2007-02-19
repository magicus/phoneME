An entry point of the runMidlet application is defined in this library.
This entry point is platform-dependent, for the provided implementations it is:

default/
int main(int argc, char** commandlineArgs);

javacall/
void JavaTask(void);

wince/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPWSTR lpCmdLine, int nShowCmd);
