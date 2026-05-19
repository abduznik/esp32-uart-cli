#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>

typedef struct {
    const char *name;
    const char *description;
    void (*handler)(int argc, char **argv);
} Command;

extern const Command commands[];
extern const size_t num_commands;

const Command* find_command(const char *name);

#endif // COMMANDS_H
