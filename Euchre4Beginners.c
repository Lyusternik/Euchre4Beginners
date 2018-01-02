/*
************************************************************************
 ECE 362 - Mini Project - Fall 2017
***********************************************************************
Euchre4Beginners Team - John, Dan, and Will
	 	   			 		  			 		  		


***********************************************************************
*/

#include <hidef.h>       
#include "derivative.h"	 
#include <mc9s12c32.h>

/* All functions after main should be initialized here */
char playCard(char player, char hand[5], char inPlay[4], char trump, char suit);
char findHighest(char hand[5], char trump, char followSuit, char pHand);
char findLowest(char hand[5], char trump, char suit, char pHand);
char containsSuit(char hand[5], char suit, char trump, char pHand);
char checkSuit(char in, char trump); //Clubs, Diamonds, Hearts, Spades; 9-10-J-Q-K-A
char checkNum(char in);
char indexOf(char find, char array[5], char length);
char trickWin(char tricks[5], char inPlay[4], char trump, char suit);
char trumpSuccession(char card1, char card2, char trump);
void printHand(char hand[5], char trump, char suit);
void printBoard(char hand[5], char bid);
//void printCard(char in);
//void printHand(char* hand, char pHand);
char bid(char hand [4][5]);//,  char upcard);
void determineValidity(char hand[5], char trump, char suit);
char cardSelect(void);
void ledHelp(void);
//void printHand(char* hand);
void delaySet(char sec);

void shiftout(char);	// LCD drivers (written previously)
void lcdwait(void);
void send_byte(char);
void send_i(char);
void chgline(char);
void print_c(char);
void pmsglcd(char[]);

/* Variable declarations */
char validity[5]; //player card validity 0 = invalid, 1 = valid, 2 = best
char playHand;
char lcdPick; // 0 for both, 1 for LCD HAND, 2 for LCD BOARD
char bidholder; 
char upcard; //self-explanitory	  
char delayed;
int delayThresh;
int inc; 			 		  			 		       



/* LCD COMMUNICATION BIT MASKS (note - different than previous labs) */
#define RS 0x20		// RS pin mask (PTT[5])
#define RW 0x40		// R/W pin mask (PTT[6])
#define LCDCLK_HAND 0x01	// LCD Hand  EN/CLK pin mask (PTM[0])
#define LCDCLK_BOARD 0x02 // LCD Board EN/CLK pin mask (PTM[1])
 
/* LCD INSTRUCTION CHARACTERS */
#define LCDON 0x0F	// LCD initialization command
#define LCDCLR 0x01	// LCD clear display command
#define TWOLINE 0x38	// LCD 2-line enable command
#define CURMOV 0xFE	// LCD cursor move instruction
#define LINE1 0x80	// LCD line 1 cursor position
#define LINE2 0xC0	// LCD line 2 cursor position

/*	 	   		
***********************************************************************
Initializations
***********************************************************************
*/

