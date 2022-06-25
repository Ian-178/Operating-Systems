// Эта инструкция обязательно должна быть первой, т.к. этот код компилируется в бинарный,
// и загрузчик передает управление по адресу первой инструкции бинарного образа ядра ОС.
extern "C" int kmain(); 
__declspec(naked) void startup()
{   
	__asm 
	{    
		call kmain;   
	}  
} 

#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)
// Селектор секции кода, установленный загрузчиком ОС
#define GDT_CS (0x8)
#define VIDEO_BUF_PTR (0xb8000)
unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
#define PIC1_PORT (0x20)
// Базовый порт управления курсором текстового экрана. Подходит для большинства, но может отличаться в других BIOS и в общем случае адрес должен быть прочитан из BIOS data area.
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80) // Ширина текстового экрана
#define INT_MIN -2147483648
#define INT_MAX +2147483647

int color_global = 0;

bool flag = 0;
int num_str = 0;
int pos_curs = 0;
char key[2];
char command[32];

char scancodes[] = 
{
0, 0, // ESC
'1','2','3','4','5','6','7','8','9','0', '-', '=',
8, // BACKSPACE
'\t', // TAB
'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', ' ', // ENTER
0, // CTRL
'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '<','>','+',
0, // LEFT SHIFT
'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
0, // RIGHT SHIFT
'*', // NUMPAD
0, // ALT
' ', // SPACE
0, // CAPSLOCK
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1 - F10
0, // NUMLOCK
0, // SCROLLLOCK
0, // HOME
0,
0, // PAGE UP
'-', // NUMPAD
0, 0,
0,
'+', // NUMPAD
0, // END
0,
0, // PAGE DOWN
0, // INS
0, // DEL
0, // SYS RQ
0,
0, 0, // F11 - F12
0,
0, 0, 0, // F13 - F15
0, 0, 0, 0, 0, 0, 0, 0, 0, // F16 - F24
0, 0, 0, 0, 0, 0, 0, 0
};
// Структура описывает данные об обработчике прерывания

#pragma pack(push, 1) // Выравнивание членов структуры запрещено
struct idt_entry
{
	unsigned short base_lo; // Младшие биты адреса обработчика
	unsigned short segm_sel; // Селектор сегмента кода
	unsigned char always0; // Этот байт всегда 0
	unsigned char flags; // Флаги тип. Флаги: P, DPL, Типы - это константы - IDT_TYPE...
	unsigned short base_hi; // Старшие биты адреса обработчика
};
// Структура, адрес которой передается как аргумент команды lidt
struct idt_ptr
{
	unsigned short limit;
	unsigned int base;
};
#pragma pack(pop)
struct idt_entry g_idt[256]; // Реальная таблица IDT
struct idt_ptr g_idtp; // Описатель таблицы для команды lidt
//Пустой обработчик прерываний. Другие обработчики могут быть реализованы по этому шаблону
__declspec(naked) void default_intr_handler() 
{
	__asm 
		{
			pusha
		}
	// ... (реализация обработки)
	__asm 
	{
		popa
		iretd
	}
}

typedef void (*intr_handler)();

void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr)
{
unsigned int hndlr_addr = (unsigned int) hndlr;
g_idt[num].base_lo = (unsigned short) (hndlr_addr & 0xFFFF);
g_idt[num].segm_sel = segm_sel;
g_idt[num].always0 = 0;
g_idt[num].flags = flags;
g_idt[num].base_hi = (unsigned short) (hndlr_addr >> 16);
}

// Функция инициализации системы прерываний: заполнение массива с адресами обработчиков
void intr_init()
{
int i;
int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
for(i = 0; i < idt_count; i++)
intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,
default_intr_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
}

void intr_start()
{
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);

	g_idtp.base = (unsigned int) (&g_idt[0]);
	g_idtp.limit = (sizeof (struct idt_entry) * idt_count) - 1;
	__asm {
		lidt g_idtp
	}
	//__lidt(&g_idtp);
}
void intr_enable()  
{
	__asm sti;
}
void intr_disable()
{
	__asm cli;
}

