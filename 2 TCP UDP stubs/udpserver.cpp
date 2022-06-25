/*
	ruby udpclientemul.rb 10.0.178.250:9000 send.txt
	cl.exe udpserver.cpp
*/ 
#define WIN32_LEAN_AND_MEAN 

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h> 
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") 
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>

#define sock_num 512


#ifdef _WIN32  
	int flags = 0; 
#else  
	int flags = MSG_NOSIGNAL; 
#endif 

int set_non_block_mode(int s)
{
#ifdef _WIN32  
	unsigned long mode = 1;  
	return ioctlsocket(s, FIONBIO, &mode); 
#else  
	int fl = fcntl(s, F_GETFL, 0);  
	return fcntl(s, F_SETFL, fl | O_NONBLOCK); 
#endif 
} 

int init() 
{ 
	#ifdef _WIN32 
		// Для Windows следует вызвать WSAStartup перед началом 
		//использования сокетов 
		WSADATA wsa_data; 
		return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data)); 
	#else 
		return 1; // Для других ОС действий не требуется 
	#endif 
}

void deinit() 
{ 
	#ifdef _WIN32 
	// Для Windows следует вызвать WSACleanup в конце работы 
		WSACleanup(); 
	#else // Для других ОС действий не требуется 
	#endif 
}

int sock_err(const char* function, int s) 
{ 
	int err; 
	#ifdef _WIN32 
		err = WSAGetLastError();
	#else
		err = errno; 
	#endif
	
	fprintf(stderr, "%s: socket error: %d\n", function, err); 
	return -1; 
}

struct clients_mass
{
    sockaddr_in addr;
    bool received[256];
    char datagr_to_send[80];
};
	struct clients_mass clients[sock_num];


int sent_to_client(int socket, int num_client)//, char num_mesage)
{
	int len = 0;
	for (int i = 0; i < sock_num; ++i)
	{
		if(clients[num_client].received[i] == 1)
		{
			// printf("num received = %d from client %d\n", i, num_client);
			clients[num_client].datagr_to_send[len*4] = (i >> 24) & 0xff;
			clients[num_client].datagr_to_send[len*4 + 1] = (i >> 16) & 0xff;
			clients[num_client].datagr_to_send[len*4 + 2] = (i >> 8) & 0xff;
			clients[num_client].datagr_to_send[len*4 + 3] = i & 0xff;
			len++;
		}
	}
	// printf("\n");

	int e = sendto(socket, clients[num_client].datagr_to_send, 80, 0, (struct sockaddr*)&clients[num_client].addr, sizeof(sockaddr));
	if (e == -1)
		printf("send error\n");

	return e;
}

char check_if_new(struct sockaddr_in client_to_check)
{
	for (int i = 0; i < sock_num; ++i)
		if(clients[i].addr.sin_addr.S_un.S_addr == client_to_check.sin_addr.S_un.S_addr &&
           clients[i].addr.sin_port == client_to_check.sin_port)
		{
			return i;			
		}

	return -1;
}

void print_file(FILE* msg, int info, char type)
{
	char buffer[256] = {0};
	char temp_char[9] = {0};
	char time_char[9] = {'0', '0', ':', '0', '0', ':', '0', '0', ' '};
	int temp_int = 0;
	int iter = 7;
	if(type == 0)//		date - ok
	{
		temp_int = info%100;
		info /= 100;
		temp_char[2] = '\0';
		temp_char[1] = temp_int%10 + '0';
		temp_char[0] = temp_int/10 + '0';
		strcat(buffer, temp_char);
		strcat(buffer, ".");
		temp_int = info%100;
		info /= 100;
		temp_char[2] = '\0';
		temp_char[1] = temp_int%10 + '0';
		temp_char[0] = temp_int/10 + '0';
		strcat(buffer, temp_char);
		strcat(buffer, ".");
		temp_char[4] = '\0';
		temp_char[3] = info%10 + '0';
		info /= 10;
		temp_char[2] = info%10 + '0';
		info /= 10;
		temp_char[1] = info%10 + '0';
		info /= 10;
		temp_char[0] = info%10 + '0';
		strcat(buffer, temp_char);
		strcat(buffer, " \0");
		fprintf(msg, "%s", buffer);
	}
	if(type == 1)// 	time - reverce copy
	{
		while(info/10)
		{
			temp_int = info%10;
			time_char[iter] = temp_int + '0';
			info /= 10;
			iter--;
			if(iter == 5 || iter == 2) iter--;
		}
		temp_int = info%10;
		time_char[iter] = temp_int + '0';
		fprintf(msg, "%s", time_char);
	}
	

	return;
}

