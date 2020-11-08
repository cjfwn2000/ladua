# 우리가 개발하고자 하는 솔루션 프로그램의 내부흐름구조를
# 어떻게 짜야할 지 생각해내기 위해 가져왔다.
# 2015년 당시 본인의 컴퓨터네트워킹 학부과제다. (서버-클라이언트 일대다 채팅통신)

import sys, socket, select

SERVER_LIST = []

def main():
	if( len(sys.argv) < 2):
		print 'Usage: %s portNumber' % sys.argv[0]
		sys.exit()
	port = int(sys.argv[1])
	
	#소켓 생성
	serverSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	serverSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	#Binding
	serverSock.bind(('', port))

	serverSock.listen(10)

	#server socket을	readable socket connection list에 추가
	SERVER_LIST.append(serverSock)

	print('Server started...')

	while 1:
		#Select library를 이용하여 소켓을 다룰 수 있도록 한다.
		ready_to_read, ready_to_write, in_error = select.select(SERVER_LIST, [],[],0)

		for sock in ready_to_read:
			#새로운 연결 요청시
			if sock == serverSock:
				sockfd, addr = serverSock.accept()
				SERVER_LIST.append(sockfd)

				broadcast(serverSock, "Client Connected! (%s:%d) " % (addr[0],addr[1]))

			#이미 연결되어 있는 소켓
			else:
				try:
					data = sock.recv(1024)
					if data:
						broadcast(serverSock, '[%s:%d] %s' % (addr[0], addr[1], data))
					else:
						#빈 메시지이면 연결 종료로 간주
						if sock in SERVER_LIST:
							SERVER_LIST.remove(sock)

						broadcast(serverSock, 'This guy has been disconnected : [%s:%d]' % (addr[0], addr[1]))

				except:
					broadcast(serverSock, 'This guy has been disconnected : [%s:%d]' % (addr[0], addr[1]))
					continue
	
	# Disconnected
	serverSock.close()
	
#연결된 모든 Client에게 채팅메시지 전달.
def broadcast(serverSock, message):
	for socket in SERVER_LIST:
		if socket != serverSock:
			socket.send(message + '\r\n')
			
if __name__ == "__main__": main()