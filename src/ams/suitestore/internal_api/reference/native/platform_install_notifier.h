/**
 * Inform on installation of a new Midlet.
 *
 * Java invokes this function when an installation of a new
 * Midlet completes successfully.
 * Allows the platform to create shortcuts for the installed Midlets which enables one click
 * launch of a Midlet
 *
 * @param midletName     Midlet name string (as appears in the *.jad file)
 * @param className       First Midlet class name (as appears in the *.jad file)
 * @param suiteID            Midlet Suite ID
 * @param midletIcon       Icon file name to use (It should be valid icon path or NULL)
 */

void platform_lifecycle_install(
     jchar *midletName,
     jsize midletNameLen,
     jchar *className,
     jsize classNameLen,
     int  suiteID,
     jchar* midletIcon,
     jsize midletIconLen
);


/**
 * Inform on uninstallation of a Midlet.
 *
 * @param suiteID            Midlet Suite ID
 */

void platform_uninstall_notify(int suiteId);