char procced_buffer(FILE* msg, char *buffer, int num_client)
{
	int num_message = 0;
	char temp_char[4];
	char Message[210000] = {0};
	int len = 0;
	unsigned int port = ntohs(clients[num_client].addr.sin_port);
	memcpy(&num_message, buffer, 4);
	num_message = ntohl(num_message);
	// printf("clients[%d].received[%d] = %d\n", num_client, num_message, clients[num_client].received[num_message]);

	if(clients[num_client].received[num_message])
	{
		printf("already received\n");
		return -4;//return already received
	}	

	clients[num_client].received[num_message] = 1;
	// printf("clients[%d].received[%d] = %d\n", num_client, num_message, clients[num_client].received[num_message]);


	fprintf(msg, "%d.%d.%d.%d:%d ", (clients[num_client].addr.sin_addr.s_addr & 0xff), 
									(clients[num_client].addr.sin_addr.s_addr >> 8 & 0xff), 
									(clients[num_client].addr.sin_addr.s_addr >> 16 & 0xff), 
									(clients[num_client].addr.sin_addr.s_addr >> 24 & 0xff), port);

	for (int j = 1; j < 6; ++j)
	{
		if(j != 5)
		for (int i = 0; i < 4; ++i)
		{
			temp_char[i] = buffer[i + j*4];
		}
		switch(j)
		{
			case 1: 
				memcpy(&num_message, temp_char, 4);
				num_message = ntohl(num_message);
				print_file(msg, num_message, 0);
				break;
			case 2: 
				memcpy(&num_message, temp_char, 4);
				num_message = ntohl(num_message);
				print_file(msg, num_message, 1);
				break;
			case 3:
				memcpy(&num_message, temp_char, 4);
				num_message = ntohl(num_message);
				print_file(msg, num_message, 1);
				break;
			case 4: 
				memcpy(&num_message, temp_char, 4);
				num_message = ntohl(num_message);
				len = num_message;
				// printf("Len = %d\n", len);
				break;
			case 5:
				memset(Message, '\0', 210000);
				for (int k = 0; k < len; ++k)
				{
					Message[k] = buffer[k + j*4];
				}
				// printf("Mess is %s\n", Message);
				fprintf(msg, "%s\n", Message);
				if(Message[0] == 's' && Message[1] == 't' && Message[2] == 'o' && Message[3] == 'p' && 
				  (Message[4] == '\n' || Message[4] == '\0' || Message[4] == '\r'))
					return 4;//return stop
				break;
		}
	}

	return 0;
}

