////////////////////////////////////////////////////////////////////////
////															   	////
////			  Автор: Стадник Володимир Сергійович   			////
////					  Дата: 03.08.2017							////
////					Проект: TachMetr V2.0						////
////		     Посилання: http://shaitech.pp.ua/?p=11             ////
////																////	
////////////////////////////////////////////////////////////////////////

#define F_CPU 12000000UL

//Піни на яких знаходиться шина даних
#define DATA 0
#define CLK 1
#define LOAD 2	

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

unsigned long long oldRPM=0, RPM=0; //Оберти на хвилину
unsigned long counter=0; //лічильник імпульсів
char rewrite=0, t; //Прапорець "Перемалювати дисплей"

ISR(TIMER1_OVF_vect) {
	oldRPM=RPM; 
	RPM=(counter*155)/100; //Коефіціент 1.55
	counter=0;
	rewrite=1; //Ноновити дані дисплею
	t++;
}

ISR(INT0_vect) {
	counter++;
}

//Надсилання одного байту до мікросхеми
void MAX7219_send_byte(unsigned char data, char direct) {
	char i;
	for (i=0; i<8; i++) {
		//В залежності від направленності бітів, виводимо біт на ногу "DATA"
		if (!direct) PORTC=((data)&0x80)>>(7-DATA); 
		  else PORTC=((data)&0x01)<<(DATA);
		asm("nop"); //Тріхо чекаємо
		PORTC=1<<CLK; //Даємо такт на CLK
		asm("nop");
		PORTC&=~(1<<CLK);
		asm("nop");
		//В залежності від направленності бітів - рухаємо біти в потрібну сторону
		if (!direct) data=data<<1;
		  else data=data>>1;
	}
}

//Надсилання команди до мікросхеми
void MAX7219_send_command(unsigned char adres, unsigned char data, char direct) {
	PORTC&=~(1<<LOAD); //Вмикаємо режим завантаження
	MAX7219_send_byte(adres, 0); //Записуємо адресу в яку будемо писати дані
	MAX7219_send_byte(data, direct); //Передаємо дані
	PORTC|=(1<<LOAD); //Дані передано
}

//Перемалювання дисплею
void MAX7219_repaint(unsigned long long RPM) {
	unsigned long long display;
	unsigned char digit[8];
	char i, tmp;
	display=0x00FFFFFFFFFFFF>>(47-RPM);
	//display=RPM;
	for (i=0; i<6; i++) {
		digit[i]=display>>(i*8);
		
		//Милиці, які можна було прибрати під час проектування плати
		tmp=(digit[i]&0x80)>>7;
		digit[i]<<=1;
		digit[i]|=tmp;
	}
	if (t>12) {
		digit[6]=(RPM/10) | 0x80;
		digit[7]=RPM%10;
		t=0;
	}
	
	for (i=0; i<8; i++) 
		if (i<6) MAX7219_send_command(i+1, digit[i], 1);
		else MAX7219_send_command(i+1, digit[i], 0);
}

int main(void) {
	//Вимикаємо непотріні порти
	DDRB=0x00;
	DDRD=0x00;
	//Вмикаємо тільки необхідні три ноги на обмін даними із MAX7219
	DDRC=0x07;
	
	//Ініціалізація дисплею
	PORTC|=(1<<LOAD);
	MAX7219_send_command(0x0F, 0x00, 0); //Режим тесту - відключений
	MAX7219_send_command(0x0C, 0x01, 0); //Увікнути робоий режим
	MAX7219_send_command(0x0B, 0x07, 0); //Вмикаємо все цифри
	MAX7219_send_command(0x09, 0xC0, 0); //Режим декодування вмикаємо для DIG6 і DIG7
	MAX7219_send_command(0x0A, 0X0A, 0); //Яскравість, яка нам більше подобається
		
	//Налаштування переривань
	//Переривання INT0
	GICR|=(1<<INT0);
	MCUCR|=(1<<ISC01)|(1<<ISC00); //По передньому фронту
	
	//Переривання по таймеру
	TCCR1B=(1<<CS11);
	TIMSK=(1<<TOIE1);
	
	//Анімація включення
	char i;
	for (i=0; i<47; i++) {
		MAX7219_repaint(i);
		_delay_ms(10);
	}
	for (i=47; i>0; i--) {
		MAX7219_repaint(i);
		_delay_ms(50);
	}
	
	sei();		
	while (1) {
		if (rewrite) {
			MAX7219_repaint((oldRPM+RPM)/2); //Малюємо на дисплеї середнє з двох попередніх значень
			rewrite=0;
			sei();
		}
		asm("nop");
	}
}
