%
% Some sample queries:
%
% What are defined data types:
% setof(Type, Obj^obj(Obj,Type), TypeList).
%
% What are defined objects:
% setof([Obj,Type], obj(Obj,Type), ObjList).
%
% What are email(s) of user id 42 ?
% prop(User, user, uid, 42), prop(User, user, email, X).
%
% To which user id 'Uid' is the mac address '01:02:03:04:05:06' attached to ?
% prop(User, user, uid, Uid), \
% prop(User, user, workstation, Workstation), \
% prop(Workstation, host, interface, Interface), \
% prop(Interface, interface, hwaddr, '01:02:03:04:05:06').
%

obj(User, user) :-
        user(User).
obj(Host, host) :-
        host(Host).
obj(Interface, interface) :-
        interface(Interface).
obj(Room, room) :-
        room(Room).
obj(Netplug, netplug) :-
        netplug(Netplug).
obj(Link, link) :-
        link(Link).

prop(User, user, uid, Uid) :-
        uid(User, Uid), user(User).
prop(User, user, email, Email) :-
        email(User, Email), user(User).
prop(User, user, workstation, Workstation) :-
        workstation(User, Workstation), user(User), host(Workstation).
prop(User, user, realname, RealName) :-
        realname(User, RealName), user(User).
prop(User, user, location, Location) :-
        location(User, Location), user(User), room(Location).
prop(Host, host, interface, Interface) :-
        interface(Host, Interface), host(Host), interface(Interface).
prop(Host, host, software, Software) :-
        installed(Host, Software), host(Host).
prop(Interface, interface, hwaddr, Hwaddr) :-
        hwaddr(Interface, Hwaddr), interface(Interface).
prop(Interface, interface, ipv4addr, Ipv4addr) :-
        ipv4addr(Interface, Ipv4addr), interface(Interface).
prop(Netplug, netplug, location, Location) :-
        location(Netplug, Location), netplug(Netplug), room(Location).

user(jsmith).
uid(jsmith, 42).
email(jsmith, 'john.smith@domain.net').
email(jsmith, 'jsmith@domain.net').
email(jsmith, 'smith@domain.net').
workstation(jsmith, jscomputer).
realname(jsmith, 'John SMITH').
location(jsmith, jsroom).

host(jscomputer).
host(server).

installed(jscomputer, 'Windows XP').
installed(server, 'FreeBSD 6.0').

interface(jscomputer-eth0).
interface(jscomputer-eth1).
interface(server-eth0).
interface(switch0-port1).
interface(switch0-port2).
interface(switch0-port3).
interface(switch0-port4).

hwaddr(jscomputer-eth0, '01:02:03:04:05:06').
hwaddr(jscomputer-eth1, '07:08:09:0a:0b:0c').
hwaddr(server-eth0, '0a:0a:0a:0a:0a:0a').

ipv4addr(jscomputer-eth0, '10.0.0.42').
ipv4addr(jscomputer-eth1, '192.168.0.42').
ipv4addr(server-eth0, '10.0.0.1').

interface(jscomputer, jscomputer-eth0).
interface(jscomputer, jscomputer-eth1).
interface(server, server-eth0).
interface(switch0, switch0-port1).
interface(switch0, switch0-port2).
interface(switch0, switch0-port3).
interface(switch0, switch0-port4).

room(server-room).
room(jsroom).

netplug(jsroom-plug-0).

link(anon-link-0).
link(anon-link-1).

linked(anon-link-0, jscomputer-eth0).
linked(anon-link-0, jsroom-plug-0).
