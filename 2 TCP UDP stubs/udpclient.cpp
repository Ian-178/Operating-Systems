/*
ruby udpserveremul.rb 9000
gcc udpclient.cpp -o udpclient
./udpclient 192.168.0.169:9000 sample.txt
*/
#include <stdlib.h>
#include <unistd.h> 

#ifdef _WIN32  
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>  
#include <winsock2.h>  // Директива линковщику: использовать библиотеку сокетов   
#pragma comment(lib, "ws2_32.lib")  
#else // LINUX  
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>  
#include <sys/select.h>
#endif
#include <netinet/in.h>
#include <unistd.h>
 
#include <stdio.h> 
#include <string.h>

char count_connections = 0;

FILE* file_to_send;
int confirm[124] = {0};
int num_of_mess = 0;


void itoa(int number, char *string)
{
	int temp_int = number;
	int i = 0;
	while(number / 10 > 0)
	{
		i++;
		number /= 10;
	}
	number = temp_int;
	do
	{
		temp_int = number % 10;
		string[i] = temp_int + 48;
		i--;
		number /= 10;
	}while(number / 10 > 0);
	temp_int = number % 10;
	string[i] = temp_int + 48;
	return;
}

//char *strcat (char *destination, const char *append)

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
		s_close(s); 
	#endif 
}

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


void bit_visual(unsigned long input, char degree)//2147483648	10000000 00000000 00000000 00000000
{
	unsigned long i = 1 << degree;
	while (true) 
	{
	    if (input & i)
	        printf("1");
	    else
	        printf("0"); 
	    if (i == 1)
	        break;
	    i >>= 1;
	}
	printf("\n");
	return;
}
 
 
// Функция принимает дейтаграмму от удаленной стороны.  
// Возвращает 0, если в течение 100 миллисекунд не было получено ни одной дейтаграммы 
unsigned int recv_response(int s, char *str) 
{
	#ifdef _WIN32  
		int flags = 0; 
	#else  
		int flags = MSG_NOSIGNAL; 
	#endif

	unsigned long us;


	char datagram[1024];  
	struct timeval tv = {0, 100*1000}; // 100 msec
	int res;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(s, &fds);
	 
	//Проверка - если в сокете входящие дейтаграммы
	//(ожидание в течение tv)
	res = select(s + 1, &fds, 0, 0, &tv);
	if(res != 0)
		count_connections = 0;
	count_connections++;
	if(count_connections == 15)
		return -110;
	// printf("res = %d\n", res);

	if (res > 0)
	{
	// Данные есть, считывание их
		struct sockaddr_in addr;
		int addrlen = sizeof(addr);
		int received = recvfrom(s, str, sizeof(str), flags, (struct sockaddr*) &addr, (socklen_t*)&addrlen);
		if (received <= 0)
		{    // Ошибка считывания полученной дейтаграммы
			sock_err("recvfrom", s);
			return 0;
		} 
	 
	  	return 1;
	}
	else
	  	if (res == 0)
	  	{   // Данных в сокете нет, возврат ошибки
	  		return 0;
	  	}  
	  	else
	  	{
	  		sock_err("select", s);
	  		return 0;
	  	}
}

bool check_if_correct(char *string_to_check)
{
	if(string_to_check[2] != '.' || string_to_check[5] != '.' || string_to_check[13] != ':' || string_to_check[16] != ':' 
		|| string_to_check[22] != ':' || string_to_check[25] != ':')
		return 1;		

	return 0;
}

int date_time_to_number(char* string, short type)
{
	int digit = 0;
	char temp_char[16] = {0};
	//31.12.2017 18:19:59 18:12:59 Message
	// \|/
	//20171231
	//0		   9 11	    18 20    27


	if (type == 0)
	{
		memset(temp_char, '\0', sizeof(temp_char));
		// printf("string is %s\n", string);

		temp_char[0] = string[6]; temp_char[1] = string[7];
		temp_char[2] = string[8]; temp_char[3] = string[9];
		temp_char[4] = string[3]; temp_char[5] = string[4];
		temp_char[6] = string[0]; temp_char[7] = string[1];
		temp_char[8] = '\0';
		return strtoul(temp_char, 0, 10);
	}

	if (type == 1)
	{
		temp_char[0] = string[11]; temp_char[1] = string[12];
		temp_char[2] = string[14]; temp_char[3] = string[15];
		temp_char[4] = string[17]; temp_char[5] = string[18];
		return strtoul(temp_char, 0, 10);	
	}


	if (type == 2)
	{
		temp_char[0] = string[11 + 9]; temp_char[1] = string[12 + 9];
		temp_char[2] = string[14 + 9]; temp_char[3] = string[15 + 9];
		temp_char[4] = string[17 + 9]; temp_char[5] = string[18 + 9];
		return strtoul(temp_char, 0, 10);
	}
	
	return 0;
}


