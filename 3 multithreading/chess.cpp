#include <windows.h> 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cmath>
CRITICAL_SECTION cs;

//threads
//N L K
//K-strings f koordinates
//N - size of the field
//L figures we need to place

volatile 
int T = 0, 
	field_size = 0, 
	figures_left = 0, 
	num_of_coordinates = 0;

typedef struct
{
	unsigned char digit;
}startinfo;
startinfo* start_info;

int answer = 0, 
	Num_lines = 0,
	possible_lines = 0;

bool 	*flag_exit, 
		*dec_ind_of_lines;

HANDLE* threads;
FILE* test_file = fopen("test_file.txt", "w+");

int* mas_num_lines;
int number_of_l_of_coord = 0;


typedef struct
{
	int* mas_of_figures;
	char number_of_figures;
	unsigned char line_digit;
} str;
str* line;

typedef struct
{
	char x;
	char y;
} coord;
coord* coordinates;

typedef struct
{
	int s;//номер строки доски, с которой работаем
	int* ind_line;//массив индексов линий, из которых состоит доска
	int count;//количество фигур на доске
} board_t;

typedef struct
{
	int I; // индекс в списке строк, на котором остановились
	board_t board; // для каждого потока свой набор досок
} type_thr;
type_thr* thread;// для рекурсии

typedef struct
{
	int* ind_of_line;
	int len;
	int i;// индекс в массиве line, на котором остановились
} for_thr;
for_thr* ind;

int bit_eject(unsigned long input, int degree, int type)
{
	unsigned long i = 1 << degree;
	int count_bit = 0;
	while (true) 
	{
	    if (input & i)
	    {
	        // printf("1");
	        if(type == count_bit)
	        	return 1;
	    }
	    else
	    {
	        // printf("0"); 

	        if(type == count_bit)
	        	return 0;
	    }
	    if (i == 1)
	        break;
	    i >>= 1;
	    count_bit++;
	}
	// printf("\n");
	return 1976;
}

void array_shift(int position)
{
	for (int i = position; i < number_of_l_of_coord - 1; ++i)
	{
		mas_num_lines[i] = mas_num_lines[i + 1];
	}
	mas_num_lines[number_of_l_of_coord] = 0;
	number_of_l_of_coord--;
	return;
}


char read_file(void)
{
	char *p;
	char strThreadsNum[10] = { 0 }, strNLKnum[32] = { 0 }, tmp[32] = { 0 };
	int massive[3] = { 0 }, q = 0;
	bool flag_read = FALSE;
	FILE *input = fopen("input.txt", "r");
	if(input == 0)
	{
		printf("Error open file\n");
		return -1;
	}

	fgets(strThreadsNum, 10, input);
	fgets(strNLKnum, 32, input);
	p = strtok(strNLKnum, " ");
	while (p != NULL)
	{
		massive[q] = atoi(p);
		q++;
		p = strtok(NULL, " ");
	}
	T = atoi(strThreadsNum);
	field_size = massive[0];
	figures_left = massive[1];
	num_of_coordinates = massive[2];
	Num_lines = pow((float)2, field_size);

	// start_info = (startinfo*)malloc(field_size * sizeof(startinfo));

	coordinates = (coord*)malloc(num_of_coordinates * sizeof(coord));
	line = (str*)calloc(sizeof(str), Num_lines);
	thread = (type_thr*)malloc(T * sizeof(type_thr));
	flag_exit = (bool*)malloc(T * sizeof(bool));
	dec_ind_of_lines = (bool*)malloc(T * sizeof(bool));
	for (int i = 0; i < T; i++)
	{
		thread[i].I = i;
		thread[i].board.ind_line = (int*)calloc(field_size * sizeof(int), sizeof(int));
		thread[i].board.count = 0;
		thread[i].board.s = 0;
		flag_exit[i] = false;
		dec_ind_of_lines[i] = false;
	}

	for (int i = 0; i < num_of_coordinates; i++)
	{
		fgets(tmp, 32, input);
		char *p = strtok(tmp, " ");
		q = 0;
		while (p != NULL)
		{
			massive[q] = atoi(p);
			q++;
			p = strtok(NULL, " ");
		}
		coordinates[i].x = massive[0];
		coordinates[i].y = massive[1];
	}

	// Sort coordinates
	for (int i = 0; i < num_of_coordinates - 1; ++i)
		for (int j = 0; j < num_of_coordinates - i - 1; ++j)
			if(coordinates[j].x > coordinates[j + 1].x)
			{
				q = coordinates[j].x;
				coordinates[j].x = coordinates[j + 1].x;
				coordinates[j + 1].x = q;
				q = coordinates[j].y;
				coordinates[j].y = coordinates[j + 1].y;
				coordinates[j + 1].y = q;
			}
	// for (int i = 0; i < field_size; ++i)
	// 	start_info[i].digit = 0;

	// // q = coordinates[0].x;
	// for (int i = 0; i < num_of_coordinates; ++i)
	// {
	// 	start_info[coordinates[i].x].digit += pow(2, coordinates[i].y);
	// }
//(2)


	fclose(input);
	return 0;
}

