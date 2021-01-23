#ifndef __CUTILS_PROPERTIES_H
#define __CUTILS_PROPERTIES_H

#ifdef __cplusplus
extern "C" {
#endif

#define PROPERTY_KEY_MAX   32
#define PROPERTY_VALUE_MAX  92

int property_get(const char *key, char *value, const char *default_value);

int property_set(const char *key, const char *value);

int property_list(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie);

#ifdef HAVE_SYSTEM_PROPERTY_SERVER

#define SYSTEM_PROPERTY_PIPE_NAME       "/tmp/android-sysprop"

enum {
    kSystemPropertyUnknown = 0,
    kSystemPropertyGet,
    kSystemPropertySet,
    kSystemPropertyList
};
#endif

#ifdef __cplusplus
}
#endif

#endif
