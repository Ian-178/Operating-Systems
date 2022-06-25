/*
ruby tcpclientemul.rb 192.168.1.35:9000 sample.txt
g++ tcpserver.cpp -o tcpserver
valgrind ./tcpserver 9000
*/

 
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>

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
#endif

//global varieties
short port;
FILE* file_to_print;
int mess_count[64] = {0};//for counting messes
int was_put[64] = {0};//for info about put

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
   
int recv_response(int s, FILE* f) 
{ 
	char buffer[256]; 
	int res;
	// Принятие очередного блока данных. 
	// Если соединение будет разорвано удаленным узлом recv вернет 0 
	while ((res = recv(s, buffer, sizeof(buffer), 0)) > 0) 
	{ 
		fwrite(buffer, 1, res, f); 
		printf(" %d bytes received\n", res); 
	}
	if (res < 0)
		return sock_err("recv", s);

	return 0; 
}


void int_to_date(int digit, char date_in[11])
{
	//20171231   31.12.2017
	char date[11] = {0};
	int temp_int;

	temp_int = digit%10;
	digit = digit/10;
	date[1] = temp_int + 48;
	temp_int = digit%10;
	digit = digit/10;
	date[0] = temp_int + 48;//date
	date[2] = '.';

	temp_int = digit%10;
	digit = digit/10;
	date[4] = temp_int + 48;
	temp_int = digit%10;
	digit = digit/10;
	date[3] = temp_int + 48;//month
	date[5] = '.';

	temp_int = digit%10;
	digit = digit/10;
	date[9] = temp_int + 48;
	temp_int = digit%10;
	digit = digit/10;
	date[8] = temp_int + 48;
	temp_int = digit%10;
	digit = digit/10;
	date[7] = temp_int + 48;
	temp_int = digit%10;
	digit = digit/10;
	date[6] = temp_int + 48;//year

	memcpy(date_in, date, 11);
	return;
}

void int_to_time(int digit, char time_in[9])
{
	//181959    18:19:59
	char time[9] = {0};
	int temp_int;

	temp_int = digit%10;
	digit = digit/10;
	time[7] = temp_int + 48;
	temp_int = digit%10;
	digit = digit/10;
	time[6] = temp_int + 48;//sec
	time[5] = ':';

	temp_int = digit%10;
	digit = digit/10;
	time[4] = temp_int + 48;
	temp_int = digit%10;
	digit = digit/10;
	time[3] = temp_int + 48;//min
	time[2] = ':';

	temp_int = digit%10;
	digit = digit/10;
	time[1] = temp_int + 48;
	temp_int = digit%10;
	digit = digit/10;
	time[0] = temp_int + 48;//hour

	memcpy(time_in, time, 9);
	return;

}

int rec_n_trans_4_bytes(int test_socket, char type, char *temp)
{	//Типы работы: 0,3 - обработка номера сообщения/количества байт сообщения(ничего делать не надо), 
	//1 - дата(перевод), 2 - время(перевод)
	//нужно вернуть строку в *temp в случае 1 или 2
	int rcv;
	char date[11], time[9];
	int int_digit = 0, temp_int = 0;
	char temp_char[8] = {0};
	memset(temp_char, '\0', 8);
	rcv = recv(test_socket, temp_char, 4, 0);
	if(rcv <= 0)
	{
		//printf("Something is wrong\n");
		return -100;
	}
	if(strstr(temp_char, "put"))
		return -3;
	memcpy(&int_digit, temp_char, 4);
	int_digit = ntohl(int_digit);

	if(type == 0)
	{
		return int_digit;
	}
	if(type == 1)
	{
		int_to_date(int_digit, temp);
		// printf("%s\n", date);
		return int_digit;
	}
	if(type == 2)
	{
		int_to_time(int_digit, temp);
		// printf("%s\n", time);
		return int_digit;
	}
}


