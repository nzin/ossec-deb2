#!/bin/sh
# preinst script for ossec-hids
#
# see: dh_installdeb(1)

set -e

# summary of how this script can be called:
#        * <new-preinst> `install'
#        * <new-preinst> `install' <old-version>
#        * <new-preinst> `upgrade' <old-version>
#        * <old-preinst> `abort-upgrade' <new-version>
# for details, see http://www.debian.org/doc/debian-policy/ or
# the debian-policy package


case "$1" in
    install|upgrade)

 	. ./src/init/shared.sh
	. ./src/init/functions.sh

    # This is a task for .. debconf .. 
        INSTYPE=${USER_INSTALL_TYPE}
	## USER_DIR have to return 0 after checking this pattern.
#                echo $USER_DIR |grep -E "^/[a-zA-Z0-9./_-]{3,128}$">/dev/null 2>&1
        INSTALLDIR=${USER_DIR}

    # Reading pre-defined file
	if [ -f ${PREDEF_FILE} ]; then
            . ${PREDEF_FILE}
	fi

	. ./src/init/shared.sh
	. ./src/init/language.sh
	. ./src/init/functions.sh
	. ./src/init/init.sh
	. ${TEMPLATE}/${LANGUAGE}/messages.txt

    ### Checks if gcc || cc is installed, wich is part of the base system, so not needed.
    # Checking dependencies
#	checkDependencies

	. ./src/init/update.sh
        # Is this an update?
	if [ "$1" = update ]; then

            update_only="yes"

	    . ./src/init/update.sh
	    
	    
	    ## src/init/update.sh::doUpdatecleanup:
## - executes $OSSEC_INIT (= /etc/ossec-init.conf) wich contains the values of DIRECTORY, VERSION, DATE, TYPE.
## - checks the DIRECTORY variable against a pattern. <-- do we need this? IMHO not needed

	    # if [ "`doUpdatecleanup`" = "${FALSE}" ]; then
            #     # Disabling update
            #     echo "${unabletoupdate}"
            #     sleep 5;
            #     update_only=""
	    # else
                # Get update
                USER_INSTALL_TYPE=`getPreinstalled`
                USER_DIR=`getPreinstalledDir`
                USER_DELETE_DIR="$nomatch"
	    # fi

            # We dont need to update the rules on agent installs
	    if [ ! "X${USER_INSTALL_TYPE}" = "Xagent" ]; then

                ## Should this be done by default os asking the user?
                $ECHO " - ${updaterules} ($yes/$no): "  
                if [ "X${USER_UPDATE_RULES}" = "X" ]; then
		    read ANY
                else
		    ANY=$yes
                fi

                case $ANY in
		    $yes)
                    update_rules="yes"
                    break;
                    ;;
		    $no)
                    break;
                    ;;
                esac
	    fi
	fi

	serverm=`echo ${server} | cut -b 1`
	localm=`echo ${local} | cut -b 1`
	agentm=`echo ${agent} | cut -b 1`
	helpm=`echo ${help} | cut -b 1`

    # Setting up the environment
#	setEnv
### setEnv code

    CEXTRA="$CEXTRA -DDEFAULTDIR=\\\"${INSTALLDIR}\\\""

    if [ "X$INSTYPE" = "Xagent" ]; then
        CEXTRA="$CEXTRA -DCLIENT"
    elif [ "X$INSTYPE" = "Xlocal" ]; then
        CEXTRA="$CEXTRA -DLOCAL"
    fi

    ls $INSTALLDIR >/dev/null 2>&1
    if [ $? = 0 ]; then
        if [ "X${USER_DELETE_DIR}" = "X" ]; then
            echo ""
            $ECHO "    - ${deletedir} ($yes/$no) [$yes]: "
            read ANSWER
        else
            ANSWER=${USER_DELETE_DIR}
        fi

        case $ANSWER in
            $yesmatch)
                rm -rf $INSTALLDIR
                if [ ! $? = 0 ]; then
                    exit 2;
                fi
                ;;
        esac
    fi


### End of setEnv code

    # Configuring the system (based on the installation type)
	if [ "X${update_only}" = "X" ]; then
            if [ "X$INSTYPE" = "Xserver" ]; then
		ConfigureServer
            elif [ "X$INSTYPE" = "Xagent" ]; then
		ConfigureClient
            elif [ "X$INSTYPE" = "Xlocal" ]; then
		ConfigureServer
            else
		catError "0x4-installtype"
            fi
	fi

    # Installing (calls the respective script
    # -- InstallAgent.sh or InstallServer.sh

