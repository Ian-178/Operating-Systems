Необходимо решить **классическую задачу о поиске всех возможных вариантов расстановки на доске шахматных фигур**. Дана квадратная шахматная доска размером **NxN**. На доске уже размещено **K** фигур. Фигуры размещены так, что находятся не под боем друг друга. Необходимо расставить на доске еще **L** фигур так, чтобы никакая из фигур на доске не находилась под боем любой другой фигуры. Необходимо найти все возможные решения.\
\
Входные данные: файл *input.txt*.
На первой строке файла записано единственное число — количество потоков для решения.\
На второй строке файла записаны три числа: N L K (через пробел).\
Далее следует K строк, содержащих числа x и y (через пробел) - координаты уже стоящей на доске фигуры. Координаты отсчитываются от 0 до N-1.\
\
Выходные данные: файл *output.txt*. Содержит ответ - количество уникальных решений задачи.\
Выходные данные: файл *time.txt*. Содержит время работы алгоритма в миллисекундах, без учета времени считывания входных данных и времени на создание рабочих потоков.\
\
\
\
It is necessary to solve **the classical problem of finding all possible options for placing chess pieces on the board **. A square chessboard of size **NxN** is given. There are already **K** pieces placed on the board. The pieces are placed so that they are not under each other's fight. It is necessary to place more **L** pieces on the board so that none of the pieces on the board is under the fight of any other figure. It is necessary to find all possible solutions.\
\
Input data: file *input.txt*.
The first line of the file contains a single number — the number of threads to solve.\
Three numbers are written on the second line of the file: N L K (separated by a space).\
Followed by K lines containing the numbers x and y (separated by a space) - the coordinates of the figure already standing on the board. Coordinates are counted from 0 to N-1.\
\
Output data: file *output.txt*. Contains the answer - the number of unique solutions to the problem.\
Output data: file *time.txt*. Contains the running time of the algorithm in milliseconds, without taking into account the time of reading the input data and the time to create work threads.