void  initializations(void) {
  inc = 0;
  delayed = 0;
  delayThresh = 500;
  playHand = 0;
  lcdPick = 0;
/* Set the PLL speed (bus clock = 24 MHz) */
  CLKSEL = CLKSEL & 0x80; // disengage PLL from system
  PLLCTL = PLLCTL | 0x40; // turn on PLL
  SYNR = 0x02;            // set PLL multiplier
  REFDV = 0;              // set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; // engage PLL

/* Disable watchdog timer (COPCTL register) */
  COPCTL = 0x40   ; // COP off; RTI and COP stopped in BDM-mode

/* Initialize asynchronous serial port (SCI) for 9600 baud, interrupts off initially */
  SCIBDH =  0x00; //set baud rate to 9600
  SCIBDL =  0x9C; //24,000,000 / 16 / 156 = 9600 (approx)  
  SCICR1 =  0x00; //$9C = 156
  SCICR2 =  0x0C; //initialize SCI for program-driven operation
  DDRB   =  0x10; //set PB4 for output mode
  PORTB  =  0x10; //assert DTR pin on COM port
         
         	 	   			 		  			 		  		
  
/* Add additional port pin intializations here */

  DDRT = 0x7F;
  DDRAD = 0x00;
  DDRM = 0xFF;
/* 
   Initialize TIM Ch 7 (TC7) for periodic interrupts every 10.0 ms  
    - Enable timer subsystem                         
    - Set channel 7 for output compare
    - Set appropriate pre-scale factor and enable counter reset after OC7
    - Set up channel 7 to generate 10 ms interrupt rate
    - Initially disable TIM Ch 7 interrupts	 	   			 		  			 		  		
*/
  TSCR1 = 0x80; // enable timer subsystem
  TIOS = 0x80; // set channel 7
  TSCR2 = 0x0C; // set prescale
  TC7 = 7500; // 10 ms?
  TIE = 0x00; // intiaially disable
  
/*
 Initialize the PWM unit to produce a signal with the following
 characteristics on PWM output channel 3:
   - sampling frequency of approximately 100 Hz
   - left-aligned, negative polarity
   - period register = $FF (yielding a duty cycle range of 0% to 100%,
     for duty cycle register values of $00 to $FF 
   - duty register = $00 (motor initially stopped)
                         
 IMPORTANT: Need to set MODRR so that PWM Ch 3 is routed to port pin PT3
*/
  // need 5?
  MODRR = 0x1F;
  PWME = 0x1F;
  PWMPOL = 0x1F;
  PWMCAE = 0x00; // left alligned
  PWMCTL = 0x00; // 8 bit
  
  PWMPER0 = 0xFF; // max period
  PWMDTY0 = 0x00; // initially stopped
  PWMPER1 = 0xFF; // max period
  PWMDTY1 = 0x00; // initially stopped
  PWMPER2 = 0xFF; // max period
  PWMDTY2 = 0x00; // initially stopped
  PWMPER3 = 0xFF; // max period
  PWMDTY3 = 0x00; // initially stopped
  PWMPER4 = 0xFF; // max period
  PWMDTY4 = 0x00; // initially stopped
  
  PWMCLK = 0x08;
  PWMPRCLK = 0x07; // max prescaler
  PWMSCLA = 0x04; // scaled to 4?
  

/* 
 Initialize the ATD to sample a D.C. input voltage (range: 0 to 5V)
 on Channel 0 (connected to a 10K-ohm potentiometer). The ATD should
 be operated in a program-driven (i.e., non-interrupt driven), normal
 flag clear mode using nominal sample time/clock prescaler values,
 8-bit, unsigned, non-FIFO mode.
                         
 Note: Vrh (the ATD reference high voltage) is connected to 5 VDC and
       Vrl (the reference low voltage) is connected to GND on the 
       9S12C32 kit.  An input of 0v will produce output code $00,
       while an input of 5.00 volts will produce output code $FF
*/

  ATDCTL2 = 0x80;    // power up
  ATDCTL3 = 0x08;
  ATDCTL4 = 0x85;
  

/*
  Initialize the RTI for an 2.048 ms interrupt rate
*/

  CRGINT = 0x80;
  RTICTL = 0x1F;		     


    
  
/*
  Initialize SPI for baud rate of 6 Mbs, MSB first
  (note that R/S, R/W', and LCD clk are on different PTT pins)
*/
  SPIBR = 0x01;
  SPICR1 = 0x50;  // 
  SPICR2 = 0x00;



/* Initialize digital I/O port pins */
  ATDDIEN = 0x00; // set the reset pin to digitial input 
  //PTM = 0;
  PTT = 0;

/* 
   Initialize the LCD
     - pull LCDCLK high (idle)
     - pull R/W' low (write state)
     - turn on LCD (LCDON instruction)
     - enable two-line mode (TWOLINE instruction)
     - clear LCD (LCDCLR instruction)
     - wait for 2ms so that the LCD can wake up     
*/ 
  PTM |= LCDCLK_HAND;
  PTM |= LCDCLK_BOARD;
  PTT &= ~RW;
  lcdPick = 1;
  send_i(LCDON);
  send_i(TWOLINE);
  send_i(LCDCLR);
  lcdwait();
  
  lcdPick = 2;
  send_i(LCDON);
  send_i(TWOLINE);
  send_i(LCDCLR);
  lcdwait(); 
	 	   			 		  			 		  		

 
	      
}

