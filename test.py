#!/usr/bin/python

import subprocess
import socket
import time
import os

p = subprocess.Popen(["./myserver", "1100"])
if p is None: exit()

for fn in ['foo', 'bar', 'caz']:
	with open(fn,'w') as f:
		f.write(fn[0]*30000)

time.sleep(1)

def recvall(s, n):
	d = ''
	while n > 0:
		c = s.recv(n)
		if len(c) == 0:
			break
		d += c
		n -= len(c)
	return d

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 1100))
s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s2.connect(('127.0.0.1', 1100))

def f1(s):
	s.sendall('foo\nbar\n')
	d = recvall(s, 5000)
	if d != 'f'*5000:
		print 'FAIL 1'
f1(s)
f1(s2)
		
def f2(s):
	s.sendall('caz\n')
	d = recvall(s, 85000)
	if d != 'f'*25000 + 'b'*30000 + 'c'*30000:
		print 'FAIL 2'

f2(s)
f2(s2)

print "ok"

s.close()
s2.close()
p.kill()

for fn in ['foo', 'bar', 'caz']:
	os.unlink(fn)
	