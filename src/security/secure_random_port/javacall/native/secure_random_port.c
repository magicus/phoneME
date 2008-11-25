
#include <java_types.h>
#include <secure_random_port.h>

jint get_random_bytes_port(unsigned char*buffer, jint size) {
       return javacall_random_get_seed(buffer, size);
}