void devide_lines_to_threads()
{
	// создание массивов с индексами строк для каждого потока (для 1ой строки доски)
	ind = (for_thr*)malloc(T * sizeof(for_thr));
	for (int i = 0; i < T; i++)
	{
		ind[i].ind_of_line = (int*)malloc(1 * sizeof(int));
		ind[i].len = 1;
		ind[i].i = 0;
		for (int j = 0; j < ind[i].len; j++)
		{
			int elem;
			if (j == 0)
				elem = i;
			else
				elem = ind[i].ind_of_line[j - 1] + T;

			if (elem < Num_lines)
			{
				ind[i].ind_of_line[j] = elem;
				ind[i].len++;
				ind[i].ind_of_line = (int*)realloc(ind[i].ind_of_line, ind[i].len * sizeof(int));
			}
			else
				break;
		}
		ind[i].len--;
	}
}

bool is_good_line(int idx, int s, int ind_line)
{
	// int time_1 = clock();
	int ind_s_1, razn = 0;
	if (s == 0)
	{
		return true;
	}
	else
	{
		for (int i = 0; i < field_size; i++)
		{
			if(line[ind_line].mas_of_figures[i] == 1)
			{
				for (int j = s - 1; j >= 0; j--)
				{
					razn++;// индекс линии в списке всех профилей 
					ind_s_1 = thread[idx].board.ind_line[j];// уже поставленная линия
					if((line[ind_s_1].mas_of_figures[i - razn] == 1 && i - razn >= 0) || 
						(line[ind_s_1].mas_of_figures[i + razn] == 1 && i + razn <= field_size - 1) )// фигура под боем
					{
						return false;
					}
				}
				razn = 0;
			}
		}
	}
	// if(clock() - time_1 > 100)
	// printf("is_good_line %d\n", clock() - time_1);
	return true;
}

bool check_coord(int s, int i)
{
	// int time_1 = clock();
	bool ret = true;
	int i_1 = 0, i_2 = 0;
	for (int k = 0; k < num_of_coordinates; k++)
	{
		int str = field_size - coordinates[k].y - 1;

		if(s == str && line[i].mas_of_figures[coordinates[k].x] != 1)
		{
			ret = false;
			break;
		}
		i_1 = k;
	}
	// if(clock() - time_1 > 100)
	// printf("check_coord %d\n", clock() - time_1);
	return ret;
}

DWORD WINAPI thread_func(void* param)
{
	int time_1 = 0, time_2 = 0;
	time_1 = clock();
	// printf("enter\n");
	int idx = ((char*)param - (char*)0);
	int tmp;

	if (flag_exit[idx] == true)
	{
		// printf("exit\n");
		return 0;
	}

	for (int i = thread[idx].I; i < Num_lines; i++)
	{		
		if (thread[idx].board.count > figures_left + num_of_coordinates && i != Num_lines - 1)
		{
			continue;
		}

		bool flag = false;

		if (thread[idx].board.s == 0)
		{
			tmp = idx;
			if (ind[idx].i < ind[idx].len)
			{
				tmp = ind[idx].ind_of_line[ind[idx].i];

				if(field_size >= 8 && num_of_coordinates > 0)
				{
					if(check_coord(thread[idx].board.s, tmp) == false)
					{
						ind[idx].i++;
						continue;
					}
				}
			}
			else
			{
				flag_exit[idx] = true;
			}
			ind[idx].i++;

			thread[idx].board.ind_line[0] = tmp;
			thread[idx].board.count = 0;
			thread[idx].board.count += line[tmp].number_of_figures;
			thread[idx].board.s++;
			thread[idx].I = 0;
			
			if (flag_exit[idx] == true)
			{
				break;
			}

			i = thread[idx].I;
			dec_ind_of_lines[idx] = true;
			continue;
		}
		else
		{
			if(dec_ind_of_lines[idx] == true)
			{
				i--;
				dec_ind_of_lines[idx] = false;
			}
			if (is_good_line(idx, thread[idx].board.s, i) == true)// хотим поставить i-й профиль на s-ю строку на доску № num_b 
			{
				if(field_size >= 8 && num_of_coordinates > 0)
				{
					if(check_coord(thread[idx].board.s, i) == false)
					{
						if(i != Num_lines - 1)
							continue;
					}
				}

				thread[idx].board.ind_line[thread[idx].board.s] = i;
				thread[idx].board.count = 0;
				for (int d = 0; d <= thread[idx].board.s; d++)
				{
					thread[idx].board.count += line[thread[idx].board.ind_line[d]].number_of_figures;
				}

				if (thread[idx].board.count == figures_left + num_of_coordinates && thread[idx].board.s == field_size - 1)
				{
					bool ex = false; 
					if (num_of_coordinates > 0) 
					{ 
						for (int k = 0; k < num_of_coordinates; k++) 
						{ 
							int str = field_size - 1 - coordinates[k].y;// тут будут номера строк, где ДОЛЖНЫ стоять фигуры 
							int ind_pr = thread[idx].board.ind_line[str];// по этим номерам строк берём индексы линий 
							if (line[ind_pr].mas_of_figures[coordinates[k].x] != 1) 
							{ 
								ex = true; 
								break; 
							} 
						} 

						if (ex == true) 
						{ 
						EnterCriticalSection(&cs); 
						answer--; 
						LeaveCriticalSection(&cs); 
						} 
					}
					if(ex == false)
					{
						for (int o = 0; o < field_size; ++o)
						{
							for (int j = 0; j < field_size; ++j)
							{
								fprintf(test_file, "%d ", line[thread[idx].board.ind_line[o]].mas_of_figures[j]);
							}
						fprintf(test_file, "\n");
						}

						fprintf(test_file, "\n");
					}
					EnterCriticalSection(&cs);
					answer++;
					LeaveCriticalSection(&cs);
				}

				if (thread[idx].board.s == field_size - 1)
				{
					flag = true;
			// printf("2 pos thread[idx].I = %d\n", thread[idx].I);
					thread[idx].I = i + 1;
				}
				else
				{
					thread[idx].board.s++;
				}

				if (flag == false)
				{
					i = thread[idx].I - 1;
			// printf("3 pos thread[idx].I = %d\n", thread[idx].I);
					continue;
				}

				if (flag_exit[idx] == true)
				{
					// printf("finally got you\n");
					break;
				}
			}
		}

		if (i == Num_lines - 1)
		{
			thread[idx].I = 0;
			i = 0;

			int a = field_size - 1, s = thread[idx].board.s;
			while (thread[idx].board.ind_line[a] == Num_lines - 1 && a >= 0)
			{
				s--;
				a--;
				i = thread[idx].board.ind_line[a];
			}

			if (s < 0)
			{
				// printf("break s < 0\n");
				break;
			}
			else
				thread[idx].board.s = s;
		}
	}
	// printf("exit normal\n");
	time_2 = clock();
	printf("idx %d time %d\n", idx, time_2 - time_1);
	return 0;
}

