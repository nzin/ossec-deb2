#
# Regular cron jobs for the ossec-hids package
#
0 4	* * *	root	[ -x /usr/bin/ossec-hids_maintenance ] && /usr/bin/ossec-hids_maintenance
