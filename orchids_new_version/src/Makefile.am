SUBDIRS = modules
bin_PROGRAMS = orchids
orchids_SOURCES = \
        orchids.h orchids_defaults.h orchids_types.h      \
        main.c main_priv.h                                \
        evt_mgr.c evt_mgr.h evt_mgr_priv.h                \
        mod_mgr.c mod_mgr.h                               \
        orchids_api.c orchids_api.h                       \
        engine.c engine.h engine_priv.h                   \
        rule_compiler.c rule_compiler.h                   \
        orchids_cfg.c                                     \
        lang.c lang.h lang_priv.h                         \
        ovm.c ovm.h ovm_priv.h                            \
        graph_output.c graph_output.h graph_output_priv.h \
        issdl.yy.c issdl.tab.c issdl.tab.h                \
 \
        util/array.c               util/array.h           \
                                   util/queue.c           \
        util/crc32.c               util/queue.h           \
        util/debuglog.c            util/debuglog.h        \
        util/safelib.c             util/safelib.h         \
        util/offsetof.h                                   \
        util/slist.h               util/dlist.h           \
        util/stailq.h              util/dtailq.h          \
        util/hash.c                util/stack.c           \
        util/hash.h                util/stack.h           \
        util/linux_process_info.c  util/strhash.c         \
        util/linux_process_info.h  util/strhash.h         \
                        util/string_util.c     \
                        util/tree.c            \
        util/misc.c                util/tree.h            \
        util/objhash.c             util/objhash.h         \
        util/timer.h

orchids_LDADD = -ldl
orchids_LDFLAGS = -export-dynamic
AM_CFLAGS= -I$(srcdir)/util

EXTRA_DIST = issdl.l issdl.y

BUILT_SOURCES = \
	issdl.yy.c issdl.tab.c issdl.tab.h

$(srcdir)/issdl.tab.c $(srcdir)/issdl.tab.h: $(srcdir)/issdl.y
	$(YACC) -p issdl -d -b issdl -o $(srcdir)/issdl.tab.c $(srcdir)/issdl.y

$(srcdir)/issdl.yy.c: $(srcdir)/issdl.l issdl.tab.h
	$(LEX) -f -Pissdl -o$(srcdir)/issdl.yy.c $(srcdir)/issdl.l