int main()
{
	// printf("8&40 = %d\n", 8&40);
	// return 0;



	int Start = 0, number_of_l = 1, count = 1;
	int temp_int = 0,
		calc_degree = 0;

	if(read_file() == -1)
		return -1;

	if(field_size == 1)
	{
		FILE *time = fopen("time.txt", "w+");
		fprintf(time, "0");
		fclose(time);

		FILE *out = fopen("output.txt", "w+");
		fprintf(out, "1");
		fclose(out);
		return 0;
	}
	for (int i = 1; i <= Num_lines; ++i)
	{
		temp_int = 0;
		line[i - 1].mas_of_figures = (int*)calloc(field_size, sizeof(int));
		line[i - 1].line_digit = i;
		for (int j = 0; j < field_size; ++j)
		{
			line[i - 1].mas_of_figures[j] = bit_eject(i, field_size - 1, j);
			if(line[i - 1].mas_of_figures[j] == 1)
				temp_int++;
			// printf("%d ", line[i - 1].mas_of_figures[j]);
		}
		line[i - 1].number_of_figures = temp_int;
	}

	for (int i = 0; i < Num_lines - 2; ++i)
	{
		for (int j = 0; j < Num_lines - i - 2; ++j)
		{
			if(line[j].number_of_figures > line[j + 1].number_of_figures)
			{
				temp_int = line[j].number_of_figures;
				line[j].number_of_figures = line[1 + j].number_of_figures;
				line[j + 1].number_of_figures = temp_int;

				temp_int = line[j].line_digit;
				line[j].line_digit = line[1 + j].line_digit;
				line[j + 1].line_digit = temp_int;
				for (int k = 0; k < field_size; ++k)
				{
					temp_int = line[j].mas_of_figures[k];
					line[j].mas_of_figures[k] = line[1 + j].mas_of_figures[k];
					line[j + 1].mas_of_figures[k] = temp_int;
				}
			}
		}
	}

	devide_lines_to_threads();

	InitializeCriticalSection(&cs);
	threads = (HANDLE*)calloc(T, sizeof(HANDLE));
	for (int i = 0; i < T; i++)
		threads[i] = CreateThread(0, 0, thread_func, (void*)((char*)0 + i), 0, 0);

	float start = clock();
	for (int i = 0; i < T; i++)
		WaitForSingleObject(threads[i], INFINITE);
	float ms = ((clock() - start) / CLOCKS_PER_SEC) * 1000;

	FILE *time = fopen("time.txt", "w+");
	FILE *out  = fopen("output.txt", "w+");
	fprintf(time, "%0.lf", ms);
	fprintf(out, "%d", answer);

	printf("time = %0.lf ms\n", ms);
	printf("answer = %d\n", answer);

	for (int i = 0; i < T; i++)
		CloseHandle(threads[i]);
	DeleteCriticalSection(&cs);
	free(coordinates);
	free(threads);
	fclose(time);
	fclose(out);
	fclose(test_file);

	return 0;
}