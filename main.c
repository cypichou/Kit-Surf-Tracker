#include "stm32f1xx_hal.h"
#include "stm32f1_uart.h"
#include "stm32f1_sys.h"
#include "stm32f1_gpio.h"
#include "macro_types.h"
#include "systick.h"
#include "HC05/HC05.h"
#include "GPS/GPS.h"
#include <string.h>
#include "qfplib-m0-full.h"
#include "stm32f1_sd.h"
#include "stm32f1_spi.h"
#include "button.h"
#include "ff.h"
#include "MPU6050/stm32f1_mpu6050.h"

#define TAILLEMOY 10
#define PI     (3.141592653589793)

// =================================== Prototype =================================== \\

void process_ms(void);
char* concat (char* messageFinale, char* tempStr);
void decalage(int16_t *tab, int16_t element);
int16_t moyenne(int16_t * tab);
int anglefct (void);
float hauteur_saut (uint8_t vitesse, int16_t angle);
void state_machine(void);

// =================================== Gestion du temps =================================== \\

static volatile uint32_t t = 0;
static volatile uint32_t clignotement = 0;
uint16_t seconde = 0; 				// nombre de secondes a renvoyer a l'application

// =================================== Carte SD =================================== \\

char tab_vitesse[9];
char tab_temps[10];
uint8_t nb_read; 					// varieble qui renvoie le nombre de characeter ecrits dans la carte sd

// =================================== Valeurs =================================== \\

int vitesse = -1;
int16_t angle = 0;
uint8_t hauteur = 0;

// =================================== accelerometre =================================== \\

int16_t moyenneTabX[TAILLEMOY];     // tableau des coordonnes precedents en x,y,z de l'accelerometre pour calculer une moyenne qui elemine les valeurs absurdes
int16_t moyenneTabY[TAILLEMOY];
int16_t moyenneTabZ[TAILLEMOY];

MPU6050_t MPU6050_Data; 			// typedef pour les variables de l'accelerometre

uint8_t etat;

// =================================== GPS =================================== \\

gps_datas_t gps_datas; // typef avec les varaibles du gps

// =================================== bluetooth =================================== \\

uint8_t const taille_buffeur = 10;   // doit etre la meme que TAILLEMSG, il est juste envoyer dans une fonction
char tab_buff[10];

int main(void)
{

	HAL_Init();

	UART_init(UART2_ID,115200);
	UART_init(UART1_ID,9600);
	UART_init(UART3_ID,9600);
	SYS_set_std_usart(UART2_ID, UART2_ID, UART2_ID);

	BSP_GPIO_PinCfg(GPIOA,GPIO_PIN_15,GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_HIGH);

	MPU6050_Init(&MPU6050_Data, GPIOA, GPIO_PIN_0, MPU6050_Device_0, MPU6050_Accelerometer_8G, MPU6050_Gyroscope_2000s);

	Systick_add_callback_function(&process_ms);

	BSP_SD_Init();

	while(1)
	{
		state_machine();
	}
}

