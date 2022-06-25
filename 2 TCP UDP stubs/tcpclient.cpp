#ifdef _WIN32 
	#define WIN32_LEAN_AND_MEAN 
	#include <windows.h> 
	#include <winsock2.h> 
	#include <ws2tcpip.h> // Директива линковщику: использовать библиотеку сокетов 
	#pragma comment(lib, "ws2_32.lib")
#else // LINUX 
	#include <sys/types.h> 
	#include <sys/socket.h> 
	#include <netdb.h> 
	#include <errno.h> 
#endif

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include "windows.h"

/*
struct sockaddr_in 
{ 
	short sin_family; //семейство протоколов: AF_INET 
	unsigned short sin_port; //номер порта 
	struct in_addr sin_addr; //IP-адрес 
	char sin_zero[8]; //не используется, заполняется нулями 
};*/

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

void s_close(int s) 
{ 
	#ifdef _WIN32 
		closesocket(s); 
	#else 
		close(s); 
	#endif 
}
   
int recv_response(int s, int number) 
{ 
	char buffer[128]; 
	int res;
	int counter = 0;
	memset(buffer, '\0', 128);
	// Принятие очередного блока данных. 
	// Если соединение будет разорвано удаленным узлом recv вернет 0 
	while((res = recv(s, buffer, sizeof(buffer), 0)) > 0) 
	{  
		counter++;
		printf("%s\n", buffer); 
		// sleep(1000);
		if(counter == number)
			return 0;
		memset(buffer, '\0', 128);
	}
	if (res < 0)
	{
		return sock_err("recv", s);
	}

	return 0; 
}

bool check_if_correct(char *string_to_check)
{
	if(string_to_check[2] != '.' || string_to_check[5] != '.' || string_to_check[13] != ':' || string_to_check[16] != ':' 
		|| string_to_check[22] != ':' || string_to_check[25] != ':')
		return 1;		

	return 0;
}

int date_time_to_number(char* string, char flag)
{
	//01.01.1999 -> 19990101
	//14:15:16 -> 161514
	int digit = 0;
	char temp_char[16] = {0};
	if(flag == 0)
	{
		temp_char[0] = string[0];
		temp_char[1] = string[1];
		digit = atoi(temp_char);
		memset(temp_char, '\0', 16);
		temp_char[0] = string[3];
		temp_char[1] = string[4];
		digit += atoi(temp_char)*100;
		memset(temp_char, '\0', 16);
		temp_char[0] = string[6];
		temp_char[1] = string[7];
		temp_char[2] = string[8];
		temp_char[3] = string[9];
		digit += atoi(temp_char)*10000;
	}
	if(flag == 1)
	{
		temp_char[0] = string[11];
		temp_char[1] = string[12];
		digit = atoi(temp_char)*10000;
		memset(temp_char, '\0', 16);
		temp_char[0] = string[14];
		temp_char[1] = string[15];
		digit += atoi(temp_char)*100;
		memset(temp_char, '\0', 16);
		temp_char[0] = string[17];
		temp_char[1] = string[18];
		digit += atoi(temp_char);
	}
	if(flag == 2)
	{
		temp_char[0] = string[20];
		temp_char[1] = string[21];
		digit = atoi(temp_char)*10000;
		memset(temp_char, '\0', 16);
		temp_char[0] = string[23];
		temp_char[1] = string[24];
		digit += atoi(temp_char)*100;
		memset(temp_char, '\0', 16);
		temp_char[0] = string[26];
		temp_char[1] = string[27];
		digit += atoi(temp_char);
	}

	return digit;
}


void print_mass(char* massive, int length)
{

	for (int i = 0; i < length; ++i)
	{
		printf("%c\n", massive[i]);
	}
	return;
}


