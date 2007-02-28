include(`orchids-rulegen.m4')dnl
dnl
pattern(`ptrace', ` .snare.syscall == "ptrace" ')
pattern(`modprobe', ` .snare.syscall == "modprobe" ')
pattern(`getregs', ` .snare.syscall == "getregs" ')
pattern(`poketext', ` .snare.syscall == "poketext" ')
dnl
actions(`do_hw', `print("Hello World !");')
dnl
dnl rule(`test',
dnl event(ptrace, do_hw, do_hw),
dnl do_hw,do_hw)

rule( `demo', 
before(event(ptrace), repeat( oneamong(event(poketext), after(event(getregs), event(modprobe))), 3) )
 )
