#include <kni.h>
#include <javacall_lifecycle.h>

void platform_lifecycle_install_notify(
     jchar *midletName,
     jsize midletNameLen,
     jchar *className,
     jsize classNameLen,
     int  suiteID,
     jchar* midletIcon,
     jsize midletIconLen
){
    javacall_lifecycle_install(
     midletName,
     midletNameLen,
     className,
     classNameLen,
     suiteID,
     midletIcon,
     midletIconLen
     );
}



void platform_lifecycle_uninstall_notify(int suiteId) {
        javacall_lifecycle_uninstall(
            suiteId
        );
}