__inline unsigned char inb (unsigned short port)
{
	unsigned char data;
	__asm 
	{
		push dx
		mov dx, port
		in al, dx
		mov data, al
		pop dx
	}
	return data;
}


//Запись
__inline void outb (unsigned short port, unsigned char data)
{
	__asm 
	{
		push dx
		mov dx, port
		mov al, data
		out dx, al
		pop dx
	}
}

void outw(unsigned short port, unsigned short data) 
{ 
	__asm 
	{ 
		push dx 
		mov dx, port 
		mov ax, data 
		out dx, ax 
		pop dx 
	} 
} 

// Функция переводит курсор на строку strnum (0 – самая верхняя) в позицию pos на этой строке (0 – самое левое положение).
void cursor_moveto(unsigned int strnum, unsigned int pos)
{
	unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
	outb(CURSOR_PORT, 0x0F);
	outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));
	outb(CURSOR_PORT, 0x0E);
	outb(CURSOR_PORT + 1, (unsigned char)( (new_pos >> 8) & 0xFF));
}

void out_str(int color, const char* ptr)
{
	unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
	video_buf += 80*2 * num_str;
	while (*ptr)
	{
		video_buf[0] = (unsigned char) *ptr;
		video_buf[1] = color;
		video_buf += 2;
		ptr++;
	}
	num_str++;
}

void out_symb(int color, char* ptr, unsigned int posnum)
{
	unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
	video_buf += 80*2 * num_str;
	video_buf += posnum*2;
		video_buf[0] = (unsigned char) *ptr;
		video_buf[1] = color;
		video_buf += 2;
}

void out_int_symb(int color, int digit, unsigned int posnum)
{
	unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
	video_buf += 80*2 * num_str;
	video_buf += posnum * 2;
		video_buf[0] = digit;
		video_buf[1] = color;
		video_buf += 2;
}

void step(void)
{	
	out_str(color_global, "$ Enter the command:");
	pos_curs = 0;
	cursor_moveto(num_str, pos_curs);	
}


void clear_screen() 
{ 
	unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR; 
	for (int i = 0; i < 2000; i++) 
	{ 
		video_buf[0] = (unsigned char) '\0'; 
		video_buf += 2; 
	} 
	cursor_moveto(0, 0);
}

void error(void)
{
	out_str(color_global, "$ Sorry you wrote unknown command, see list of command: help");
	step();
}

int cmp(char* a, char* b, bool parametrs)
{
	int flag = 0;
	while(*a && *b && (*a == *b))
    {
        a++; b++;
    }
    if(parametrs == true)
    {
	    while(*a)
	    {
	    	a++;
	    	flag++;
	    }
	    if(flag == 0) return -2; // нет параметров
	}
    if (*a > *b) return 1;
    if (*a < *b) return -1;
    
    return 0;
}

size_t strlen(const char *str)
{
    const char *s;
 
    for (s = str; *s; ++s);
	return (s - str);
}

void end_of_screen(void)
{
	if(num_str >= 19)
	{
		clear_screen();
	}
}

void shutdown(void) 
{ 
	outw(0x604, 0x2000); 
}

void null_mass(char *str, int len)
{
	for (int i = 0; i < len; ++i)
	{
		str[i] = 0;
	}
	return;
}

int pow(int x, int y)
{
    int i, rez = 1;
    
	for(i = 0; i < y; i++)
    {
    	rez *= x;
	}
        
return rez;  
}

void null_int_mass(int *mass, int len)
{
	for (int i = 0; i < len; ++i)
	{
		mass[i] = 0;
	}
	return;
}