void print_mass(char* massive, int length)
{

	for (int i = 0; i < length; ++i)
	{
		printf("%c\n", massive[i]);
	}
	return;
}

void send_request(int s, struct sockaddr_in* addr, char* datagram) 
{
	int flags = MSG_NOSIGNAL; 
	int res = sendto(s, datagram, 10000, flags, (struct sockaddr*) addr, sizeof(struct sockaddr_in));
	if (res <= 0) 
		sock_err("sendto", s); 
}


int main(int argc, char* argv[])
{
	#ifdef _WIN32  
		int flags = 0; 
	#else  
		int flags = MSG_NOSIGNAL; 
	#endif

	int sock = 0, temp_int = 0;
	unsigned long un_int;
	char messages[128][21000] = {0};
	char datagr[128][21000] = {0};
	struct sockaddr_in addr;
	int port = 0;
	char temp_char[21000] = {0}, ip_address[20] = {0}, file_name[32] = {0};
	strcpy(file_name, argv[2]);
	memset(ip_address, '\0', sizeof(ip_address));
	memset(temp_char, 0, sizeof(temp_char));

	int addrlen = sizeof(addr);
	file_to_send = fopen(file_name, "r+");
	if(file_to_send == 0)
	{
		printf("error\n");
		return 0;
	}

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
	memset(temp_char, '\0', sizeof(temp_char));
	temp_int = 0;
	//----------------------
 	init();

 	while(fgets(temp_char, 21000, file_to_send))
 	{
 		printf("%s\n", temp_char);
	 	memcpy(messages[temp_int], temp_char, sizeof(temp_char));
		memset(temp_char, '\0', sizeof(temp_char));
		temp_int++;
 	}
 	num_of_mess = temp_int;
 	strcat(messages[num_of_mess - 1], "\n");
 	temp_int = 0;
 
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return sock_err("socket", sock);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	port = 0;
	
again:
	for (int i = 0; i < num_of_mess; i++)
	{
		if(confirm[i] == 0)
		{
			memset(datagr[i], '\0', sizeof(datagr[i]));
			memset(temp_char, '\0', sizeof(temp_char));
			un_int = (unsigned long)i;
			datagr[i][0] = (un_int >> 24) & 0xff;
			datagr[i][1] = (un_int >> 16) & 0xff;
			datagr[i][2] = (un_int >> 8) & 0xff;
			datagr[i][3] = (un_int) & 0xff;

			temp_int = date_time_to_number(messages[i], 0);
			un_int = (unsigned long)temp_int;
			datagr[i][4] = (un_int >> 24) & 0xff;
			datagr[i][5] = (un_int >> 16) & 0xff;
			datagr[i][6] = (un_int >> 8) & 0xff;
			datagr[i][7] = (un_int) & 0xff;


			temp_int = date_time_to_number(messages[i], 1);
			un_int = (unsigned long)temp_int;
			datagr[i][8] = (un_int >> 24) & 0xff;
			datagr[i][9] = (un_int >> 16) & 0xff;
			datagr[i][10] = (un_int >> 8) & 0xff;
			datagr[i][11] = (un_int) & 0xff;


			temp_int = date_time_to_number(messages[i], 2);
			un_int = (unsigned long)temp_int;
			datagr[i][12] = (un_int >> 24) & 0xff;
			datagr[i][13] = (un_int >> 16) & 0xff;
			datagr[i][14] = (un_int >> 8) & 0xff;
			datagr[i][15] = (un_int) & 0xff;


			for (int j = 0; j < sizeof(messages[i]); ++j)
			{
				if(messages[i][j] == '\n')
					break;
				temp_int = j;
			}
			temp_int -= 28;
			un_int = (unsigned long)temp_int;
			datagr[i][16] = (un_int >> 24) & 0xff;
			datagr[i][17] = (un_int >> 16) & 0xff;
			datagr[i][18] = (un_int >> 8) & 0xff;
			datagr[i][19] = (un_int) & 0xff;

			for (int j = 0; j < temp_int; ++j)
			{
				datagr[i][j + 20] = messages[i][j + 29];
			}

			// int res = sendto(sock, datagr[i], 10000, 0, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));
			send_request(sock, &addr, (char*)datagr[i]);// отправили k-ю датаграмму
			int result = recv_response(sock, temp_char);
			if(result == -110)
			{
				printf("connection error\n");
				return 0;
			}
			un_int = strtoul(temp_char, 0, 10);
		}
		confirm[temp_char[3]] = 1;
		usleep(100000);
	}

	for (int j = 0; j < num_of_mess; ++j)
	{
		if(confirm[j] == 0)
			goto again;
	}

	// Закрытие сокета
	close(sock);
 	deinit();
 	return 0;
}