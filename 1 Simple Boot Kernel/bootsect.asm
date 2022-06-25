use16
org 0x7C00

start:
	mov ax, cs
	mov ds, ax
	mov ss, ax
	mov sp, start
	mov edi, 0xb8000

kernel:
mov ax, 0x1100
mov es, ax
mov bx, 0x0000
mov ah, 0x02
mov dl, 1
mov dh, 0
mov ch, 0

mov al, 12
mov cl, 2

int 0x13

mov bx, loading_str
call clear_screen
call puts
jmp steady


clear_screen:
	mov ax, 03
	int 10h
	ret


puts:
	mov al, [bx]
	test al, al
	jz end_puts
	mov ah, 0x0e
	int 0x10
	add bx, 1
	jmp puts
end_puts:
	ret

steady:
	mov ah, 0x00
	int 0x16
	mov [bx], al
	call clear_screen
	mov ah, 0x0e
	mov al, [bx]
	int 0x10
jmp _data


_data:
mov ax, 0x1300
mov es, ax
mov bx, 0x0000
mov ah, 0x02
mov dl, 1
mov dh, 0
mov ch, 0

mov al, 6
mov cl, 14

int 0x13

; Отключение прерываний
cli
lgdt[gdt_info] ; Загрузка размера и адреса таблицы дескрипторов lgdt gdt_info
    
; Включение адресной линии А20
in al, 0x92
or al, 2
out 0x92, al

; Установка бита PE регистра CR0 - процессор перейдет в защищенный режим
mov eax, cr0
or al, 1
mov cr0, eax
jmp 0x8:protected_mode 	; "Дальний" переход для загрузки корректной информации в cs, 
						;архитектурные особенности не позволяют этого сделать напрямую).

gdt:
db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00 
db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_info:
dw gdt_info - gdt
dw gdt, 0



number:
	db "123456", 0

loading_str:
	db "Press: 1 - green, 2 - blue, 3 - red, 4 - yellow, 5 - gray, 6 - white", 0



; Внимание! Сектор будет считаться загрузочным, если содержит в конце своих 512 байтов два следующих байта: 0x55 и 0xAA



use32
protected_mode:
mov ax, 0x10
mov es, ax
mov ds, ax
mov ss, ax
call 0x10000
times (512 - ($ - start) - 2) db 0 ; Заполнение нулями до границы 512 - 2 текущей точки
db 0x55, 0xAA ; 2 необходимых байта чтобы сектор считался загрузочным