/*	 		  			 		  		
***********************************************************************
Main
***********************************************************************
*/
void main(void) {
  char hold; //misc holding variable
  char trump; //current Trump suit i.e. suit that beats other suits
  char suit; //current lead suit - MUST follow suit.
  char score[2]; //scores for teams 1 and 2 
  char tricks[4]; //number of tricks taken by each player 
  char bidHolder = 1; //who called up trump?
  char i; // misc loop variable
  char inPlay[4] = {-1, -1, -1, -1}; //cards currently played in the current trick
  char lead = 1; //who leads?
  char dc = 0; //'deck count' - used as loop variable
  char deck[24]; //the deck of cards.
  char hand[4][5]; // 2D array of hands.
  char temp;
  char temp1;

  DisableInterrupts
	initializations(); 		  			 		  		
	EnableInterrupts;
  for(dc = 0; dc < 24; dc++)//generate the deck - 
  {
	  deck[dc] = dc;
  }
 for(;;) {
  //some deck shuffling thing
  for(i = 0; i < 20; i++) {  
    for(dc = 0; dc < 12; dc++) 
    {
  	  temp = deck[dc];
  	  deck[dc] = deck[dc + 12];
  	  deck[dc+12] = temp; 
    }
    for(dc = 0; dc < 24; dc++) {
      ATDCTL5 = 0x10;
      while(!ATDSTAT0_SCF); 
      temp1 = ATDDR0H % 24;
      temp = deck[temp1];
      deck[temp1] = deck[dc];
      deck[dc] = temp;   
    }
  }
  
  //this deck shuffling thing
  score[0] = 0;
  score[1] = 0;
 	hand[0][0] = deck[0];
	hand[0][1] = deck[1];
	hand[0][2] = deck[2];
	hand[1][0] = deck[3];
	hand[1][1] = deck[4];
	hand[2][0] = deck[5];
	hand[2][1] = deck[6];
	hand[2][2] = deck[7];
	hand[3][0] = deck[8];
	hand[3][1] = deck[9];
	hand[0][3] = deck[10];
	hand[0][4] = deck[11];
	hand[1][2] = deck[12];
	hand[1][3] = deck[13];
	hand[1][4] = deck[14];
	hand[2][3] = deck[15];
	hand[2][4] = deck[16];
	hand[3][2] = deck[17];
	hand[3][3] = deck[18];
	hand[3][4] = deck[19];
	upcard = deck[20];
  
  //trump = checkSuit(upcard, '\0')//, upcard);
  lcdPick = 1;
	send_i(LCDCLR);
	lcdPick = 2;
	send_i(LCDCLR);
  lead = 0;
  // LCD TEST
  lcdPick = 2;
  chgline(LINE1);
  pmsglcd("Welcome");
  chgline(LINE2);
  pmsglcd("to");
  
  lcdPick = 1;
  chgline(LINE1);
  pmsglcd("Euchre");
  chgline(LINE2);
  pmsglcd("Shuffling...     ");
	//play
	delaySet(15);
	printHand(hand[0],trump,lead);
	trump = bid(hand);
	delaySet(6);
	lcdPick = 1;
	send_i(LCDCLR);
	lcdPick = 2;
	send_i(LCDCLR);
	printHand(hand[0],trump,lead);
	delaySet(6);
	for(dc = 0; dc < 5; dc++) //Five tricks
	{
	  suit = ' ';	
	  //lead suit should reset every trick
		for(i = 0; i < 4; i++) //four plays per trick
		{
		  
			lead = lead % 4; // prevents overflows
			if(lead == 0) {
			  playHand = 1;
			  determineValidity(hand[0], trump, suit);
			  printBoard(inPlay, 0);
			  hold = cardSelect();
			  send_i(LCDCLR);
			  inPlay[0] = hand[0][hold];
			  hand[0][hold] = -1;
			  lead++;
			  if(suit != 'c' && suit != 'd' && suit != 'h' && suit != 's'){
			    suit = checkSuit(inPlay[0], trump);
			  }
			  playHand = 0;
			  
			} else {

			  lcdPick = 2;
			  chgline(LINE2);
			  pmsglcd("Thinking...     ");
			  delaySet(3);
			  //thinking 
  			if(suit != 'c' && suit != 'd' && suit != 'h' && suit != 's')
  			{
  				suit = playCard(lead, hand[lead], inPlay, trump, suit); //where each side plays their card in trick 
  			}
  			else
  			{
  				playCard(lead, hand[lead], inPlay, trump, suit); //where each side plays their card in trick 
  			}
  			lcdPick = 2;
  			chgline(LINE1);
  			print_c(checkNum(inPlay[lead]));
        print_c(checkSuit(inPlay[lead], '\0'));
        print_c(0x20); // blank space
        chgline(LINE2);
        pmsglcd("Player ");
        print_c(lead + 0x30);
        pmsglcd(" plays  ");
        delaySet(6);	    
  			lead++; // lead inc
			}
			printHand(hand[0], trump, suit);
			hold = 0;
		}
		//bid = 0;
		printBoard(inPlay, 0);
		lead = trickWin(tricks, inPlay, trump, suit); //calculates who won the trick, sets lead for next round 
    chgline(LINE2);
    pmsglcd("Trick Winner: ");
    print_c(lead + 0x30);
    delaySet(10);
    send_i(LCDCLR);
    score[lead % 2]++;
    lcdPick = 2;
    chgline(LINE1);
    pmsglcd("P0-P2 score: ");
    print_c(score[0] + 0x30);
    chgline(LINE2);
    pmsglcd("P1-P3 score: ");
    print_c(score[1] + 0x30);
    delaySet(10);
    send_i(LCDCLR);
    inPlay[0] = 254;
    inPlay[1] = 254;
    inPlay[2] = 254;
    inPlay[3] = 254;
	}
		lcdPick = 2;
	  chgline(LINE2);
	  if(score[0] > 2){
      pmsglcd("P0-P2 win!");
	  } else {
	    pmsglcd("P1-P3 win!");
	  }
	  score[0] = 0;
	  score[1] = 0;
	  delaySet(20);
//	trickCount(score, tricks); //increments score based on tricks   
  } /* loop forever */
   
}   /* do not leave main */


