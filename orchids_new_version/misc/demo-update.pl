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

connec :-
    odbc_connect('orchids', _, [alias(orchids)]).

connec(D) :-
    odbc_connect(D, _, [alias(orchids)]).

getusers :-
    odbc_query(orchids, 'SELECT uid, login, real_name, host from user INNER JOIN host ON user.workstation=host.host_id', row(Uid, Login, RName, Ws)), assert(user(Login)), assert(uid(Login, Uid)), assert(realname(Login, RName)), assert(workstation(Login, Ws)), false.
getemails :-
    odbc_query(orchids, 'SELECT login, email FROM user_email INNER JOIN user ON user.user_id=user_email.user_id', row(Login, Email)), assert(email(Login, Email)), false.
gethosts :-
    odbc_query(orchids, 'SELECT host, installed FROM host', row(Host, Installed)), assert(host(Host)), assert(installed(Host, Installed)), false.
getinterfaces :-
    odbc_query(orchids, 'SELECT host, interface, hwaddr, ipv4addr FROM interface INNER JOIN host ON host.host_id=interface.host_id',
	       row(Host, If, Hw, Ipv4)), assert(interface(If)), assert(interface(Host, If)), assert(hwaddr(If, Hw)), assert(ipv4addr(If, Ipv4)), false.

getall :- connec, \+ getusers, \+ getemails, \+ gethosts , \+ getinterfaces, odbc_disconnect(orchids), true.

getall(D) :- connec(D), \+ getusers, \+ getemails, \+ gethosts , \+ getinterfaces, odbc_disconnect(orchids), true.


flushall :-
    abolish(user/1),
    abolish(uid/2),
    abolish(realname/2),
    abolish(workstation/2),
    abolish(email/2),
    abolish(host/1),
    abolish(installed/2),
    abolish(interface/1),
    abolish(interface/2),
    abolish(hwaddr/2),
    abolish(ipv4addr/2).