char find_error_mult_div(char *str, int len)
{
	if(str[len - 1] == '*' || str[len - 1] == '+' || str[len - 1] == '/' || str[len - 1] == '-')
		return -1;

	if(str[0] == '*' || str[0] == '/')
		return -1;

	for (int i = 1; i < len; ++i)
	{
		if(str[i - 1] == '*' && str[i] == '*' || str[i - 1] == '/' && str[i] == '/' || str[i] == '0' && str[i - 1] == '/')
			return -1;

		if(str[i - 1] == '*' && str[i] == '/' || str[i - 1] == '/' && str[i] == '*')
			return -1;

		if(str[i - 1] == '+' && str[i] == '*' || str[i - 1] == '+' && str[i] == '/')
			return -1;

		if(str[i - 1] == '-' && str[i] == '*' || str[i - 1] == '-' && str[i] == '/')
			return -1;

		if(str[i - 1] == '*' && str[i] == '+' || str[i - 1] == '/' && str[i] == '-')
			return -1;

		if(str[i - 1] == '*' && str[i] == '-' || str[i - 1] == '/' && str[i] == '+')
			return -1;
	}

	
	return 0;
}

char overflow(char *str, int len)
{
	bool flag = false; 

	if(len < 10)
		return 0;

	if(str[0] <= 50)
		;
	else
		flag = true;
	if(str[1] <= 49)
		;
	else
		flag = true;
	if(str[2] <= 52)
		;
	else
		flag = true;
	if(str[3] <= 55)
		;
	else
		flag = true;
	if(str[4] <= 52)
		;
	else
		flag = true;
	if(str[5] <= 56)
		;
	else
		flag = true;
	if(str[6] <= 51)
		;
	else
		flag = true;
	if(str[7] <= 54)
		;
	else
		flag = true;
	if(str[8] <= 52)
		;
	else
		flag = true;
	if(str[9] <= 56)
		;
	else
		flag = true;


	if(flag == true)
		return 111;
	else
		return 0;
}

char incorrect_input(char *str, int len)
{
	for (int i = 0; i < len; ++i)
	{
		if((str[i] > 46 && str[i] < 58) || str[i] == 42 || str[i] == 43 || str[i] == 45);
			else
			{
			return -1;
			}
	}
	return 0;
}

char* delete_extra(char *str, int len)
{
	char str_copy[32] = {0};
	int number = 0;
	*str_copy = *str;
	number++;
	bool flag = false;

	for (int i = 1; i < len; ++i)
	{
		if(str[i - 1] == '+' && str[i] == '+')
		{
			flag = true;
			continue;
		}
		if(str[i - 1] == '-' && str[i] == '-')
		{
			flag = true;
			continue;
		}
		str_copy[number] = str[i];
		//out_str(color_global, str_copy);
		number++;
	}
	null_mass(str, 32);

	for (int i = 0; i < number; ++i)
	{
		str[i] = str_copy[i];
	}

	return 0;
}

int digit_to_integer(char *str, int len)
{
	int integer = 0;
	int num_pos = 0;
	if(overflow(str, len) == 111)
	{
		out_str(color_global, "stack overflow");
		return -1;
	}

	for (int i = 0; i < len; ++i)
	{
		// out_symb(color_global, &str[i], 0);
		// num_str++;
		integer += 	pow(10, len - i - 1)*(str[i] - 48);
	}

	return integer;
}

void integer_to_digit(int integer, char *temp_char)
{
	int temp = integer;
	char t;
	int count = 0;

	while(temp = temp/10)
	{
		count++;
	}
	count++;
	null_mass(temp_char, count);


	for (int i = count - 1; i >= 0; i--)
	{
		temp = integer % 10;
		integer /= 10;
		temp_char[i] = temp | 0x30;
	}

	return;
}

void slide(int *mass, int result, int place, int size)
{
	mass[place] = result;
	for (int i = place + 1; i < size - 2; ++i)
	{
		mass[i] = mass[i + 2];
	}
	return;
}

int func_mul_ovf(int si_a, int si_b) 
{
  int result; 
  if (si_a > 0) 
  {
    if (si_b > 0) 
    { 
      if (si_a > (INT_MAX / si_b)) 
      {
      	result = -1;
      	return result;
      }
    }else 
    {
      if (si_b < (INT_MIN / si_a)) 
      {
      	result = -1;
      	return result;
      }
    }
  }else 
  {
    if (si_b > 0) 
    {
      if (si_a < (INT_MIN / si_b)) 
      {
      	result = -1;
      	return result;
      }
    }else 
    {
      if ( (si_a != 0) && (si_b < (INT_MAX / si_a))) 
      {
      	result = -1;
      	return result;
      }
    }
  }

  result = si_a * si_b;
  return result;
}