void determineValidity(char hand[5], char trump, char suit)
{
  char i;
  char hold; 
  char card = 0;
	char trumpContained;
	char suitContained;
	suitContained = containsSuit(hand, suit, trump, 1);
	trumpContained = containsSuit(hand, trump, trump, 1);
	if((!suitContained || (suit != 'c' && suit != 'd' && suit != 'h' && suit != 's')) && trumpContained) // if you don't have suit, but have trump, play trump. Lead trump if possible
	{
	  for(i = 0; i < 5; i++){
	    if(hand[i] != -1) {
	      validity[i] = 1;  
	    } else {
	      validity[i] = 0;
	    }
	  }
		validity[findHighest(hand, trump, trump, 1)] = 2;
	} else if(!suitContained && !trumpContained) // if you don't have the lead suit, and you don't have trump, throw bad cards
	{
	  for(i = 0; i < 5; i++){
	    if(hand[i] != -1) {
	      validity[i] = 1;  
	    } else {
	      validity[i] = 0;
	    }
	  }
	  if(suit != 'c' && suit != 'd' && suit != 'h' && suit != 's') {
	    hold = findHighest(hand, trump, suit, 1);
	  } else{
	    hold = findLowest(hand, trump, '\0', 1);
	  }
	  validity[hold] = 2;

	}
	else if(suitContained) //if you have the suit, you have to follow it
	{
	  for(i = 0; i < 5; i++){
	    if(checkSuit(hand[i], trump) == suit) {
	      validity[i] = 1;  
	    } else {
	      validity[i] = 0;
	    }
	  }
	  hold = findHighest(hand, trump, suit, 1);
	  if(checkSuit(hand[hold], trump) == suit) {
	    validity[hold] = 2;
	  } else {
	    validity[hold] = 0;
	  }
	}
  for(i = 0; i < 5; i++){
	  if(hand[i] == -1) {
	     validity[i] = 0;  
	   } 
	} 
} 
/* Inputs:
 * char* hand: array of cards
 * char* validity: array of card validity for hand. 0 is invalid, 1 is valid, 2 is best
 * Returns:
 * char hold - index of card selected
 * Function:
 * User selects card via external switches read via ATD. Function does not exit until valid card is selected
 */
char cardSelect() {  
  short hold[2];
  short m = 0; 
  short hold1 = 30;
  char validCheck = 0;
  char selection = 0;
  char card;
  lcdPick = 2;
  chgline(LINE2);
  hold[0] = 1;
  hold[1] = 2;
  pmsglcd("Select A Card   ");
  while(!validCheck){
    ATDCTL5 = 0x10;
    while(!ATDSTAT0_SCF);
    m++;
    m = m % 2;  
    hold[m] = ATDDR0H;
    delaySet(1);
    if(hold[0] == hold[1]){
      
      switch(hold[0]) {
        case 43:
        case 44: //833 mV      // the forumla is ATD = V * 255 / 5   // maybe 128?
        case 45:
        card = 4;
        selection = 1;
        break;
        case 85:
        case 86:
        case 87: //1.67 V
        card = 3;
        selection = 1; 
        break;
        case 127:
        case 128:
        case 129://2.5  V
        card = 2;
        selection = 1;
        break;
        case 169:
        case 170: //3.33 V
        case 171:
        card = 1;
        selection = 1;
        break;
        case 212:
        case 213: //4.17 V 
        case 214:
        card = 0;
        selection = 1;
        break;
        default:
        card = 5;
        break;
      }
      if((card == 5 || !validity[card]) && selection) {
        selection = 0;
        lcdPick = 2;
        chgline(LINE2);
        pmsglcd("Invalid Card   ");
        continue;  
      } else if(selection && validity[card]) {
        lcdPick = 2;
        chgline(LINE2);
        pmsglcd("Selected ");
        print_c(card + 0x30);
        pmsglcd("       ");
        validCheck = 1;
      }
    }
  }
  delaySet(6);
  chgline(LINE2);
  pmsglcd("Flip Switch Back");
  delaySet(6);
  return card; 
}
/* Inputs:
 * int** hand: arrays of hands
 * char upcard: upcard
 * Returns:
 * char trump - trump suit
 * Function:
 * Performs rounds of bidding
 */
