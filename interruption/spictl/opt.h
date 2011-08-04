#ifndef __OPT_H
#define __OPT_H

typedef int (*opt_func)(char *arg,void *target,int opt);

struct option2 {
  int has_arg;
  opt_func f;
  void *target;
  const char *name;
  const char *help;
};

extern int opt_cli_defaultport;
int opt_cli(char *arg,int *target,int opt);
int opt_str(char *arg,char **target,int opt);
int opt_bool(char *arg,int *target,int opt);
int opt_long(char *arg,unsigned *target,int opt);

void process_options(int argc,char **argv,struct option2 *opts);
#endif
