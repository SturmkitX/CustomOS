import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  
sock.bind(('0.0.0.0', 12345))  
sock.listen(5)  
while True:  
    connection,address = sock.accept()  
    buf = connection.recv(1024)  
    print(buf)
    connection.send(b'Cacatule\n')    		
    connection.close()
    print('Connection closed')
	