//correct string
int calculate(char *str, int len)
{
	char operand[16] = {0};
	int mass_int[32];
	int count = 0, result = 0;
	int sup_count = 0;

	null_mass(operand, 16);
	null_int_mass(mass_int, 32);

	for (int i = 0, j = 0; i < len; ++i)
	{
		if(str[i] == '/' || str[i] == '*' || str[i] == '+' || str[i] == '-')
		{
			mass_int[j] = digit_to_integer(operand, sup_count);
			if(mass_int[j] == -1)
				return -1;
			null_mass(operand, 16);
			sup_count = 0;
			j++;
			mass_int[j] = str[i];
			j++;
			count = j;
			continue;
		}

		operand[sup_count] = str[i];
		sup_count++;

		if(i == len - 1)
		{
			mass_int[j] = digit_to_integer(operand, sup_count);
			if(mass_int[j] == -1)
				return -1;
			j++;
			count = j;
			null_mass(operand, 16);
			sup_count = 0;
		}
	}

	for (int i = 0; i < count && count != 1; ++i)
	{
		if(count == 1)
			break;

		if(mass_int[i] == '/')
		{
			result = mass_int[i - 1]/mass_int[i + 1];
			if(result < 0)
			{
				out_str(color_global, "here overflow");
				return -1;
			}
			slide(mass_int, result, (i - 1), count);
			i = 0;
			count = count - 2;
		}

		if(mass_int[i] == '*')
		{
			result = func_mul_ovf(mass_int[i - 1], mass_int[i + 1]);
			if(result < 0 || result < mass_int[i - 1] || result < mass_int[i + 1] || result > 214)
			{
				out_str(color_global, "here overflow");
				return -1;
			}
			slide(mass_int, result, (i - 1), count);
			i = 0;
			count = count - 2;
		}
	}

	for (int i = 0; i < count || count != 1; ++i)
	{
		if(mass_int[i] == '+')
		{
			result = mass_int[i - 1] + mass_int[i + 1];
			if(result < 0)
			{
				out_str(color_global, "here overflow");
				return -1;
			}
			slide(mass_int, result, (i - 1), count);
			i = 0;
			count = count - 2;
		}
		if(mass_int[i] == '-')
		{
			result = mass_int[i - 1] - mass_int[i + 1];
			if(result < 0)
			{
				out_str(color_global, "here overflow");
				return -1;
			}
			slide(mass_int, result, (i - 1), count);
			i = 0;
			count = count - 2;
		}
	}

	integer_to_digit(mass_int[0], operand);
	out_str(color_global, operand);
	num_str++;

	return 0;
}

void processing(char *str)
{
	int str_len = strlen(str);
	char expr[32];

	for (int i = 5; i < str_len; ++i)
	{
		expr[i - 5] = str[i];
	}

	if(expr == 0 || str_len == 1)
	{
		out_str(color_global, "expression is empty!");
		return;
	}

	str_len = str_len - 5;

	if(incorrect_input(expr, str_len) != 0)
	{
		out_str(color_global, "$ Error: input is incorrect");
		num_str++;
		return;
	}

	if(find_error_mult_div(expr, str_len) != 0)// || overflow(expr, str_len) != 0)
	{
		out_str(color_global, "$ Error: expression is incorrect");
		num_str++;
		return;
	}

	if(str_len == 0)
	{
		out_str(color_global, "$ Somethig is wrong");
		num_str++;
		return;
	}
	if(calculate(expr, str_len) == -1)
		out_str(color_global, "calculate fault");
	return;
}

