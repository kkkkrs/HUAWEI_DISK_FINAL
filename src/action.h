#include "manager.h"
#ifndef ACTION_H
#define ACTION_H

void project_init();

void timestamp_action();

void delete_action(Manager &MAN);

void write_action(Manager &MAN);

void read_action(Manager &MAN);

void change_action(Manager &MAN);

void obj_tag_action(Manager &MAN);

#endif // ACTION_H
