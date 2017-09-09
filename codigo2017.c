#include <16f877A.h>
#DEVICE 	ADC=10
#fuses    	xt,nowdt,noprotect,put,brownout,nolvp,nocpd,nowrt// configuração dos fusíveis
#use      	delay(clock=4000000, RESTART_WDT)

// Configurando Porta Serial
#use rs232(baud=19200, xmit=pin_c6, rcv=pin_c7, BRGH1OK)

#use   fast_io(a)
#use   fast_io(b)
#use   fast_io(c)
#use   fast_io(d)
#use   fast_io(e)

#byte porte =   0x09

#bit rs = porte.0        // via do Lcd que sinaliza recepção de dados ou comando
#bit enable = porte.1    // enable do Lcd

int buffer[6];               // Recebe o envio via RS - 232
int DUTY,i,temp1;
int16 SAIDA;
char UNIDADE[4];

void beep(int cont)
{
	for (int i=0; i<cont; ++i)
	{
		delay_ms(200);
		for(int j=0; j<100; ++j)
		{
			output_high(pin_a5);
			delay_ms(1);
			output_low(pin_a5);
			delay_us(500);
		}
	}
}	
void comando_lcd(int caracter)
{
	rs = 0;                              // seleciona o envio de um comando
	output_d(caracter);                  // carrega o portd com o caracter
	enable = 1 ;                         // gera pulso no enable
	delay_us(1);                         // espera l microsegundo
	enable = 0;                          // desce o pino de enable
	delay_us(40);                        // espera minimo 40 microsegundos
	return;                              // retorna 
}

void escreve_lcd(int caracter)
{
	rs = 1;                            // seleciona o envio de um DADO 
	output_d(caracter);                // carrega o portd com o caracter 
	enable =1;                         // gera pulso no enable
	delay_us(1);                       // espera l microsegundo
	enable = 0;                        // desce o pino de enable
	delay_us(40);                      // espera minimo 40 microsegundos
	return;                            // retorna 
}

void limpa_lcd()
{
	comando_lcd(0x01);
	delay_ms(2);
	return; 
}

void inicializa_lcd()
{
	comando_lcd(0x30);              // envia comando para inicializar display
	delay_ms(4);                    // espera 4 milisengundos
	comando_lcd(0x30);              // envia comando para inicializar display
	delay_us(100);                  // espera 100 microsengundos
	comando_lcd(0x30);              // envia comando para inicializar display
	comando_lcd(0x38);              // liga o display, sem cursor e sem blink
	limpa_lcd() ;                   // limpa Lcd
	comando_lcd(0x0c);              // display sem cursor
	comando_lcd(0x06);              // desloca cursor para a direita 
	return;                         // retorna
}


void tela_principal()
{
	//Atualizar informações no lcd
	comando_lcd(0xC0);                   			// posiciona o cursor na linha 1, coluna 0
	printf (escreve_lcd,"%4Lu%3s ",SAIDA,UNIDADE); 	// imprime mensagem no Lcd
	comando_lcd(0xC8);                   			// posiciona o cursor na linha 1, coluna 8
	printf (escreve_lcd,"PWM %3d%%",DUTY);          // imprime mensagem no Lcd
	return;                              			// retorna da função
}

void leitura_velocidade();
void leitura_temperatura();
void pwm_ventilador()
{
	int16 Duty2;
	DUTY = buffer[1];
	Duty2 = DUTY*10.0;
	set_pwm2_duty((int16)Duty2);
}


void pwm_resistor()
{
	int16 Duty2;
	DUTY = buffer[1];
	Duty2 = DUTY*10.0;
	set_pwm1_duty((int16)Duty2);
}

void leitura_temperatura()
{
	int16 conv = 0;
	// Inicia conversão AD
	// Faz regra de 3 para converter o valor para mili volts
	conv = (int16)(read_adc()*4.888);

	//Mandar para o MATLAB
	putc((int8)(conv>>8));
	putc((int8)(conv));

	SAIDA = conv;
	UNIDADE[0] = ' ';
	UNIDADE[1] = 'm';
	UNIDADE[2] = 'V';
}

void leitura_velocidade()
{
	int16 cont = 0;
	//Rotações por Segundo
	cont = get_timer1();
	set_timer1(0);

	//Mandar para o MATLAB
	putc((int8)(cont>>8));
	putc((int8)(cont));

	SAIDA = cont;
	UNIDADE[0] = 'P';
	UNIDADE[1] = 'T';
	UNIDADE[2] = 'a';
} 

