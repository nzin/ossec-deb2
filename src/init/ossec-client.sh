#!/bin/sh
# ossec-control        This shell script takes care of starting
#                      or stopping ossec-hids
# Author: Daniel B. Cid <daniel.cid@gmail.com>


[ -e /etc/ossec-init.conf ] && . /etc/ossec-init.conf # Source the configuration file for DIRECTORY
if [ -z "$DIRECTORY" ]; then
	echo "ERROR: Cannot determine the value of the OSSEC directory" 
	[ ! -e "/etc/ossec-init.conf" ] && echo "ERROR: /etc/ossec-init.conf does not exist"
	exit 1
fi
LOCAL=`dirname $0`;
cd ${LOCAL}

###  Do not modify bellow here ###
NAME="OSSEC HIDS"
VERSION="v2.7.1"
AUTHOR="Trend Micro Inc."
DAEMONS="ossec-logcollector ossec-syscheckd ossec-agentd ossec-execd"


## Locking for the start/stop
LOCK="${DIRECTORY}/var/run/ossec-hids/"
LOCK_PID="${LOCK}/start-script-lock.pid"


# This number should be more than enough (even if it is
# started multiple times together). It will try for up
# to 10 attempts (or 10 seconds) to execute.
MAX_ITERATION="10"



# Check pid
checkpid()
{
    for i in ${DAEMONS}; do
        for j in `cat ${LOCK}/${i}*.pid 2>/dev/null`; do
            ps -p $j |grep ossec >/dev/null 2>&1
            if [ ! $? = 0 ]; then
                echo "Deleting PID file '${LOCK}/${i}-${j}.pid' not used..."
                rm ${LOCK}/${i}-${j}.pid
            fi    
        done    
    done    
}



# Lock function
lock()
{
    i=0;
    
    # Providing a lock.
    while [ 1 ]; do
        [ ! -e "${LOCK}" ] && mkdir -p ${LOCK} > /dev/null 2>&1
        # Ensure we can make the LOCK properly first
	if [ ! -d "${LOCK}" ] ; then
		echo "ERROR: The configured lock directory ${LOCK} is not a directory or it does not exist, cannot continue" 
		exit 1
	fi
	# If there is no PIDfile then we can set the pid and break
	if [ ! -e "${LOCK_PID}" ] ; then
            # Lock aquired (setting the pid)
            echo "$$" > ${LOCK_PID}
            return;
        fi

        # Waiting 1 second before trying again
        sleep 1;
        i=`expr $i + 1`;

        # If PID is not present, speed things a bit.
        kill -0 `cat ${LOCK_PID}` >/dev/null 2>&1
        if [ ! $? = 0 ]; then
            # Pid is not present.
            i=`expr $i + 1`;
        fi    

        # We tried 10 times to acquire the lock.
        if [ "$i" = "${MAX_ITERATION}" ]; then
            # Unlocking and executing
            unlock;
            echo "$$" > ${LOCK_PID}
            return;
        fi
    done
}

# Unlock function
# Just remove the lock file if it is there, keep the lock directory
# for later. We don't remove the directory (rm -rf is dangerous in a shell script)
unlock()
{
	[ ! -e "${LOCK}" ]  && return 0
	[ ! -d "${LOCK}" ]  && return 0
	[ -e "${LOCK_PID}" ] &&  rm -f ${LOCK_PID}
}


# Help message
help()
{
    # Help message
    echo "Usage: $0 {start|stop|restart|status}";
    exit 1;
}


# Status function
status()
{
    for i in ${DAEMONS}; do
        pstatus ${i};
        if [ $? = 0 ]; then
            echo "${i} not running..."
        else
            echo "${i} is running..."
        fi
    done             
}

testconfig()
{
    # We first loop to check the config. 
    for i in ${SDAEMONS}; do
        ${DIRECTORY}/bin/${i} -t;
        if [ $? != 0 ]; then
            echo "${i}: Configuration error. Exiting"
            unlock;
            exit 1;
        fi    
    done
}

# Start function
start()
{
    SDAEMONS="ossec-execd ossec-agentd ossec-logcollector ossec-syscheckd"
    
    echo "Starting $NAME $VERSION (by $AUTHOR)..."
    lock;
    checkpid;

    
    # We actually start them now.
    for i in ${SDAEMONS}; do
        pstatus ${i};
        if [ $? = 0 ]; then
            ${DIRECTORY}/bin/${i};
            if [ $? != 0 ]; then
		echo "${i} did not start";
                unlock;
                exit 1;
            fi 

            echo "Started ${i}..."            
        else
            echo "${i} already running..."                
        fi    
    
    done    

    # After we start we give 2 seconds for the daemons
    # to internally create their PID files.
    sleep 2;
    unlock;
    echo "Completed."
}

# Process status
pstatus()
{
    pfile=$1;
    
    # pfile must be set
    if [ "X${pfile}" = "X" ]; then
        return 0;
    fi
        
    ls ${LOCK}/${pfile}*.pid > /dev/null 2>&1
    if [ $? = 0 ]; then
        for j in `cat ${LOCK}/${pfile}*.pid 2>/dev/null`; do
            ps -p $j |grep ossec >/dev/null 2>&1
            if [ ! $? = 0 ]; then
                echo "${pfile}: Process $j not used by ossec, removing .."
                rm -f ${LOCK}/${pfile}-$j.pid
                continue;
            fi
                
            kill -0 $j > /dev/null 2>&1
            if [ $? = 0 ]; then
                return 1;
            fi    
        done    
    fi
    
    return 0;    
}


# Stop all
stopa()
{
    lock;
    checkpid;
    for i in ${DAEMONS}; do
        pstatus ${i};
        if [ $? = 1 ]; then
            echo "Killing ${i} .. ";
            
            kill `cat ${LOCK}/${i}*.pid`;
        else
            echo "${i} not running .."; 
        fi
        
        rm -f ${LOCK}/${i}*.pid
        
     done    
    
    unlock;
    echo "$NAME $VERSION Stopped"
}


### MAIN HERE ###

case "$1" in
  start)
    testconfig
	start
	;;
  stop) 
	stopa
	;;
  restart)
    testconfig
	stopa
        sleep 1;
	start
	;;
  reload)
	DAEMONS="ossec-logcollector ossec-syscheckd ossec-agentd"
	stopa
	start
	;;
  status)
    status
	;;
  help)  
    help
    ;;
  *)
    help
esac

