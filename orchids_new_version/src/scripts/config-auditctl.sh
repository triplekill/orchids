#!/bin/sh
##############################################################################################
# auditctl
##############################################################################################

echo >>$CONFIG_FILE
echo "# We make sure auditctl is configured correctly." >>$CONFIG_FILE
    
RULE_FILES=`grep AddRuleFile $DISTDIR/orchids-rules.conf.dist | sed -e "s|.*/\([^/]*\).pprule|\\1.rule|g"`
TMP_FILE=/tmp/orchids-config-$$
echo >$TMP_FILE
for rule in $RULE_FILES
do
    # enumerate those rule files that contain the magic string "*** [local:auditctl]" somewhere
    if grep -q "\*\*\* *\[local:auditctl\]" $DISTDIR/rules/$rule
    then
	tr -cs "[:alpha:]_" "\n" <$DISTDIR/rules/$rule | grep SYS_ | sed -e "s/SYS_//" >>$TMP_FILE
    fi
done
echo "* instruct auditctl to report the following syscalls:"
AUDITCTL_SYSCALLS=`sort $TMP_FILE | uniq`
echo "  "$AUDITCTL_SYSCALLS
rm $TMP_FILE

AUDITCTL_SYSCALL_OPTIONS=`for syscall in $AUDITCTL_SYSCALLS; do echo -n " -S $syscall"; done`
cat <<EOT >>$CONFIG_FILE
if hash auditctl 2>/dev/null
then
    ARCH=\`arch\`
    # Let us remove all the rules we might have added in earlier:
    auditctl -D -k orchids
    # Now add all the syscalls we need to monitor.
    # These are obtained by inspection of the rules that mention *** [local:auditctl]
    \$ORCHIDS_RUNTIME_USER=\`id -u @@RUNUSER@@\`
    auditctl -a always,exit -F arch=\$ARCH -F uid!=\$ORCHIDS_RUNTIME_USER $AUDITCTL_SYSCALL_OPTIONS -k orchids
fi
EOT
# For example:
# auditctl -a always,exit -F arch=x86_64 -F uid!=65534 -S clone -S fork -S vfork -S execve -S setreuid -S setresuid -S setuid -S setregid -S setresgid -S setgid -S exit -S kill