int recv_string(int test_socket, int client_num, struct sockaddr_in addr) 
{
	char buffer[10000] = {0}, data_string[256] = {0};
	char date[11], time[9];
	int curlen = 0;
	int int_digit = 0, rcv = 0, iter = 4;

	inet_ntop(AF_INET, &addr.sin_addr, date, 16);
	strcat (date, ":");
	strcat (data_string, date);
	memset(date, '\0', 11);
	itoa(port, date);
	strcat (data_string, date);
	strcat (data_string, " ");
	memset(date, '\0', 11);
	//Сначала - принять номер сообщения 4 байта
	//Далее по 4/4/4/4 дата/время/время/длина сообщения
		//Номер сообщения
		int_digit = rec_n_trans_4_bytes(test_socket, 0, NULL);
		if(int_digit == -1)
			return -10;
		if(int_digit == -3)
			return -3;
		//Дата
		int_digit = rec_n_trans_4_bytes(test_socket, 1, date);
		if(int_digit == -1)
			return -10;
		//fprintf(stdout, "date - %s' ", date);
		strcat (date, " ");
		strcat (data_string, date);

		//Время
		int_digit = rec_n_trans_4_bytes(test_socket, 2, time);
		if(int_digit == -1)
			return -10;
		//fprintf(stdout, "time - %s' ", time);
		strcat (time, " ");
		strcat (data_string, time);

		//Время
		int_digit = rec_n_trans_4_bytes(test_socket, 2, time);
		if(int_digit == -1)
			return -10;
		if(int_digit == -1)
			return -10;
		//fprintf(stdout, "time - %s' ", time);		
		strcat (time, " ");
		strcat (data_string, time);

		//Длина сообщения
		int_digit = rec_n_trans_4_bytes(test_socket, 0, NULL);
		if(int_digit == -1)
			return -10;

		//Сообщение
		rcv = recv(test_socket, buffer, int_digit + 1, 0);	
		strcat (buffer, "\n");
		strcat (data_string, buffer);
		fprintf(file_to_print, "%s", data_string);
		mess_count[client_num]++;

		if(buffer[0] == 's' && buffer[1] == 't' && buffer[2] == 'o' && buffer[3] == 'p' && buffer[4] == '\n')
			return -4;
		memset(data_string, '\0', 256);
	  	//scanf("%s\n", buffer);
		memset(buffer, '\0', 10000);
		memset(date, '\0', 11);
		memset(time, '\0', 9);

		for (curlen = 0; curlen < int_digit; curlen++)   
			if (buffer[curlen] == '\n')
				return curlen;
 
	  	if (curlen > 5000)   
	  	{    
	  		printf("input string too large\n");   
	  		return 5000;   
	  	}

 	return curlen; 
} 

int send_notice(int cs, int i) 
{ 
	int ret; 
 
	#ifdef _WIN32  
	int flags = 0; 
	#else  
	int flags = MSG_NOSIGNAL; 
	#endif 
 
	while(mess_count[i] > 0)
	{
 		ret = send(cs, "ok", 2, flags);
 		mess_count[i]--;
 		if (ret < 0)    
 			return sock_err("send", cs);
 	}

 	return 0; 
} 


int main(int argc, char* argv[]) 
{
	char input;
	int i = 0, s_count = 0;
	int temp_int;
	char buffer[64] = {0};
	socklen_t addrlen = 0;
	port = (short)atoi(argv[1]);
	file_to_print = fopen("msg.txt", "w+");


	int ls;
	int cs[64];
	struct sockaddr_in cs_st[64];
	struct sockaddr_in addr;
	fd_set rfd;
	fd_set wfd;
	int nfds = ls;
	struct timeval tv = { 10, 10 };

 	if(file_to_print == 0)
 	{
 		printf("Bad file\n");
 		return 0;
 	}
 	// Инициалиазация сетевой библиотеки
	init();
 
 	// Создание TCP-сокета
 	ls = socket(AF_INET, SOCK_STREAM, 0);
 	if (ls < 0)
 		return sock_err("socket", ls); 
 	// Заполнение адреса прослушивания  
 	memset(&addr, 0, sizeof(addr));  
 	addr.sin_family = AF_INET;  
 	addr.sin_port = htons(port); // Сервер прослушивает порт 9000  
 	addr.sin_addr.s_addr = htonl(INADDR_ANY); // Все адреса
 	set_non_block_mode(ls);

	addrlen = sizeof(addr);

 	// Связывание сокета и адреса прослушивания  
 	while(bind(ls, (struct sockaddr*) &addr, sizeof(addr)) < 0)
 		return sock_err("bind", ls); 

	// Начало прослушивания  
	if(listen(ls, 5) < 0)
		return sock_err("listen", ls);
 //	printf("listening\n");



 	while(1)
 	{
 		FD_ZERO(&rfd);// очищает и инициализирует объявленную структуру fd_set
		FD_ZERO(&wfd);
		FD_SET(ls, &rfd);// добавляет сокет в fd_set (rfd)
		nfds = ls;

		for(i = 0; i < s_count; i++)
		{
			FD_SET(cs[i], &rfd);
			FD_SET(cs[i], &wfd);
			if (nfds < cs[i])
				nfds = cs[i];
		}

		if(select(nfds + 1, &rfd, &wfd, 0, &tv) >= 0)
		{
			if (FD_ISSET(ls, &rfd))
			{// Принятие очередного подключившегося клиента
		 		cs[s_count] = accept(ls, (struct sockaddr*) &cs_st[s_count], (socklen_t*)&addrlen);
				if (cs[s_count] < 0)
				{
					return sock_err("accept", ls);
				}
				set_non_block_mode(cs[s_count]);
				inet_ntop(AF_INET, &cs_st[s_count].sin_addr, buffer, 16);
				fprintf(stdout, "%s:%i connect to server\n", buffer, port);
				s_count++;
			}
			for (i = 0; i < s_count; i++)
			{
				if (FD_ISSET(cs[i], &rfd))
				{// Сокет cs[i] доступен для чтения. Функция recv вернет данные, recvfrom - дейтаграмму
				//printf("Read\n");
				//scanf("%c\n", &input);
		  			temp_int = recv_string(cs[i], i, cs_st[i]);
		  			if(temp_int == -4)
		  			{
		  				printf("stop\n");
		  				return 0;
		  			}
				}
				if (FD_ISSET(cs[i], &wfd))
				{// Сокет cs[i] доступен для записи. Функция send и sendto будет успешно завершена
					if(send_notice(cs[i], i))
						return sock_err("send", cs[i]);
				}
			}
		}
		else
		{
			return sock_err("select", ls);
		}
 
 	}

	 
	// Отключение клиента   
	s_close(cs[i]); 
 	s_close(ls);  
 	deinit(); 
 
 	return 0; 
} 























