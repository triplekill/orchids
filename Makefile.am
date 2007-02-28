SUBDIRS    = src dist

EXTRA_DIST = misc/orchids.spec misc/orchids-sysv

# install_exec_DIRS = \
#   $(sysconfdir)/orchids \
#   $(sysconfdir)/orchids/rules

# install_rule_FILES = \
#   pid_tracker.rule \
#   apachessl.rule \
#   portscan.rule \
#   dhcp_lease_check.rule \
#   password_sniff.rule \
#   anti_ssh_scan.rule

# install_data_DIRS = \
#   $(localstatedir)/orchids/htmlstate \
#   $(localstatedir)/orchids/htmlstate/rule-insts \
#   $(localstatedir)/orchids/htmlstate/rules \
#   $(localstatedir)/orchids/htmlstate/reports \
#   $(localstatedir)/orchids/reports

# install_htmlstate_FILES = \
#   icon_blank.png \
#   icon_doc.png \
#   icon_folder_closed.png \
#   icon_folder_open.png \
#   icon_lastnode.png \
#   icon_mlastnode.png \
#   icon_mnode.png \
#   icon_node.png \
#   icon_plastnode.png \
#   icon_pnode.png \
#   icon_vertline.png \
#   index-about.html \
#   index-copyright.html \
#   index-help.html \
#   index.html \
#   index-main.html \
#   index-menu.html \
#   index-title.html \
#   logo-cnrs.gif \
#   logo-ens.gif \
#   logo-inria.gif \
#   logo-lsv.gif \
#   orchids.gif \
#   simplex-bkg.gif \
#   style.css \
#   tree.js \
#   reports/simplex-bkg.gif \
#   reports/style.css \
#   rule-insts/simplex-bkg.gif \
#   rule-insts/style.css \
#   rules/simplex-bkg.gif \
#   rules/style.css

# install-exec-hook:
# 	for dir in $(install_exec_DIRS) ; do \
# 	  $(INSTALL) -d $$dir; \
# 	done ; \
#         for rule in $(install_rule_FILES) ; do \
#           $(INSTALL_DATA) $(srcdir)/dist/rules/$$rule \
#                           $(DESTDIR)$(sysconfdir)/orchids/rules ; \
#         done ; \
# 	sed -e 's#@@VARDIR@@#$(localstatedir)#g' \
#             -e 's#@@ETCDIR@@#$(sysconfdir)#g' \
#             -e 's#@@LIBDIR@@#$(libdir)#g' \
#             -e 's#@@RUNUSER@@#$(ORCHIDS_RUNTIME_USER)#g' \
#            $(srcdir)/dist/orchids.conf.dist \
#            > $(DESTDIR)$(sysconfdir)/orchids/orchids.conf

# install-data-hook:
# 	for dir in $(install_data_DIRS) ; do \
#           $(INSTALL) -o $(ORCHIDS_RUNTIME_USER) -d $$dir; \
#         done ; \
# 	for file in $(install_htmlstate_FILES) ; do \
#           $(INSTALL_DATA) -o $(ORCHIDS_RUNTIME_USER) \
#             $(srcdir)/dist/htmlstate/$$file \
#             $(DESTDIR)$(localstatedir)/orchids/htmlstate/$$file; \
#         done

# orchidsstatedir must be owned by orchids
install-data-hook:
	mkdir -p $(DESTDIR)$(localstatedir)/orchids/reports
	mkdir -p $(DESTDIR)$(localstatedir)/orchids/log
	mkdir -p $(DESTDIR)$(localstatedir)/run/orchids
	-chown -R $(ORCHIDS_RUNTIME_USER) $(DESTDIR)$(localstatedir)/orchids