char bid(char hand[4][5])//, char upcard)
{
  char hold;
	char i = 0;
	char ordered = 0;
	char trump = '\0';
	char low = -1;
	char thresh = 3;
	trump = checkSuit(upcard, '\0');
	lcdPick = 2;
	send_i(LCDCLR);
	chgline(LINE1);
  pmsglcd("Upcard: ");
  print_c(checkNum(upcard));
  print_c(checkSuit(upcard, '\0'));
  pmsglcd("    ");
  	 
	for(i = 0; i < 4; i++)
	{
	  if(i == 0) {
	 	  validity[0] = 1;
	    validity[1] = 0;
	    validity[2] = 0;
	    validity[3] = 0;
	    validity[4] = 1;
	    
	    lcdPick = 1;
	    chgline(LINE2);
	    pmsglcd("Y Order Up?   N ");
	    
	    hold = cardSelect();
	    if(hold == 0) {
        ordered = 1;
	      bidholder = 0;
	      break;
	    }
	    
	  } else {
	    //delay
	    delaySet(6);
  		if(containsSuit(hand[i], trump, trump, 1) < 3)
  		{
  			//printf("%d passes\n", i);
  			lcdPick = 2;
  			chgline(LINE2);
  			print_c('P');
  			print_c(i+0x30);
  			pmsglcd(" passed       ");
  			  
  		}
  		else
  		{
  			//printf("%d orders it up\n", i);
  			lcdPick = 2;
  			chgline(LINE2);
  			print_c('P');
  			print_c(i + 0x30);
  			pmsglcd(" ordered it up");
  			ordered = 1;
  	    bidholder = i;
  			break;
  		}
	  }
	}	    
	delaySet(6);
	if(ordered)
	{	
	  trump = checkSuit(upcard, '\0');
    low = findLowest(hand[3], trump, '\0', 1); //finds lowest card in South that isn't a trump 	
		hand[3][low] =  upcard; // swaps them
		return trump; //check suit of Upcard
	}
	else
	{
		//printf("upcard declined!\n");
		lcdPick = 2;
		chgline(LINE2);
		pmsglcd("Upcard Declined!");
		
		// delay
		delaySet(6);
		for(i = 0; i < 4; i++)
		{
		  if(i == 0) {
		    validity[0] = 1;
	      validity[1] = 1;
	      validity[2] = 1;
	      validity[3] = 1;
	      validity[4] = 1;
	      lcdPick = 2;
	      chgline(LINE1);
	      pmsglcd("User Select Suit");
	      lcdPick = 1;
	      chgline(LINE2);
	      pmsglcd("C D H S Pass   ");
	      
	      hold = cardSelect();
	      switch(hold) {
	        case 0:
	          bidholder = 0;
	          return 'c';
	        break;
	        case 1:
	          bidholder = 0;
	          return 'd';
	        break;
	        case 2:
	        	bidholder = 0;
	          return 'h';
	        break;
	        case 3:
	        	bidholder = 0;
	          return 's';
	        break;
	        case 4:
	        break;
	      }
		  } else {
		    
		    //delay
		    delaySet(6);
		    lcdPick = 2;
  			chgline(LINE2);
  			if(containsSuit(hand[i], 'c', 'c', 1) > thresh)
  			{
  			  print_c('P');
  			  print_c(i + 0x30);
	        pmsglcd(" - clubs     ");
  				bidholder = i;
  				return 'c';
  			}
  			else if(containsSuit(hand[i], 'd', 'd', 1) > thresh)
  			{
  			  print_c('P');
  			  print_c(i + 0x30);
	        pmsglcd(" - diamonds   ");
  				bidholder = i;
  				return 'd';
  			}
  			else if(containsSuit(hand[i], 'h', 'h', 1) > thresh)
  			{
  				print_c('P');
  			  print_c(i + 0x30);
	        pmsglcd(" - hearts   ");
  				bidholder = i;
  				return 'h';
  			}
  			else if(containsSuit(hand[i], 's', 's', 1) > thresh)
  			{
  				print_c('P');
  			  print_c(i + 0x30);
	        pmsglcd(" - spades   ");
  				bidholder = i;
  				return 's';
  			}
  			else
  			{
  				if(i != 3)
  				{
  					//printf("%d passes\n", i);
  					chgline(LINE2);
  					pmsglcd("P");
  					print_c(i+0x30);
  					pmsglcd(" passed       ");
  					
  				}
  				else
  				{
  				  bidholder = i;
  					//printf("Dealer has been stuck\n");
  					chgline(LINE2);
  					pmsglcd("Dealer is stuck  ");
  					
  					i = 2;
  					thresh--;
  				}
  			}
		  }
		}
	}

}
/* Inputs:
 * char card1: trump card in play
 * char card2: trump card in play
 * char trump: trump suit
 * Returns:
 * char card
 * Function:
 * Two trumps enter, one trump leaves
 */

char trumpSuccession(char card1, char card2, char trump)
{	
	if(card2 == -1)
	{
		return card1;
	}
	if(!(checkNum(card1) == 'J' || checkNum(card2) == 'J')) //if neither card is a bower, it's reasonably straightforward
	{
		return card1 > card2 ? card1 : card2;
	}
	else //otherwise, you have to do weird bower checking
	{
		if((card1 % 6) == (card2 % 6)) //if both bowers are in play, the on-suit one wins
		{
			return checkSuit(card1, '\0') == trump ? card1 : card2;
		}
		else
		{
			return card1 % 6 == 2 ? card1 : card2;
		}
	}
}
/* Inputs:
 * int* tricks: size 4 array of tricks won per player in hand
 * int* inPlay: size 4 array of cards in play
 * char trump: current trump suit in hand
 * char suit: current lead suit in trick
 * Returns:
 * char index of trick of winner
 * Function:
 * Checks inPlay to determine who won the trick. Highest trump wins by default. Otherwise you have to check the highest of the lead suit. Increments trick winner trick count. 
 */