int main(int argc, char* argv[])
{
	if(atoi(argv[1]) < 0)
	{
		printf("%s\n", "Wrong port from");
		return -1;
	}
	if(atoi(argv[2]) < 0)
	{
		printf("%s\n", "Wrong port to");
		return -1;
	}

	char buffer[210000];
	char send_data[80];
	int from = atoi(argv[1]), to = atoi(argv[2]);
	int differ = to - from + 1;
	int num_connected = 0;
	struct pollfd pfd_c[sock_num];
	struct pollfd *pfd_s = (struct pollfd*)malloc(sizeof(struct pollfd) * differ);
	FILE* file_out = fopen("msg.txt", "w");
	int int_scan = 0;
	struct sockaddr_in test_sock;
	int *client_sockets = (int*)malloc(sizeof(int) * sock_num);
	struct sockaddr_in *clients_strct = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in) * sock_num);


	int *listen_ports = (int*)malloc(sizeof(int) * differ);
	struct sockaddr_in *servers_strct = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in) * differ);

	for (int i = 0; i < sock_num; ++i)
	{
		memset(clients[i].received, '\0', 256);
		memset(clients[i].datagr_to_send, '\0', 80);
	}

	int addrlen = sizeof(servers_strct[0]);

	// Инициалиазация сетевой библиотеки  
	init();
	printf("listening\n");


	for (int i = 0; i < sock_num; i++) 
	{
		if(i < differ)
		{
			listen_ports[i] = socket(AF_INET, SOCK_DGRAM, 0);
			if (listen_ports[i] < 0) 
				return sock_err("socket", listen_ports[i]);

			set_non_block_mode(listen_ports[i]);

			// Заполнение структуры с адресом прослушивания узлов
	        memset(&servers_strct[i], 0, sizeof(servers_strct[i]));
	        servers_strct[i].sin_family = AF_INET;
	        servers_strct[i].sin_port = htons(from + i);
	        servers_strct[i].sin_addr.s_addr = htonl(INADDR_ANY);

	        // Связь адреса и сокета, чтобы он мог принимать входящие дейтаграммы 
	        if (bind(listen_ports[i], (struct sockaddr*) &servers_strct[i], sizeof(servers_strct[i])) < 0) 
	            return sock_err("bind", listen_ports[i]);
			pfd_s[i].fd = listen_ports[i];
			pfd_s[i].events = POLLIN;
		}
		pfd_c[i].fd = client_sockets[i];
		pfd_c[i].events = POLLIN | POLLOUT | POLLHUP;
	}


	do
	{/////22222

	// printf("none\n");
		int ev_cnt = WSAPoll(pfd_s, differ, 3000);
		if (ev_cnt > 0) 
		{
			for (int i = 0; i < differ; i++)
			{
				if (pfd_s[i].revents & POLLERR)
				{
					printf("sock error\n");
				}
				if (pfd_s[i].revents & POLLHUP)
				{
					printf("exit server\n");
				}
				if (pfd_s[i].revents & POLLIN)
				{ // Сокет cs[i] доступен на чтение, можно вызывать recv/recvfrom
					if(int recv = recvfrom(listen_ports[i], buffer, 210000, 0, (struct sockaddr*) &test_sock, &addrlen) > 0)
					{//Receive from listen_port[i] and write info to buffer client_info to struct test_sock for compare

						int num_client = check_if_new(test_sock);
						if(num_client == -1)
						{
							// printf("New\n");
							//Создание сокета и заполнение полей структур
							
							client_sockets[num_connected] = socket(AF_INET, SOCK_DGRAM, 0);
							if (client_sockets[num_connected] < 0) 
								return sock_err("socket", client_sockets[num_connected]);

							// set_non_block_mode(client_sockets[num_connected]);

							// Заполнение структуры с адресом прослушивания узлов
					        memset(&clients_strct[num_connected], 0, sizeof(clients_strct[num_connected]));
					        clients_strct[num_connected].sin_family = AF_INET;
					        clients_strct[num_connected].sin_port = test_sock.sin_port;
					        clients_strct[num_connected].sin_addr.s_addr = test_sock.sin_addr.s_addr;

							pfd_c[num_connected].fd = client_sockets[num_connected];
							pfd_c[num_connected].events = POLLIN | POLLOUT;

							unsigned int port = ntohs(clients_strct[0].sin_port);
							printf("new client detected: %d.%d.%d.%d:%d\n", (clients_strct[0].sin_addr.s_addr & 0xff), 
																			(clients_strct[0].sin_addr.s_addr >> 8 & 0xff), 
																			(clients_strct[0].sin_addr.s_addr >> 16 & 0xff), 
																			(clients_strct[0].sin_addr.s_addr >> 24 & 0xff), port);
		                    clients[num_connected].addr = test_sock;
		                    num_connected++;
	                		if(procced_buffer(file_out, buffer, num_connected - 1) == 4)
	                		{
								int e = sent_to_client(client_sockets[num_connected - 1], num_connected - 1);
	                			goto end;
	                		}
							int e = sent_to_client(client_sockets[num_connected - 1], num_connected - 1);
	                	}


	                	else
	                	{
	                		// printf("Just was\n");
	                		if(procced_buffer(file_out, buffer, num_client) == 4)
	                		{
								int e = sent_to_client(client_sockets[num_client], num_client);
	                			goto end;
	                		}
							int e = sent_to_client(client_sockets[num_client], num_client);
	                	}
					}
	            else
	            {
	               // Ошибка считывания полученной дейтаграммы 
	                return sock_err("recvfrom pc", pfd_c[i].fd); 
	            }
				}
			}
		}
		else
		{
			printf("timeout\n");
			break;
		}
 	}while (1); 



end:
 	// Закрытие сокета
 	for (int i = 0; i < sock_num; i++)
	{
		if(i < differ)
        	closesocket(listen_ports[i]);
        closesocket(client_sockets[i]);
    }
 	deinit();
 	fclose(file_out);

 	return 0;
}