/*
FILE* f; 
int num_received_msg[200];// тут находятся количества принятых сообщений от каждого 
//подключенного клиента i-й клиент, от него num_received_msg[i] сообщений 
short port; 
int s_count = -1;// количество сокетов уже подключенных клиентов 
int cs_put[200] = {0};// пришло put 1 или нет 0 от кажого подключенного клиента 
int recv_string(int cs, int i, struct sockaddr_in addr)// принять данные от клиента 
{ 
char Message[10000]; 
unsigned long int buf; 
int rcv; 
char time; 
char ip[16]; 
inet_ntop(AF_INET, &addr.sin_addr, ip, 16); // IP 

// put 
if(cs_put[i]!=1) 
{ 
rcv = recv(cs, (void*) &buf, 3, 0); 
if(rcv < 0) 
return sock_err("receive1", cs); 
cs_put[i] = 1; 
} 
// 4 байта - номер сообщения в исходном файле 
rcv = recv(cs, (void*)&buf, 4, 0); 
if(rcv < 0) 
return sock_err("receive2", cs); 
if(rcv == 0) 
return 0; 
fprintf(f, "%s:%i ", ip, port); //IP:порт клиента пробел 

// время 
for(int j = 0; j < 2; j++) 
{ 
rcv = recv(cs, (void*)&time, 1, 0); 
if(rcv < 0) 
return sock_err("receive3", cs); 
else 
{ 
if((int)time < 10) 
fprintf(f, "0"); 
fprintf(f, "%d:", (int)time); 
} 
} 

rcv = recv(cs, (void*)&time, 1, 0); 
if(rcv < 0) 
return sock_err("receive3", cs); 
else 
{ 
if((int)time < 10) 
fprintf(f, "0"); 
fprintf(f, "%d ", (int)time); 
} 

// время 
for(int j = 0; j < 2; j++) 
{ 
rcv = recv(cs, (void*)&time, 1, 0); 
if(rcv <= 0) 
return sock_err("receive3", cs); 
else 
{ 
if((int)time < 10) 
fprintf(f, "0"); 
fprintf(f, "%d:", (int)time); 
} 
} 

rcv = recv(cs, (void*)&time, 1, 0); 
if(rcv<=0) 
return sock_err("receive3", cs); 
else 
{ 
if((int)time < 10) 
fprintf(f, "0"); 
fprintf(f, "%d ", (int)time); 
} 

// BBB 
rcv = recv(cs, (void*)&buf, 4, 0); 
if(rcv<=0) 
return sock_err("receive4", cs); 
buf = ntohl(buf); 
fprintf(f, "%lu ", buf); 

// Message 
rcv = recv(cs, Message, 10000, 0); 
if(rcv<=0) 
return sock_err("receive5", cs); 
fprintf(f, "%s", Message); 

if(strncmp(Message, "stop", 4) == 0 && Message[4] == '\0') 
return 77; 
fprintf(f, "\n"); 

num_received_msg[i]++; 
return 0; 
}*/