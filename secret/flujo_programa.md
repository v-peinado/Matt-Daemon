main()
  │
  ├─► TintinReporter()
  │     └─► createLogDirectory()     [mkdir()]
  │     └─► open log file            [std::ofstream]
  │
  ├─► MattDaemon::init()
  │     ├─► checkRoot()              [getuid()]
  │     ├─► createLockFile()         [open(), flock(LOCK_EX)]
  │     └─► Server::init()
  │           ├─► createSocket()     [socket(), setsockopt()]
  │           ├─► bindSocket()       [bind()]
  │           ├─► listenSocket()     [listen()]
  │           └─► setupSignals()     [sigprocmask(), signalfd()]
  │
  └─► MattDaemon::run()
        │
        ├─► Daemonize::daemonize()
        │     ├─► fork() + _exit()   [1er fork - desvincula terminal]
        │     ├─► setsid()           [nueva sesión, nuevo PGID]
        │     ├─► fork() + _exit()   [2do fork - no puede readquirir TTY]
        │     ├─► chdir("/")         [evita bloquear mount points]
        │     ├─► umask(0)           [permisos sin restricción]
        │     └─► redirectFd()       [dup2() stdin/out/err → /dev/null]
        │
        └─► Server::run()            [LOOP PRINCIPAL]
              │
              ├─► setupFdSet()       [FD_ZERO, FD_SET]
              ├─► select()           [espera actividad en FDs]
              │
              ├─► [señal recibida]
              │     └─► handleSignal()   [read(signalfd), stop()]
              │
              ├─► [nuevo cliente]
              │     └─► acceptNewClient()  [accept(), push to vector]
              │
              └─► [datos de cliente]
                    ├─► handleClientData()   [recv()]
                    └─► processMessage()
                          ├─► "quit" → stop()
                          └─► otro  → log mensaje

~MattDaemon()
  └─► removeLockFile()               [flock(LOCK_UN), close(), unlink()]