char trickWin(char tricks[5], char inPlay[4], char trump, char suit)
{
	char k = 0;
	char winningCard = -1;
	char currentWinner = -1;
	char hasTrump;
	hasTrump = containsSuit(inPlay, trump, trump, 0);
	if(hasTrump) //if there is trump in play, it wins
	{
		for(k = 0; k < 4; k ++)
		{
			if(checkSuit(inPlay[k], trump) == trump)
			{
				winningCard = trumpSuccession(inPlay[k], winningCard, trump);
				currentWinner = indexOf(winningCard, inPlay, 4);
			}
		}
	}
	else
	{
		currentWinner = findHighest(inPlay, '\0', suit, 0);
	}
	if(currentWinner == -1) {
	  currentWinner = 0;   
	}
	return currentWinner;
}
/* Inputs:
 * char find: value to find in array
 * int* array: array to look in
 * char length: length of array
 * Returns:
 * char index of desired value
 * Function:
 * Sequential search for value
 */

char indexOf(char find, char array[5], char length)
{
	char k = 0; 
	for(k = 0; k < length; k++)
	{
		if(array[k] == find)
		{
			return k;
		}
	}
	return -1;
}
/* Inputs:
 * char player: index of inPlay for that player
 * int* hand: size 5 char array of cards in hand
 * int* inPlay: size 3 char array of cards currently on the table
 * char trump: trump suit for current hand 
 * char suit: currently lead suit for current trick
 * Returns:
 * char suit of card played
 * Function:
 * Dumb AI for card handling. Will throw bad cards if no trump is available and cannot follow suit. Will play trump when possible. Will follow suit with highest available card.
 */
char playCard(char player, char hand[5], char inPlay[4], char trump, char suit)
{
	char i = 0;
  char card = 0;
	char trumpContained;
	char suitContained;
	suitContained = containsSuit(hand, suit, trump, 1);
	trumpContained = containsSuit(hand, trump, trump, 1);
	if(!suitContained && !trumpContained) // if you don't have the lead suit, and you don't have trump, throw bad cards
	{
	  if((suit != 'c' && suit != 'd' && suit != 'h' && suit != 's')) {
	    card = findHighest(hand, trump, suit, 1);
	  } else {
		  card = findLowest(hand, trump, '\0', 1);
	  }
	}
	else if((!suitContained || (suit != 'c' && suit != 'd' && suit != 'h' && suit != 's')) && trumpContained) // if you don't have suit, but have trump, play trump. Lead trump if possible
	{
	  if(trickWin(hand, inPlay, trump, suit) == ((player + 2) % 4)) {
	    card = findLowest(hand, trump, suit, 1);  
	  }
	  else {
	    inPlay[player] = hand[card];
		card = findLowest(hand, trump, trump, 1);
		if(trickWin(hand, inPlay, trump, suit) == player) {
			card = findLowest(hand, trump, trump, 1);
		} else {
			card = findHighest(hand, trump, trump, 1);
		}
	  }
	}
	else if(suitContained) //if you have the suit, you have to follow it
	{
	  if(trickWin(hand, inPlay, trump, suit) == ((player + 2) % 4)) {
	    card = findLowest(hand, trump, suit, 1);  
	  }
	  else {
		card = findHighest(hand, trump, suit, 1);
		inPlay[player] = hand[card];
		if(trickWin(hand, inPlay, trump, suit) != player) {
			card = findLowest(hand, trump, suit, 1);
		} else {
			card = findHighest(hand, trump, suit, 1);
		}
	  }
	}
	inPlay[player] = hand[card];
	hand[card] = -1;
	return checkSuit(inPlay[player], trump);

}
/* Inputs:
 * int* hand: size 4 OR 5 char array, player hand
 * char trump: suit of trump for hand
 * char followSuit: suit to be followed, if applicable
* char hand: 1 is hand array of size 5. 0 is inPlay of size 4
 * Returns:
 * char highest: index of card to play
 * Function:
 * returns highest card in hand within constraints. If suit to be follow is null, it is ignored. 
 */
char findHighest(char hand[5], char trump, char suit, char pHand)
{
	char highest;
	char max = -1;
	char k;
	char suitReq = 1;
	if(suit != 'c' && suit != 'd' && suit != 'h' && suit != 's') //if there is no lead suit, all cards are valid
	{
		suitReq = 0;
	}
	for(k = 0; k < (4 + pHand); k++)
	{
		if(hand[k] % 6 > max && (!suitReq || (checkSuit(hand[k], trump) == suit)) && hand[k] < 24)
		{
			max = hand[k] % 6;
			highest = k;
		}	
	}
	return highest;
}
/* Inputs:
 * int* hand: size 4 OR 5 char array, player hand
 * char trump: suit of trump for hand
 * char hand: 1 is hand array of size 5. 0 is inPlay of size 4
 * Returns:
 * char lowest: index of card to play
 * Function:
 * Selects lowest card in hand that isn't trump
 */
