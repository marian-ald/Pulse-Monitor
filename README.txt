/*
 * Aldescu Marian
 * 331 CA
 * README
 */


 								Proiect PM


 	Arhiva este compusa din fisierele:
 	- pulse.c (programul principal)
 	- cele 2 biblioteci pentru usart si lcd(usart.h/.c, lcd.h/.c)
 	- plot.py (scriptul in python)
 	- Makefile

 	Frecventa quartz-ului este de 16MHz(este specificata ca parametru in Makefile)

 	Pentru compilare este folosit avr-g++
 	Pentru instalare:
 		http://fab.cba.mit.edu/classes/863.16/doc/projects/ftsmin/windows_avr.html

 	Pentru USART frecventa este 9600 bps

 	Pentru scriptul in python este necesar python 3 si pe deasupra este nevoie de
 	bibliotecile:
	 	matplotlib
		serial
		drawnow
		atexit

	Pentru a porni conversia, se apasa butonul, iar bulsul va fi afisat pe lcd.
	Atata timp cat nu este terminat de calculat pulsul, se va afisa mesajul:
		"Calibrare..."

	Senzorul va fi plasat cu ajutorul clestisorului pe lobul oricarei urechi.