#include <keymap_invalid_key_port.h>
#include <keymap_input.h>

int
is_invalid_key_code_port(int keyCode)
{
    return (keyCode <= KEYMAP_KEY_MACHINE_DEP);
}