char findLowest(char hand[5], char trump, char suit, char pHand)
{
	char lowest;
	char min = 7;
	char k;
	char suitReq = 1;
	if(suit != 'c' && suit != 'd' && suit != 'h' && suit != 's') //if there is no lead suit, all cards are valid
	{
		suitReq = 0;
	}
	for(k = 0; k < (4 + pHand); k++)
	{
		if(!suitReq || checkSuit(hand[k], trump) == suit) {  
			if(hand[k] % 6 < min && hand[k] != -1)
			{
				min = hand[k] % 6;
				lowest = k;
			}	
		}
	}
	if(min == 7) //all trump;
		lowest = 0;
	return lowest;
}
/* Inputs:
 * char suit: suit to search for
 * int* hand: size 5 char array of player hand
 * Returns:
 * char count: number of cards per suit
 * Function:
 * returns number of cards of that suit
 */

char containsSuit(char hand[5], char suit, char trump, char pHand)
{	
  char count = 0;
	char k = 0;
	if(suit != 'c' && suit != 'd' && suit != 'h' && suit != 's') {
	  return 0; 
	}

	for(k = 0; k < (4 + pHand); k++)
	{
		if(checkSuit(hand[k], trump) == suit)
		{
			count++;
		}
	}
	
	return count;
}
/* Inputs:
 * char in: card to check
 * char trump: current trump suit
 * Returns:
 * char suit of card 
 * Function:
 * returns suit of card
 */
char checkSuit(char in, char trump) //Clubs, Diamonds, Hearts, Spades; A-9-10-J-Q-K
{
	if(in < 0)
	{
		return ' ';
	}
	if(in < 6)
	{
		if((trump == 's') && (in == 2)) //special case for trump - jack of same color but opposite suit is treated as trump suit
		{
			return 's'; 
		}
		return 'c';
	}
	else if (in < 12)
	{	
		
		if((trump == 'h') && (in == 8))
		{
			return 'h';
		}
		return 'd';
	}
	else if (in < 18)
	{

		if((trump == 'd') && (in == 14))
		{
			return 'd';
		}
		return 'h';
	}
	else
	{	
		if((trump == 'c') && (in == 20))
		{
			return 'c';
		}
		return 's';
	}
}
/* Inputs:
 * char in: card
 * Returns:
 * char out: character representing value of card
 * Function:
 * Returns value of card
 */

char checkNum(char in)
{
	char out = ' ';
	switch(in % 6)
	{
		case 0:
			out = '9';
			break;
		case 1: 
			out = 'T';
			break;
		case 2: 
			out = 'J';
			break;
		case 3:
			out = 'Q';
			break;
		case 4:
			out = 'K';
			break;
		case 5: 
			out = 'A';
			break;
	}
	return out;
}

void printHand(char hand[5], char trump, char suit) {
  char i;
  char num;
  char cardSuit;
  // the hand
  lcdPick = 1;
  chgline(LINE1);
  for(i = 0; i < 5 ; i++) {
    num = checkNum(hand[i]);
    cardSuit = checkSuit(hand[i], '\0');
    print_c(num);
    print_c(cardSuit);
    print_c(0x20); // blank space	      
  }
  
  chgline(LINE2);
  pmsglcd("Trump: ");
  print_c(trump);
  pmsglcd(" Lead: ");
  print_c(suit);
  pmsglcd("   ");
  
}

void printBoard(char hand[5], char bid) {
  // the board
  char i;
  lcdPick = 2; 
  chgline(LINE1);
  
  if(bid) {
    
    pmsglcd("Upcard: ");
    print_c(checkNum(upcard));
    print_c(checkSuit(upcard,'\0'));
    //// print score
    
  } else {
  chgline(LINE1);
  for(i = 0; i < 4 ; i++) {
    print_c(checkNum(hand[i]));
    print_c(checkSuit(hand[i], '\0'));
    print_c(0x20); // blank space	      
  }
    
    
  }
  // if bid == 0 print all of the other cards, wait a certain amount of time to display, while waiting, print thinking       // this is how we will use the TIM interrupt 
  // if bid == 1 
  chgline(LINE2);
  // print thinking 
  
    
}

void delaySet(char sec) {
  TIE = 0x80;
  delayThresh = sec * 100;
  delayed = 0;
  while(!delayed) {
    
  }
    
}
/*
***********************************************************************                       
 RTI interrupt service routine: RTI_ISR

 Initialized for 8.192 ms interrupt rate

  Samples state of pushbuttons (PAD7 = left, PAD6 = right)

  If change in state from "high" to "low" detected, set pushbutton flag
     leftpb (for PAD7 H -> L), rghtpb (for PAD6 H -> L)
     Recall that pushbuttons are momentary contact closures to ground	
************************************************************************
*/