##	Install    ## Next is the Install() function code

    echo "DIR=\"${INSTALLDIR}\"" > ${LOCATION}
    echo "CC=${CC}" >> ${LOCATION}
    echo "GCC=${CC}" >> ${LOCATION}
    echo "CLANG=clang" >> ${LOCATION}

    ## next code going into debian/rules :: configure target

    # Changing Config.OS with the new C flags
    # Checking if debug is enabled
    if [ "X${SET_DEBUG}" = "Xdebug" ]; then
        CEXTRA="${CEXTRA} -DDEBUGAD"
    fi

    echo "CEXTRA=${CEXTRA}" >> ./src/Config.OS

    ## End of code going into debian/rules :: configure target

    cd ./src
    # Binary install will use the previous generated code.
    if [ "X${USER_BINARYINSTALL}" = "X" ]; then
        make all
        if [ $? != 0 ]; then
            cd ../
            catError "0x5-build"
        fi

        # Building everything
        make build
        if [ $? != 0 ]; then
            cd ../
            catError "0x5-build"
        fi
    fi

    # If update, stop ossec
    if [ "X${update_only}" = "Xyes" ]; then
        UpdateStopOSSEC
    fi

    # Making the right installation type
	if [ "X$INSTYPE" = "Xserver" ]; then
        ./InstallServer.sh

    elif [ "X$INSTYPE" = "Xagent" ]; then
        ./InstallAgent.sh

    elif [ "X$INSTYPE" = "Xlocal" ]; then
        ./InstallServer.sh local
	fi

    cd ../

    
    ### next code going to postinst file

    # Generate the /etc/ossec-init.conf
    VERSION_FILE="./src/VERSION"
    VERSION=`cat ${VERSION_FILE}`
    chmod 700 ${OSSEC_INIT} > /dev/null 2>&1
    echo "DIRECTORY=\"${INSTALLDIR}\"" > ${OSSEC_INIT}
    echo "VERSION=\"${VERSION}\"" >> ${OSSEC_INIT}
    echo "DATE=\"`date`\"" >> ${OSSEC_INIT}
    echo "TYPE=\"${INSTYPE}\"" >> ${OSSEC_INIT}
    chmod 600 ${OSSEC_INIT}
    cp -pr ${OSSEC_INIT} ${INSTALLDIR}${OSSEC_INIT}
    chmod 644 ${INSTALLDIR}${OSSEC_INIT}


    # If update_rules is set, we need to tweak
    # ossec.conf to read the new signatures.
    if [ "X${update_rules}" = "Xyes" ]; then
        UpdateOSSECRules
    fi

    # If update, start OSSEC
    if [ "X${update_only}" = "Xyes" ]; then
        UpdateStartOSSEC
    fi

    # Calling the init script  to start ossec hids during boot
    if [ "X${update_only}" = "X" ]; then
        runInit
        if [ $? = 1 ]; then
            notmodified="yes"
        fi
    fi
  fi

    ### end of the code going to postinst file


## End of Install() code

    # User messages
	echo ""
	echo " - ${configurationdone}."
	echo ""
	echo " - ${tostart}:"
	echo "		$INSTALLDIR/bin/ossec-control start"
	echo ""
	echo " - ${tostop}:"
	echo "		$INSTALLDIR/bin/ossec-control stop"
	echo ""
	echo " - ${configat} $INSTALLDIR/etc/ossec.conf"
	echo ""


	if [ "X${update_only}" = "Xyes" ]; then
        # Message for the update
            if [ "X`sh ./src/init/fw-check.sh`" = "XPF" -a "X${ACTIVERESPONSE}" = "Xyes" ]; then
		AddPFTable
            fi
            echo " - ${updatecompleted}"
            exit 0;
	fi


    # PF firewall message
	if [ "X`sh ./src/init/fw-check.sh`" = "XPF" -a "X${ACTIVERESPONSE}" = "Xyes" ]; then
            AddPFTable
	fi


	if [ "X$INSTYPE" = "Xserver" ]; then

            echo " - ${addserveragent}"
            echo "   ${runma}:"
            echo "   $INSTALLDIR/bin/manage_agents"
            echo "   http://www.ossec.net/en/manual.html#ma"


	elif [ "X$INSTYPE" = "Xagent" ]; then
            catMsg "0x104-client"
            echo "   $INSTALLDIR/bin/manage_agents"
            echo "   http://www.ossec.net/en/manual.html#ma"
	fi

	if [ "X$notmodified" = "Xyes" ]; then
            catMsg "0x105-noboot"
            echo "		$INSTALLDIR/bin/ossec-control start"
	fi


    ;; # end of install|upgrade

    abort-upgrade)
    ;;

    *)
        echo "preinst called with unknown argument \`$1'" >&2
        exit 1
    ;;
esac

# dh_installdeb will replace this with shell code automatically
# generated by other debhelper scripts.

#DEBHELPER#

exit 0