int main(int argc, char* argv[]) 
{
	/*
	ruby tcpserveremul.rb 9000
	cl.exe tcpclient.cpp -o tcpclient.exe
	g++ tcpclient.cpp -o tcpclient.exe -lws2_32
	tcpclient.exe 192.168.0.102:9000 tcp_to_send.txt
	*/

	short port = 0;
	unsigned long temp_int = 0;
	int number_of_messages = 0;
	char long_temp_char[21000];
	char temp_char[64], ip_address[20];//, port[4];
	char correct_message[8][21000] = {0};
	char file_name[16];
	char message[21000], test_char[12];
	//int test_socket; 
	FILE* file_to_send;
	struct sockaddr_in addr; 
	printf("Begin\n");
	memset(ip_address, '\0', sizeof(ip_address));
	memset(temp_char, '\0', sizeof(temp_char));
	memset(test_char, '\0', sizeof(test_char));

	//--------------- обработка строки cmd, запись ip и port
	for (int i = 0; argv[1][i] != ':'; ++i)
	{
		temp_int = i;
		if(argv[1][i] != ':')
			ip_address[i] = argv[1][i];
	}
	temp_int++;
	ip_address[temp_int] = '\0';


	temp_int++;
	for (int i = temp_int; argv[1][i] != '\0'; ++i)
		temp_char[i - temp_int] = argv[1][i];

	port = (short)atoi(temp_char);
	// printf("%s\n", ip_address);
	// printf("%d\n", port);

	//----------------------

	// Инициалиазация сетевой библиотеки init();
	// Создание TCP-сокета 
	init();
	int test_socket = socket(AF_INET, SOCK_STREAM, 0); 
	if (test_socket < 0) 
		return sock_err("socket", test_socket);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip_address);

	for (int i = 0; i < 10; ++i)
	{
		temp_int = connect(test_socket, (struct sockaddr*) &addr, sizeof(addr));
		if(temp_int == 0)
		{
			printf("Connected\n");
			break;
		}
		else printf("None\n");
	}

	if(temp_int != 0) 
	{ 
		s_close(test_socket); 
		return sock_err("connect", test_socket); 
	}

	strcpy(file_name, argv[2]);
	file_to_send = fopen(file_name, "r+");
	temp_int = 0;

	
	while(fgets(long_temp_char, 21000, file_to_send))
	{
		// printf("%s\n", long_temp_char);
		// Sleep(2000);
		if(!check_if_correct(long_temp_char) && long_temp_char[0] != '\n')
		{
			memcpy(correct_message[temp_int], long_temp_char, 21000);
			// printf("%s\n", correct_message[temp_int]);
			temp_int++;
		}
	}
	// printf("num = %d\n", temp_int);
	number_of_messages = temp_int;
	// printf("num = %d\n", number_of_messages);

		// Sleep(500);
	send(test_socket, "put", 3, 0);

	temp_int = 0;
	port = 0;
	memset(temp_char, '\0', 64);

	// for (int i = 0; i < number_of_messages; ++i)
	// {
	// 	printf("%s\n", correct_message[i]);
	// }
	//ЦИКОЛ ПОСЫЛКИ СООБЩЕНИЯ.
	for(int i = 0; i < number_of_messages; ++i)
	{
		// printf("%s\n", correct_message[i]);
		memset(message, '\0', 21000);
		temp_int = htonl(i);
		// Sleep(10);
		send(test_socket, (char*) &temp_int, 4, 0);

		temp_int = (unsigned long)htonl(date_time_to_number(correct_message[i], 0));
		// Sleep(10);
		send(test_socket, (char*) &temp_int, 4, 0);

		temp_int = (unsigned long)htonl(date_time_to_number(correct_message[i], 1));
		// printf("%d\n", date_time_to_number(correct_message[i], 1));
		// Sleep(10);
		send(test_socket, (char*) &temp_int, 4, 0);

		temp_int = (unsigned long)htonl(date_time_to_number(correct_message[i], 2));
		// printf("%d\n", date_time_to_number(correct_message[i], 2));
		// Sleep(10);
		send(test_socket, (char*) &temp_int, 4, 0);

		for(int j = 29; ; ++j)
		{
			// if(i > 6)
			// printf("j = %d  ", j);
			if(correct_message[i][j] == '\n' || correct_message[i][j] == '\0' || correct_message[i][j] == '\r')
			{
			// 	j++
			// printf("%s\n", correct_message[i]);
				break;
			}
			message[j - 29] = correct_message[i][j];
			// if(i > 6)
			// printf("%c\n", message[j - 29]);
			port = j;
		}
		message[port] = '\0';
		printf("%s\n", message);

		port = port - 28;//Длина сообщения. ТАк как char, то сразу в байтах
		// printf("len  %d\n", port);
		temp_int = htonl(port);
		// Sleep(500);
		send(test_socket, (char*) &temp_int, 4, 0);
		
		//Передаётся всё сообщение по байтам. Message - это только сообщение до конца строки
		// Sleep(500);
		send(test_socket, message, port, 0);
	}

	// Прием результата  
	recv_response(test_socket, number_of_messages);

	fclose(file_to_send);
	// Закрытие соединения 
	s_close(test_socket);
	deinit();

	return 0; 
}