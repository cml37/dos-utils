import socket

UDP_IP = "192.168.1.255"
UDP_PORT = 20000
MESSAGE = "Hello, World! Push notifications in MS-DOS?  You bet!!!"

print("UDP target IP:", UDP_IP)
print("UDP target port:", UDP_PORT)
print("message:", MESSAGE)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

sock.sendto(bytes(MESSAGE, "utf-8"), (UDP_IP, UDP_PORT))
