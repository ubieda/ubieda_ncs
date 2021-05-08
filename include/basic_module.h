#ifndef _BASIC_MODULE_H_
#define _BASIC_MODULE_H_

int basic_module_init(void);
void basic_module_destroy(void);
int basic_module_read(void);
void basic_module_write(int val);

#endif /* _BASIC_MODULE_H_ */