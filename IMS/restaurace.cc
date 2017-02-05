#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <simlib.h>

Stat Restaurace("Restaurace: Cas straveny v restauracii");
Stat Caka_stol("Zakaznik: Cas straveny cakanim na stol");

Stat Zak_obsluzenie("Zakaznik: Cas straveny cakanim na obsluhu");
Stat Zak_pitie("Zakaznik: Cas straveny cakanim na pitie");
Stat Zak_polievka("Zakaznik: Cas straveny cakanim na polievku");
Stat Zak_druhe_jedlo("Zakaznik: Cas straveny cakanim na druhe jedlo");
Stat Zak_zaplatenie("Zakaznik: Cas straveny cakanim na zaplatenie");

Stat Ob_pitie("Jedlo: Cas straveny pripravou pitia");
Stat Ob_polievka("Jedlo: Cas straveny pripravou polievky");
Stat Ob_druhe_jedlo("Jedlo: Cas straveny pripravou druheho jedla");

Stat Cakanie("Celkova doba stravena cakanim");

int netrpezlivi = 0;
int nevosli = 0;
int neobsluzeni = 0;
double volno = 0.0;

int POCET_HODIN = 3;
int POCET_STOLOV = 12;
int POCET_OBSLUHY = 2;
double PRICHODY_ZAKAZNIKOV = 4.8;

Store stoly("Stoly", POCET_STOLOV);

int zak_caka_obsluhu = 0;
int zak_chce_platit = 0;
int zak_dostal_pitie = 0;
int zak_dostal_polievku = 0;
int zak_dostal_2jedlo = 0;
int zak_dojedol_polievku = 0;
int zak_bol_obsluzeny = 0;
int zaplatene = 0;
int polievka = 0;
int druhe_jedlo = 0;
int druhe_jedlo_pripravene = 0;
int riady = 0;
int pitie = 0;

enum {Epolievka = 0, Edruhe_jedlo, Epitie};
enum {Esmadny = 0, Ehladny};

class Jedlo : public Process {
public:
	int typ;

	Jedlo(int typ) : Process() {
		this->typ = typ;
	}

	void Behavior() {
		double start = Time;
		switch (typ) {

			case Epolievka:
		   		Wait(Uniform(0.25,0.5)); // priprava polievky
				polievka++;
				Ob_polievka(Time-start);
				return;

			case Edruhe_jedlo: // priprava druheho jedla
		   		Wait(Uniform(5,20));
				druhe_jedlo++;
				Ob_druhe_jedlo(Time-start);
				return;

			case Epitie: //priprava pita
		   		Wait(Uniform(0.7,2));
				pitie++;
				Ob_pitie(Time-start);
				return;
		}
	}
};

// timing class
class Timeout : public Event {
    Process *ptr;               // which process
	int flag;
  public:
    Timeout(double t, Process *p, int flag): ptr(p) {
      Activate(Time+t);         // when to time-out the process
    }
    void Behavior() {
		if (flag == 1) {
			netrpezlivi++;            // counter of time-out operations
		}
		if (flag == 2) {
			ptr->Leave(stoly,1);
	  		zak_caka_obsluhu--;
			neobsluzeni++;
		}
    	delete ptr;               // kill process
    }
};

class Zakaznik : public Process {
public:
	int typ;

	Zakaznik(int typ) : Process() {
		this->typ = typ;
	}

	void Behavior() {
		double prichod = Time;
		double zarazka = Time;
		double cakanie = 0.0;

		if (stoly.Full())
		{
			// 70% ludi odijde a necaka
			double odds = Random();
			if (odds >= 0.3) {
				nevosli++;
				return;
			}
		}
		Event *fronta_stoly = new Timeout(1.8, this, 1); // set timeout
		Enter(stoly, 1);
		delete fronta_stoly;
		Caka_stol(Time-prichod);

		Wait(Exponential(0.5)); // usadenie

		zak_caka_obsluhu++;
		Event *fronta_obsluha = new Timeout(9.1, this, 2); // set timeout
		WaitUntil(zak_bol_obsluzeny > 0)
		{
			delete fronta_obsluha;
			Zak_obsluzenie(Time-zarazka);
			cakanie += Time-zarazka;
			zak_bol_obsluzeny--;
			zarazka = Time;

			if (this->typ == Esmadny)
			{
				(new Jedlo(Epitie))->Activate();
			}
			else if (this->typ == Ehladny)
			{
				(new Jedlo(Epitie))->Activate();
				(new Jedlo(Epolievka))->Activate();
				(new Jedlo(Edruhe_jedlo))->Activate();
			}
		}

		switch (this->typ)
		{
			case Esmadny:
				WaitUntil(zak_dostal_pitie > 0)
			   	{
					Zak_pitie(Time-zarazka);
					cakanie += Time-zarazka;
					zak_dostal_pitie--;
				   	Wait(Exponential(20));//pitie
				   	riady++;
				   	zak_chce_platit++;
				}
				break;

			case Ehladny:
				WaitUntil(zak_dostal_pitie > 0)
			   	{
					zak_dostal_pitie--;
				   	riady++;
				}
		   		WaitUntil(zak_dostal_polievku > 0)
		   		{
					Zak_polievka(Time-zarazka);
					cakanie += Time-zarazka;
					zak_dostal_polievku--;
				   	Wait(Uniform(2,5)); //Jedenie polievky
					zak_dojedol_polievku++;
					riady++;
					zarazka = Time;
				}
				WaitUntil(zak_dostal_2jedlo > 0 && zak_dostal_polievku > 0)
				{
					Zak_druhe_jedlo(Time-zarazka);
					cakanie += Time-zarazka;
					zak_dostal_2jedlo--;
					Wait(Exponential(9.2)); //Jedenie 2. jedla
					riady++;
					zak_chce_platit++;
				}
				break;
		}

		zarazka = Time;
		WaitUntil(zaplatene > 0)
		{
			Zak_zaplatenie(Time-zarazka);
			cakanie += Time-zarazka;
			zaplatene--;
			Leave(stoly,1);
		}
		Restaurace(Time-prichod);
		Cakanie(cakanie);
		cakanie = 0;
	}
};