void state_machine(void)
{
	typedef enum
	{
		INIT,
		MODE_ATTENTE_ACTIVITE,
		MODE_ACTIVITE,
		MODE_ENVOIE_DONNEE
	}state_e;

	static state_e state = INIT;
	static state_e previous_state = INIT;                   //permet de sauvegarder l'état précédent
	bool_e entrance = (state!=previous_state)?TRUE:FALSE;	//ce booléen sera vrai seulement 1 fois après chaque changement d'état.
	previous_state = state;                                 //previous_state mémorise l'état actuel (qui est le futur état précédent)

	button_event_e button_event;
	button_event = BUTTON_state_machine();	               //à chaque passage ici, on scrute un éventuel évènement sur le bouton

	if(!entrance){
	switch(state)
	{
		case INIT:
			BUTTON_init();
			state =MODE_ATTENTE_ACTIVITE ;
			printf("INIT");
			break;

		case MODE_ATTENTE_ACTIVITE: // faire en sorte qu'on ne puisse pas y aller si on a pas lancer une premiere session
			if (button_event == BUTTON_EVENT_LONG_PRESS)
			{
				state = MODE_ENVOIE_DONNEE;
				printf("MODE_ENVOIE_DONNEE depuis MODE_ATTENTE_ACTIVITE");

			}
			if(button_event ==BUTTON_EVENT_SHORT_PRESS){
				state = MODE_ACTIVITE;
				HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);
				printf("MODE_ACTIVITE depuis MODE_ATTENTE_ACTIVITE");
			}

			if(clignotement % 2000 == 0){
				HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
			}
			if(clignotement % 500 == 0){
				HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);
			}
			break;

		case MODE_ACTIVITE:

			vitesse = GPS_VITESSE(&gps_datas);

			if(vitesse>=0){
				angle = anglefct();

				hauteur = hauteur_saut(vitesse, angle);

				sprintf(tab_vitesse, "%d/%d/",vitesse,hauteur);
				sd_machine_write(tab_vitesse);

				sprintf(tab_vitesse, "");
				t=0;
			}
			if(clignotement % 250 == 0){
				HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
			}
			if(clignotement % 500 == 0){
				HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);
			}


			if (button_event == BUTTON_EVENT_SHORT_PRESS)
			{
				state =  MODE_ATTENTE_ACTIVITE;
				printf("MODE_ATTENTE_ACTIVITE depuis mode mode active");
				HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
			}

		break;

		case MODE_ENVOIE_DONNEE:

			if (UART_getc(UART3_ID)=='1'){
				HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);
				printf("LES DATAS VONT ETRE ENVOYE  \n");

				while(!sd_machine_read(tab_buff,&taille_buffeur, &nb_read)){
							UART_puts(UART3_ID,tab_buff,taille_buffeur);
							sprintf(tab_buff, "");
						}

						for(int i = nb_read; i < taille_buffeur; i++){
							tab_buff[i] = '\0';
						}

						UART_puts(UART3_ID,tab_buff,taille_buffeur);
						sprintf(tab_temps, "t%d", seconde);
						UART_puts(UART3_ID,tab_temps,10);
						UART_putc(UART3_ID, '\n');

						formater_fct();

				HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
			}

			if (button_event == BUTTON_EVENT_LONG_PRESS)
			{
				state =MODE_ATTENTE_ACTIVITE;
				printf("MODE_ATTENTE_ACTIVITE depuis  MODE_ENVOIE_DONNEE");

			}
		break;

		default:
			break;
	}
	}
}

void decalage(int16_t *tab, int16_t element){
	tab[TAILLEMOY-1] = element;

	for(int i=TAILLEMOY-2; i>0;i--){
		tab[i-1] = tab[i];
	}
}

int16_t moyenne(int16_t * tab){
	int16_t moy;
	for(int i=0; i<TAILLEMOY;i++){
		  moy += tab[i];
	}
	return moy/TAILLEMOY;
}

void process_ms(void)
{
		t++;
		clignotement++;
		if(t == 1000){
			seconde++;
			t=0;
		}

}

char* concat (char *messageFinale, char *tempStr){ // le tableau de toutes les vitesses, la vitesse a rajouter

	uint16_t i,j;

     for (i = 0; messageFinale[i+1] != '\0'; i++);

     messageFinale[i] = '/';
     i++;

     for (j = 0; tempStr[j]!= '\0' && i<taille_buffeur ; ++j, ++i)
       {
         messageFinale[i] = tempStr[j];
       }

     messageFinale[i++] = '\n';
     messageFinale[++i]='\0';

     return messageFinale;

}

int anglefct (void){

				float angle = 0.0;

				MPU6050_ReadAccelerometer(&MPU6050_Data);

				decalage(moyenneTabX, MPU6050_Data.Accelerometer_X);
				decalage(moyenneTabY, MPU6050_Data.Accelerometer_Y);
				decalage(moyenneTabZ, MPU6050_Data.Accelerometer_Z);

				int16_t moyenneX_int = moyenne(moyenneTabX);
				int16_t moyenneY_int = moyenne(moyenneTabY);
				int16_t moyenneZ_int = moyenne(moyenneTabZ);

				float moyenne_XY = qfp_int2float(qfp_fsqrt(moyenneX_int * moyenneX_int + moyenneY_int * moyenneY_int));

				angle = qfp_fatan2(moyenneZ_int, moyenne_XY);

				angle = angle * 180 / PI;

				return qfp_float2int(angle);

}

float hauteur_saut (uint8_t vitesse, int16_t angle){

	float hauteur = 0;
	hauteur = vitesse * vitesse * qfp_dsin(angle) * qfp_dsin(angle);
	hauteur = hauteur / 19.6;
	return hauteur;

}

