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



void null_mass(char *str, int len)
{
	for (int i = 0; i < len; ++i)
	{
		str[i] = 0;
	}
	return;
}