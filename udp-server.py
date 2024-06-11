import random
import socket

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind(('', 8080))

print('Started server on udp://:8080')

while True:
    message, address = server_socket.recvfrom(1024)
    print(message)
    server_socket.sendto(b'Multumesc pentru mesaj\n', address)
