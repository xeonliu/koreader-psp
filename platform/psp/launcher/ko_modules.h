#ifndef KO_MODULES_H
#define KO_MODULES_H
struct ko_module;
void ko_modules_set_dir(const char *dir);
const char *ko_modules_last_error(void);
void *ko_resolve(const char *name);
struct ko_module *ko_module_load(const char *name);
struct ko_module *ko_module_for_library(const char *libname);   /* NULL + empty error: not a module library */
void *ko_module_symbol(struct ko_module *m, const char *name);
#endif