void com_handler(void)
{
	end_of_screen();
	if(cmp(command, "info", false) == 0)
	{
		num_str++;
		out_str(color_global, "Calc OS.");
		out_str(color_global, "Developer: Ivnenko Ivan, SpbPU, 2019");
		out_str(color_global, "Compilers: bootloader: fasm, kernel: gcc");
		step();
	}
	
	else if(cmp(command, "help", false) == 0)
	{
		num_str++;
		out_str(color_global, "info  - information about developer and OS");
		out_str(color_global, "expr * - to calculate * expression");
		out_str(color_global, "shutdown - turn off the computer");
		step();
	}

	else if(cmp(command, "shutdown", false) == 0)
	{
		shutdown();
	}

	else if(command[0] == 'e' && command[1] == 'x' && command[2] == 'p' && command[3] == 'r')
	{
		num_str++;
		processing(command);
		step();
	}
	else
	{
		num_str++;
		error();
	} 

	*command = 0;
}


void on_key(unsigned char scan_code)
{
	key[0] = scancodes[scan_code];
	if(scan_code != 28)// не ENTER
	{
		if(key[0] != 8)
		{
			if(pos_curs != 40)
			{	
				if(scan_code == 75 && pos_curs!=0)
				{
					pos_curs--;
					cursor_moveto(num_str, pos_curs);
				}
				else if(scan_code == 77 && pos_curs!=40)
				{
					pos_curs++;
					cursor_moveto(num_str, pos_curs);
				}
				else
				{
					if(key[0] != 0)
					{
						out_int_symb(color_global, (int)*(key + 0), pos_curs);
						command[pos_curs] = key[0];
						pos_curs++;
						cursor_moveto(num_str, pos_curs);
						command[pos_curs]='\0';
					}
				}
			}
		}
		else
		{
			if(pos_curs != 0)
			{
				pos_curs--;
				out_int_symb(color_global, (int)*" ", pos_curs);
				command[pos_curs]='\0';
				cursor_moveto(num_str, pos_curs);
			}
		}
	}
	else
	{
		com_handler();
	}
}

void keyb_process_keys()
{
	// Проверка что буфер PS/2 клавиатуры не пуст (младший бит присутствует)
	if (inb(0x64) & 0x01)
	{
		unsigned char scan_code;
		unsigned char state;
		scan_code = inb(0x60); // Считывание символа с PS/2 клавиатуры
		if (scan_code < 128) // Скан-коды выше 128 - это отпускание клавиши
			on_key(scan_code);
	}
}

__declspec(naked) void keyb_handler()
{
	__asm pusha;
	// Обработка поступивших данных
	keyb_process_keys();
	// Отправка контроллеру 8259 нотификации о том, что прерывание обработано.
	// Если не отправлять нотификацию, то контроллер не будет посылать новых сигналов 
	// о прерываниях до тех пор, пока ему не сообщать что прерывание обработано. 
	outb(PIC1_PORT, 0x20);
	__asm 
	{
		popa
		iretd
	}
}


void keyb_init()
{
	// Регистрация обработчика прерывания
	intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
	// Разрешение только прерываний клавиатуры от контроллера 8259
	outb(PIC1_PORT + 1, 0xFF ^ 0x02); // 0xFF - все прерывания, 0x02 - бит IRQ1 (клавиатура).
	// Разрешены будут только прерывания, чьи биты установлены в 0
}

void mode(char* mode)
{


	return;
}

extern "C" int kmain()
{
	switch((char)video_buf[0])
	{
		case '1':
			color_global = 2;
			break;
		case '2':
			color_global = 1;
			break;
		case '3':
			color_global = 4;
			break;
		case '4':
			color_global = 14;
			break;
		case '5':
			color_global = 8;
			break;
		case '6':
			color_global = 7;
			break;
		default:
			out_str(color_global, "Bad color!\nGoodbye");
			shutdown();
	}
	//clear_screen();

	out_str(color_global, "Welcome to CalcOS!");
	num_str++;
	out_str(color_global, "To see description of all commands, print \"help\"");
	num_str++;

	intr_disable();
	intr_init();
	keyb_init();
	intr_start();
	intr_enable();

	step();

	while(1)
	{
		__asm
		{
			hlt;
		}
	}
	return 0;
}