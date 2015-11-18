# posterLib - remote posters

This posterLib version can read and write local and remote posters.

*   posterServ startup:

    if `POSTER_HOST` is *not* set when running the `h2 init -p`
    command, then a posterServ process is launched on the local host; if
    `POSTER_HOST` is set, then no posterServ process will be launched.

*   where are my posters ? 

    if `POSTER_HOST` is *not* set when a module is started, the
    posters are created locally in shared memory.

    if `POSTER_HOST` is set, then the posters are created through
    posterServ on the machine that `POSTER_HOST` refers to.

*   how do I find posters ? 

    the posterFind() function looks for posters sequentially in that order:
 
    1. locally in shared memory
    1. on the host designated by `POSTER_HOST` if defined
    1. on each host designated by the colon-separated `POSTER_PATH` list.


*   typical setups:

    * My posters are created locally, but I need to access to posters
      on the remote host 'host1':
 
        unsetenv POSTER_HOST
        setenv POSTER_PATH host1


    * I want to create my posters on the remote host 'host1', and also
      access posters on remote hosts 'host2' and 'host3:

        setenv POSTER_HOST host1
        setenv POSTER_PATH host2:host3

## Mac OS X / Darwin note

The remote poster daemon (posterServ) is a RPC server and needs the
portmap/rpcbind service to be active.

The RPC portmap daemon is not started automatically on most Mac OS X
configurations (it's started only if NFS file systems are exported or
if the machine is a netinfo server).

To force portmap startup on Mac OS X, set `RPCSERVER=-YES-` in
`/etc/hostconfig` and reboot.

/!\ also note that since 1010 pocolibs is no longer actively
maintained on Mac OS X

## KNOWN BUGS

*   posterShow only shows local posters. 

*   posterMemCreate is not yet implemented in this version .

*   the deletion of an existing poster is handled poorly. remote clients
    may still crash if trying to access a deleted poster. 

*   there is a potential problem/leak in remotePosterWrite() 
