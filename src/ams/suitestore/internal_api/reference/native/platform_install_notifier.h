void platform_lifecycle_install(
     jchar *midletName,
     jsize midletNameLen,
     jchar *className,
     jsize classNameLen,
     int  suiteID,
     jchar* midletIcon,
     jsize midletIconLen
);

void platform_uninstall_notify(int suiteId);