interrupt 7 void RTI_ISR(void)
{
  	// clear RTI interrupt flagt 
  	char j;
  	char pwm;
  	CRGFLG = CRGFLG | 0x80;
  	
  	
  	if(playHand) {
  	  for(j = 0; j < 5; j++) {
  	    if(validity[j] == 0) {
  	      pwm = 0;  
  	    } else if(validity[j] == 1) {
  	      pwm = 20;
  	    } else {
  	      pwm = 254;
  	      
  	    }
  	    
  	    switch(j) {
  	      
    	    case 0: PWMDTY0 = pwm;
    	    break;
    	    case 1: PWMDTY1 = pwm;
    	    break;
    	    case 2: PWMDTY2 = pwm;
    	    break;
    	    case 3: PWMDTY3 = pwm;
    	    break;
    	    case 4: PWMDTY4 = pwm;
    	    break;
  	    }
  	  }
  	} else {
  	  PWMDTY0 = 0;
  	  PWMDTY1 = 0;
  	  PWMDTY2 = 0;
  	  PWMDTY3 = 0;
  	  PWMDTY4 = 0;
  	  
  	}
 

}

/*
***********************************************************************                       
  TIM interrupt service routine

  Initialized for 10.0 ms interrupt rate

  Uses variable "tencnt" to track if one-tenth second has accumulated
     and sets "tenths" flag 
                         
  Uses variable "onecnt" to track if one second has accumulated and
     sets "onesec" flag		 		  			 		  		
;***********************************************************************
*/

interrupt 15 void TIM_ISR(void)
{
  	// clear TIM CH 7 interrupt flag 
 	TFLG1 = TFLG1 | 0x80; 
 	
 	// wait 3 seconds for each person to play?
 	if(++inc == delayThresh) {
 	  delayed = 1; // it is done
 	  inc = 0;
 	  TIE = 0;
 	}
 	
 

}


/*
***********************************************************************
  shiftout: Transmits the character x to external shift 
            register using the SPI.  It should shift MSB first.  
            MISO = PM[4]
            SCK  = PM[5]
***********************************************************************
*/
 
void shiftout(char x)

{
 
  // test the SPTEF bit: wait if 0; else, continue
  // write data x to SPI data register
  // wait for 30 cycles for SPI data to shift out 
   int k = 0;
   while(SPISR_SPTEF == 1) {
     SPIDR = x;
   }
   while(k < 30) {
     k++;
   }
}

/*
***********************************************************************
  lcdwait: Delay for approx 2 ms
***********************************************************************
*/

void lcdwait()
{
  short wait;
  for(wait = 0; wait<5000; wait++);
 
}

/*
*********************************************************************** 
  send_byte: writes character x to the LCD
***********************************************************************
*/

void send_byte(char x)
{
     // shift out character
     // pulse LCD clock line low->high->low
     // wait 2 ms for LCD to process data
     shiftout(x);
     
     /*if(lcdPick == 0) {
      PTM &= ~LCDCLK_HAND;
      PTM |= LCDCLK_HAND;
      PTM &= ~LCDCLK_HAND;
       
      PTM &= ~LCDCLK_BOARD;
      PTM |= LCDCLK_BOARD;
      PTM &= ~LCDCLK_BOARD; 
           
     } else */if(lcdPick == 1) {
      PTM &= ~LCDCLK_HAND;
      PTM |= LCDCLK_HAND;
      PTM &= ~LCDCLK_HAND;
     } else if(lcdPick == 2) {
      PTM &= ~LCDCLK_BOARD;
      PTM |= LCDCLK_BOARD;
      PTM &= ~LCDCLK_BOARD; 
     }
     
     lcdwait();
      
}

/*
***********************************************************************
  send_i: Sends instruction byte x to LCD  
***********************************************************************
*/

void send_i(char x)
{
        // set the register select line low (instruction data)
        // send byte
        PTT &= ~RS;
        send_byte(x);
}

/*
***********************************************************************
  chgline: Move LCD cursor to position x
  NOTE: Cursor positions are encoded in the LINE1/LINE2 variables
***********************************************************************
*/

void chgline(char x)
{

        send_i(CURMOV);
        send_i(x);

}

/*
***********************************************************************
  print_c: Prchar (single) character x on LCD            
***********************************************************************
*/
 
void print_c(char x)
{
        PTT |= RS;
        send_byte(x);
}     

/*
***********************************************************************
  pmsglcd: prchar character string str[] on LCD
***********************************************************************
*/

void pmsglcd(char str[])
{
      char index = 0;
      while(str[index] != 0) {
        print_c(str[index]);
        index++;
      }
}




