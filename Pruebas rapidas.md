1  make bonus
./Matt dos veces para ver que funciona flock

2 ./Matt_Daemon 
    sudo tail -f /var/log/matt_daemon/matt_daemon.log

3 nc 127.0.0.0 4242
    4 veces y vemos que el cuarto falla

4   # SIGTERM (15) - Terminación normal
        sudo pkill -SIGTERM Matt_daemon
        sudo pkill -15 Matt_daemon
        sudo pkill Matt_daemon           # (SIGTERM es default)


    # SIGINT (2) - Interrupt (Ctrl+C)
    sudo pkill -SIGINT Matt_daemon
    sudo pkill -2 Matt_daemon

    # SIGQUIT (3) - Quit (Ctrl+\)
    sudo pkill -SIGQUIT Matt_daemon
    sudo pkill -3 Matt_daemon

    # SIGHUP (1) - Hangup
    sudo pkill -SIGHUP Matt_daemon
    sudo pkill -1 Matt_daemon


    # SIGKILL (9) - Kill inmediato (NO se captura)
    sudo pkill -SIGKILL Matt_daemon
    sudo pkill -9 Matt_daemon
    sudo pkill -KILL Matt_daemon

    # SIGSTOP (19) - Stop inmediato (NO se captura)
    sudo pkill -SIGSTOP Matt_daemon
    sudo pkill -19 Matt_daemon
    sudo pkill -STOP Matt_daemon


Terminal 2: Ver estado
ls -l /var/lock/matt_daemon.lock
rw-r--r-- 1 root root 0 ...  ← ¡Archivo SIGUE existiendo!

sudo lslocks | grep matt_daemon
(vacío)  ← ¡Pero lock DESAPARECIÓ!

significa que el kernel libera el archivo lock automaticamente cuando el proceso muere


probasr el rotate en logs
while true; do echo "Message XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX $(date +%s%N)"; sleep 0.01; done | nc localhost 4242

