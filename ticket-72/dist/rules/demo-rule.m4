include(`orchids-rulegen.m4')dnl
dnl
pattern(`ptrace', ` .snare.syscall == "ptrace" ')
pattern(`modprobe', ` .snare.syscall == "modprobe" ')
pattern(`getregs', ` .snare.syscall == "getregs" ')
pattern(`poketext', ` .snare.syscall == "poketext" ')
dnl
actions(`do_report', `report();')
dnl
rule( `demo', 
  before(event(ptrace),
         repeat(oneamong(event(poketext),
                after(event(getregs),
                      event(modprobe))), 3) ),,
  do_report)