class Obsluha : public Process {
public:
	void Behavior() {
		double start = Time;
		while(1) {
			if (zak_chce_platit > 0) /*priorita 5 platenie*/
			{
				zak_chce_platit--;
				Wait(Exponential(1.5)); //platenie
				zaplatene++;
				continue;
			}

			if (pitie > 0)	/*priorita 4 pitie*/
			{
				pitie--;
				Wait(Uniform(0.5,1)); //nesie pitie
				zak_dostal_pitie++;
				continue;
			}

			if (polievka > 0) /*priorita 3 polievka*/
			{
				polievka--;
				Wait(Uniform(0.5,1)); //nesie polievku
				zak_dostal_polievku++;
				continue;
			}

			if ((druhe_jedlo > 0) && (zak_dojedol_polievku >0)) /*priorita 2 2. jedlo*/
            {   //odnesie 2.jedlo zakaznikovy
                zak_dojedol_polievku--;
                druhe_jedlo--;
				Wait(Uniform(0.5,1)); //nesie druhe jedlo
                zak_dostal_2jedlo++;
                continue;
            }

			if (zak_caka_obsluhu > 0) /*priorita 1 objednavanie*/
			{
				zak_caka_obsluhu--;
				Wait(Exponential(1.5)); //Objednavanie jedla
				zak_bol_obsluzeny++;
				continue;
			}

			if (riady > 0) /*priorita 0 upratovanie*/
			{
				riady--;
				Wait(Exponential(0.3)); //upratovanie
				continue;
			}
			Wait(0.01); // okno pre dalsie procesy
			volno += Time - start;
		}
	}
};

class Generator : public Event {
	void Behavior() {
		double odds = Random();

		if (odds <= 0.1) /* 10% Iba pite */
		{
			(new Zakaznik(Esmadny))->Activate();
			Activate(Time+Exponential(PRICHODY_ZAKAZNIKOV));
		}
		/* 90% Obedove menu */
		else
		{
			(new Zakaznik(Ehladny))->Activate();
			Activate(Time+Exponential(PRICHODY_ZAKAZNIKOV));
		}
	}
};

int	main(int argc, char *argv[])
{
	// skontroluj argumenty
	if (argc != 4) {
		printf("Zly pocet argumentov");
		return 1;
	}

	// nastavenie poctu stolov
	POCET_STOLOV = atoi(argv[2]);
	// nastavenie poctu obsluhy
	POCET_OBSLUHY = atoi(argv[1]);
	// nastavenie intenzity zakaznikov
	PRICHODY_ZAKAZNIKOV = atof(argv[3]);


	// nastav output file
	SetOutput("restaurace.dat");

	// inicializacia simulacie
	Init(0, POCET_HODIN*60);
	// nastavenie poctu obsluhy
	for (int i = 0; i < POCET_OBSLUHY; i++)
	{
		(new Obsluha)->Activate();
	}
	// inicializacia seedu pre generator nahodnych cisel
	RandomSeed(time(NULL));
	(new Generator)->Activate();
	// spustenie simualcie
	Run();

	// histogramy a statistiky
	// printf("Zakaznik: Pocet netrpezlivych zakaznikov = %d\n", netrpezlivi);
	printf("Zakaznik: Pocet nevosli a odisli = %d\n", nevosli);
	// printf("Volny cas obsluhy = %f\n", volno/POCET_OBSLUHY);
	// printf("Zakaznik: Pocet neobsluzenych = %d\n", neobsluzeni);
	Restaurace.Output();
	// Caka_stol.Output();
	Cakanie.Output();
	Zak_obsluzenie.Output();
	// Zak_pitie.Output();
	// Zak_polievka.Output();
	// Zak_druhe_jedlo.Output();
	// Zak_zaplatenie.Output();
	// Ob_pitie.Output();
	// Ob_polievka.Output();
	// Ob_druhe_jedlo.Output();
	// SIMLIB_statistics.Output();
}
