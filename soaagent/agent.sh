#!/bin/bash
# Written at 2017.05.05 by song 


this_dir=`pwd`
dirname $0|grep "^/" >/dev/null
if [ $? -eq 0 ];then
    this_dir=`dirname $0`
else
    dirname $0|grep "^\." >/dev/null
    retval=$?
    if [ $retval -eq 0 ];then
        this_dir=`dirname $0|sed "s#^.#$this_dir#"`
    else
        this_dir=`dirname $0|sed "s#^#$this_dir/#"`
    fi
fi

# echo $this_dir
executefile=${this_dir}/agent
configfile=${this_dir}/conf/core.conf
pidfile=${this_dir}/logs/agent.pid

stop_timeout=30

start() {
    # find ${this_dir} -name '.svn' | xargs rm -rf
    # export CONFIGD_LOG_FILE=/usr/local/whistle/configd/logs/configd.log
    # export LD_LIBRARY_PATH=/usr/local/whistle/configd/libs:$LD_LIBRARY_PATH
    #[ -x $exec ] || exit 5
    # [ ! -d ${runpath} ] && echo "Create Run Dir: ${runpath}"; (mkdir -p ${runpath}; chown -R ${user}:${user} ${runpath} || exit 6)
    ulimit -c unlimited
    ulimit -n 8192
    export AGENT_HOME=${this_dir}
    export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
    echo -n $"Starting agent server: "
    ${executefile} -c ${configfile} -d
    sleep 1
    processexist=0
    if [ -e $pidfile ]; then
        pidfileexist=1
        processpid=`cat $pidfile`
        processcount=`ps -p ${processpid} | wc -l`
        if [ ${processcount} -lt 2 ]; then
            processexist=0
        else
            processexist=1
        fi
    fi
    if [ $processexist -eq 1 ]; then
        echo "successfully"
    else
        echo "failed"
    fi
}

stop() {
    echo -n $"Stopping agent server: "
    processpid=0
    processexist=0
    if [ -e $pidfile ]; then
        pidfileexist=1
        processpid=`cat $pidfile`
        processcount=`ps -p ${processpid} | wc -l`
        if [ ${processcount} -lt 2 ]; then
            processexist=0
        else
            processexist=1
        fi
    fi
    if [ $processexist -eq 1 ]; then
        kill -TERM ${processpid}
        sleep 1
    fi
    processexist=0
    if [ -e $pidfile ]; then
        pidfileexist=1
        processpid=`cat $pidfile`
        processcount=`ps -p ${processpid} | wc -l`
        if [ ${processcount} -lt 2 ]; then
            processexist=0
        else
            processexist=1
        fi
    fi
    if [ $processexist -eq 1 ]; then
        echo "failed"
    else
        echo "successfully"
    fi
}

restart() {
    stop
    start
}

status() {
    echo -n $"Soa agent: "
    processpid=0
    processexist=0
    if [ -e $pidfile ]; then
        pidfileexist=1
        processpid=`cat $pidfile`
        processcount=`ps -p ${processpid} | wc -l`
        if [ ${processcount} -lt 2 ]; then
            processexist=0
        else
            processexist=1
        fi
    fi
    if [ $processexist -eq 1 ]; then
		echo "running"
    else
        echo "down"
    fi
    #cat $pidfile | xargs pstree -a -p
}

prepare() {
    find ${this_dir} -name '.svn' | xargs rm -rf
}

clear(){
	rm -rf ${this_dir}/logs/*
}

monitor(){
    # */1 * * * * /abc/agent.sh monitor
    for((i=0;i<6;i++))
    do
        echo -n $"Soa agent: "
        processpid=0
        processexist=0
        if [ -e $pidfile ]; then
            pidfileexist=1
            processpid=`cat $pidfile`
            processcount=`ps -p ${processpid} | wc -l`
            if [ ${processcount} -lt 2 ]; then
                processexist=0
            else
                processexist=1
            fi
        fi
        if [ $processexist -eq 1 ]; then
            echo "running"
        else
            echo "down"
            start
        fi
        sleep 10
    done
}

# See how we were called.
case "$1" in
  start)
    start
    ;;
  stop)
    stop
    ;;
  restart)
    restart
    ;;
  status)
    status
    ;;
  prepare)
    prepare
    ;;
  clear)
    clear
    ;;
  monitor)
    monitor
    ;;
  *)
    echo $"Usage: $0 {start|stop|restart|status|prepare|clear|monitor}"
    exit 1
esac

exit $?
