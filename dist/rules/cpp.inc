
#ifndef CPP_INC
#define CPP_INC 

#define MSG "Blah Blah Blah"
#define TITI 1

rule cpp_inc {
  state init { if (.syslog.prog == "pouet") goto next; }
  state next { print("end of rule cpp_inc\n"); }
}

#endif
