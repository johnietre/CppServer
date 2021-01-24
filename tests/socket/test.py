from time import sleep
import socket as so
s = so.socket(so.AF_INET, so.SOCK_STREAM)
s.connect(("localhost", 8000))
# sleep(4)
s.send(b"hello")
print(str(s.recv(1024)))