void parar_ventilador()
{
	set_pwm2_duty(0);
	DUTY = 0;
	SAIDA = 0;
	
	//Escrever "   *PWM ZERO*   "
	comando_lcd(0xC0);                   // posiciona o cursor na linha 1, coluna 0
	printf (escreve_lcd,"   *PWM ZERO*   "); // imprime mensagem no Lcd
	temp1 = 60;
	beep(3);
	set_timer1(0);
}

void parar_calor()
{
	set_pwm1_duty(0);
	DUTY = 0;
	SAIDA = 0;
	
	//Escrever "   *PWM ZERO*   "
	comando_lcd(0xC0);                   // posiciona o cursor na linha 1, coluna 0
	printf (escreve_lcd,"   *PWM ZERO*   "); // imprime mensagem no Lcd
	temp1 = 60;
}	
void alterar()
{ 
	switch(buffer[0])
	{
		case '0':	pwm_resistor();	leitura_temperatura();				break;
		case '1':	pwm_ventilador();	leitura_velocidade();			break;
		case '2':	parar_ventilador();									break;
		case '3':	parar_calor(); 										break; 
		case '4':	pwm_resistor();										break;
		case '5':	pwm_ventilador();									break;
		case '6':	leitura_temperatura();								break;
		case '7':	leitura_velocidade(); 								break;
	}
}
void buffer_serial()
{

	if(kbhit())
	{
		buffer[i]=getc();
		if(buffer[i]=='\n')
		{
			i=0;
			alterar();
		}
		else
		{
			i++;
		}
	}
}

void config()
{
	//ponteiro do buffer
	i = 0;
	//Configurar conversor AD - ler temperatura
	setup_adc_ports(RA0_RA1_RA3_analog); 
	setup_adc (adc_clock_div_32); 
	//Canal 0 - temperatura
	set_adc_channel(0);
	
	setup_timer_0(rtcc_internal | rtcc_div_256);
	
	//Tacômetro no pino RC0 - entrada do TMR1
	setup_timer_1(t1_external_sync|t1_div_by_1);
	//Conversores AD e Buzzer (RA5)
	set_tris_a(0b11011111);
	//Botões (desativado) e ativadores de Displays de 7 segmentos (ativado)
	set_tris_b(0b00001111);
	
	//Tacômetro no pino RC0 - entrada do TMR1
	//PWM para variar velocidade do ventilador - RC1
	//PWM para gerar calor - RC2
	set_tris_c(0b11111001);
	
	//Saída para LCD e Displays de 7 segmentos
	set_tris_d(0b00000000);
	
	//E0 e E1 - Enable e Rs do LCD
	set_tris_e(0b100);
	
	//Desativar Displays
	output_b(0b00000000);
	//Configurar a frequência dos PWMs (1 kHz)
	setup_timer_2(T2_DIV_BY_4,249,1);
	
	//Pino RC1 como PWM
	setup_ccp2(ccp_pwm);
	//Pino RC2 como PWM
	setup_ccp1(ccp_pwm);
	
	//Pwm zero inicialmente
	DUTY = 0;
	SAIDA = 0;
	set_pwm1_duty(0);
	set_pwm2_duty(0);
	
	set_timer1(0);
  	//Ativar LCD
	inicializa_lcd();

	UNIDADE[0] = 'R';
	UNIDADE[1] = 'd';
	UNIDADE[2] = 'y';
	
 	comando_lcd(0x80);							// posiciona o cursor na linha 0, coluna 0
	printf(escreve_lcd,"LAB.CONT.DIGITAL");		// imprime mensagem no Lcd
	
	
	comando_lcd(0xC0);                   		// posiciona o cursor na linha 1, coluna 0
	printf (escreve_lcd,"Seja Bem-Vindo!");		// imprime mensagem no Lcd
	//4 Segundos
	temp1 = 60;
	
	//Habilitar todas as interrupções
	enable_interrupts(GLOBAL| INT_RTCC);	
	
}	

void main()
{
	config();
	while(TRUE)
	{
		buffer_serial();
	}
}
/**********************************************************
*    Rotina de tratamento de interrupcao de TMR0          *
***********************************************************/

//atualizar informações do lcd
#int_rtcc
void trata_int_tmr0(void)
{
	if(temp1==0)               //ja passou um segundo
	{                       //Sim,
		tela_principal();
		temp1 = 15;
	}
	else                       // Nao,
	{
		temp1--;                // Decrementa temp1
	}

}