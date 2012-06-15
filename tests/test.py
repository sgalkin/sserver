#!/usr/bin/env python

import socket
import threading
import subprocess
import tempfile
import time
import sys
import os
import stat
import signal

config = {"daemon": False, 
          "tcp_if": "192.168.0.105",
          "tcp_port": 10080 if os.getuid() != 0 else 600,
          "udp_if": "127.0.0.1",
          "udp_port": 10081 if os.getuid() != 0 else 601,
          "datafile": "data.txt",
          "sleep": 1000,
          "maint": False,
          "loglevel": "ERROR"}

CHECK = [ ('GET username=id%d\r\n', '404 Not Found\r\n'),
          ('REGISTER username=id%d;email=email@example.com\r\n', '200 OK\r\n'),
          ('GET username=id%d\r\n', '200 OK email=email@example.com\r\n'),
          ('REGISTER username=id%d;email=em@example.com\r\n', '409 Conflict\r\n'),
          ('GET username=,id%d\r\n', '406 Not Acceptable\r\n'),
          ('REGISTER username=id%d;email=email\r\n', '406 Not Acceptable\r\n'),
          ('PING %d\r\n', '400 Bad request\r\n') ]

  

def server(name):
    return subprocess.Popen(['./sserver', '-c', name])

def tcp_client(ident, data):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((config['tcp_if'], config['tcp_port']))
        for x in data:
            sock.send(x[0] % ident)
            res = sock.recv(1024)
            if(res != x[1]): print >> sys.stderr, "ERROR: ", ident, x, res
        sock.close()
    except socket.error, msg:
        print >> sys.stderr, msg
        sys.exit(1)

def udp_client(ident, data):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        for x in data:
            sock.connect((config['udp_if'], config['udp_port']))
            sock.send(x[0] % ident) #, (config['udp_if'], config['udp_port']))
            res = sock.recv(1024)
            if(res != x[1]): print >> sys.stderr, "ERROR: ", ident, x, res

    except socket.error, msg:
        print >> sys.stderr, msg
        sys.exit(1)

def test_protocol(data):
    print "TEST PROTOCOL"
    open(data, mode="w").truncate(0)
    check = CHECK
    threads = ([ threading.Thread(target=tcp_client, args=(i,check,)) for i in xrange(10) ] +
               [ threading.Thread(target=udp_client, args=(10+i,check,)) for i in xrange(1) ])
        
    st = time.time()
    [ x.start() for x in threads ]
    [ x.join() for x in threads ]
    if(time.time() - st > 10): print >> sys.stderr, "ERROR: ", time.time() - st, "seconds"

def test_sleep(data):
    print "TEST SLEEP"
    open(data, mode="w").truncate(0)
    st = time.time()
    udp_client(0, [ CHECK[0] ])
    if(time.time() - st < 0.8 * config['sleep'] / 1000):
        print >> sys.stderr, "ERROR: NO SLEEP", time.time() - st, "seconds"

def test_overloaded(data):  
    print "TEST OVERLOAD"
    register = [('REGISTER username=id%d%s;email=email@example.com\r\n' % (i, "%d"), '200 OK\r\n')
                for i in xrange(101) ]
    open(data, mode="w").truncate(0)
    threads = [threading.Thread(target=tcp_client, 
                                args=(i,register[i*len(register)/10:(i+1)*len(register)/10],)) 
               for i in xrange(10) ]
    [ x.start() for x in threads ]
    [ x.join() for x in threads ]
    tcp_client(0, [ (CHECK[1][0], "405 Overloaded\r\n") ])

def test_unavailable(data):
    print "TEST UNAVAILABLE"
    open(data, mode="w").truncate(0)
    cnt = [ ('GET username=id%d\r\n', '404 Not Found\r\n'),
            ('REGISTER username=id%d;email=email@example.com\r\n', '200 OK\r\n'),
            ('GET username=id%d\r\n', '200 OK email=email@example.com\r\n') ]
    tcp_client(0, cnt)
    os.chmod(data, 0444)
    cnt[1] = ('REGISTER username=id%d;email=email@example.com\r\n', '503 Service unavailable\r\n')
    cnt[2] = cnt[0]
    tcp_client(1, cnt)
    os.chmod(data, 0)
    cnt[0] = ('GET username=id%d\r\n', '503 Service unavailable\r\n')
    cnt[2] = ('GET username=id%d\r\n', '503 Service unavailable\r\n')
    tcp_client(0, cnt)
    os.chmod(data, 0666)
    cnt = [ ('GET username=id%d\r\n', '404 Not Found\r\n'),
            ('REGISTER username=id%d;email=email@example.com\r\n', '200 OK\r\n'),
            ('GET username=id%d\r\n', '200 OK email=email@example.com\r\n') ]
    tcp_client(2, cnt)
    os.unlink(data)
    cnt = [ ('GET username=id%d\r\n', '503 Service unavailable\r\n'),
            ('REGISTER username=id%d;email=email@example.com\r\n', '503 Service unavailable\r\n'),
            ('GET username=id%d\r\n', '503 Service unavailable\r\n') ]
    tcp_client(3, cnt)
    open(data, "w").close()
    os.chmod(data, 0666)

def test_reconfigure(data, conf, srv):
    print "TEST RECONFGIURE"
    open(data, mode="w").truncate(0)
    cnt = [ ('GET username=id%d\r\n', '404 Not Found\r\n'),
            ('REGISTER username=id%d;email=email@example.com\r\n', '200 OK\r\n'),
            ('GET username=id%d\r\n', '200 OK email=email@example.com\r\n') ]
    tcp_client(0, cnt)
    with tempfile.NamedTemporaryFile(mode="rw+") as ndata:
        os.chmod(ndata.name, 0666)
        config['datafile'] = ndata.name
        nconf = open(conf, "w")
        print >> nconf, "\n".join([ "%s=%s" % (str(k), str(v)) for (k, v) in config.iteritems()])
        nconf.flush()
        nconf.close()

        srv.send_signal(signal.SIGUSR1)
        time.sleep(1)
        tcp_client(0, cnt)

def kill_daemon():
    psout,_ = subprocess.Popen(['ps', '-A'], stdout=subprocess.PIPE).communicate()
    [ os.kill(int(x.split(" ")[0]), signal.SIGINT)
      for x in psout.split("\n") if "sserver" in x ]

def main():
    with tempfile.NamedTemporaryFile(mode="rw+") as data:
        with tempfile.NamedTemporaryFile(mode="rw+") as tmp:
            os.chmod(data.name, 0666)
            os.chmod(tmp.name, 0644)
            config['datafile'] = data.name
            print >> tmp, "\n".join([ "%s=%s" % (str(k), str(v)) for (k, v) in config.iteritems()])
            tmp.flush()

            if "--daemon" in sys.argv:
                config['daemon'] = True

            srv = server(tmp.name)
            time.sleep(1)

            test_protocol(data.name)
            test_sleep(data.name)
            test_overloaded(data.name)
            test_unavailable(data.name)
            test_reconfigure(data.name, tmp.name, srv)

            if "--daemon" in sys.argv:
                kill_daemon()
            else:
                srv.kill()

if __name__ == "__main__":
    main()
