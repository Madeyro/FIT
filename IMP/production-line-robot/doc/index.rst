.. article:: apps_demo_msp_production-line-robot
   :author: Maroš Kopec <xkopec44 AT stud.fit.vutbr.cz>
   :updated: 6.12.2016

   MSP430: Řízení robota na výrobní lince

========================================
 MSP430: Řízení robota na výrobní lince
========================================

Úvod
==================
Výrobná linka je tvorená lisom (ďalej už len L), priamym zásobným pásom (ďalej už len FB), o dĺžke 5 metrov so samočinným nepretržitým posuvom k L konštantnou rýchlosťou 10cm/s a priamym odkladacím pásom (ďalej už len DB), ), o dĺžke 5 metrov so samočinným nepretržitým posuvom od L konštantnou rýchlosťou 10cm/s. Na linke pracuje robot tvorený dvomi ramenami označenými písmenami A, B, ktoré sú na seba kolmé. Robot je schopný rotačného pohybu v smere alebo proti smeru hodinových ručičiek. Ramená realizujú nezávislý úchop nad materiálom. To znamená, že robot vykonáva tandemový pohyb a je schopný uchopiť materiál ramenom A a ramenom B súčasne.
Celý systém je realizovaný pomocou preemptívneho real-time multitasking operačného systému uC/OC-II bežiaceho na MCU rodiny MSP430 (FITkit 2.0).

VSTUPY DO SYSTÉMU
==================
Systém reaguje na množinu vstupov, ktorými sa ďalej vo svojej činnosti riadi. Tieto vstupy je možné ovládať, čím slúžia ako interakcia s užívateľom. Všetky vstupné dáta upravujú činnosť priameho FB.
Množina vstupov:

**Signál IN**
  Tento signál je možné ovládať pomocou maticovej klávesnice. Jeho počiatočný stav je nastavený na ``False``, čím indikujeme nečinnosť robota nakladajúceho materiál na FB. Stlačením klávesy so symbolom ``#`` sa signál invertuje.

**Premenné beltSecs a beltMilisecs**
  Nastavením týchto premenných vo funkcii ``main()`` má za následok úpravu frekvencie, po akej bude prichádzať materiál po FB. Napríklad pre nastavenie ``beltSecs = 10`` a ``beltMilisecs = 0``, bude FB generovať materiál každých 10 sekúnd. Toto správanie neplatí pre príchod prvého materiálu. Vieme, že prvý materiál musí prejsť celých 5 metrov dlhý FB, čo mu trvá presne 50 sekúnd. Po príchode prvého materiálu na koniec FB nasleduje ďalší materiál. FB je už naplnený a premenné definujú ako blízko je pri sebe materiál.

VÝSTUPY SYSTÉMU
==================
Výstupy systému slúžia ako kontrola a zároveň interakcia s celým systémom. Stav systému a jeho činnosť je možné sledovať na displeji spolu s dvojicou LED diód alebo na termináli.

VÝSTUP NA DISPLEJI, LED DIÓDY
--------------------------------

Výstup na displej je limitovaný na 32 znakov, čo znemožňuje rozsiahlejší popis systému na tomto výstupnom médiu. Užívateľ môže pozorovať aktuálnu pozíciu ramien (pozície sú popísané nižšie), aktuálnu činnosť L, hodnotu signálu IN a počet uskladnených vyrobených produktov.
Dvojica LED diód signalizuje činnosť ramien A, B. Dióda č. 6 predstavuje stav ramena A, dióda č. 5 stav ramena B. Zasvietenie diód predstavuje obsadenosť ramena materiálom/produktom.
Pozície ramien:

- Rameno A sa nachádza nad koncom FB
- Rameno A sa nachádza nad L
- Rameno A sa nachádza nad koncom DB
- Rameno A je paralelne s pásmi FB a DB

VÝSTUP NA TERMINÁL
------------------------

Terminál poskytuje oveľa širšie možnosti pre interakciu s užívateľom vďaka čomu je toto médium bohatšie na informácie. V okne terminálu je možné dozvedieť sa aktuálnu činnosť robota, lisu a informáciu o príchode materiálu na koniec FB.

ČINNOSŤ SYSTÉMU
==================
Systém funguje na operačnom systéme uC/OS—II, ktorý umožňuje real-time multitasking pomocou priorít úloh. Celý systém je rozdelený na 4 úlohy, ktoré sa striedajú v činnosti v závislosti na nastavenej priorite. Operačný systém vyžaduje minimálne jednu úlohu, s najvyššou prioritou, pred spustením samotného operačného systému. Najvyššia priorita je vyjadrená najnižším číslom, teda 0. Dokumentácia k uC/OS-II odporúča nepoužívať priority s číslom 0, 1, 2, 3 a 4. tieto priority môžu byť v budúcnosti využívané samotným OS.
DB je v systéme zanedbaný, na koľko nie je definovaná veľkosť materiálu a činnosť robota spolu s lisom je pomalšia ako pás DB. Preto je vždy možné vložiť produkt na DB.
Systém je navrhnutý pre maximálne využitie oboch ramien súčasne.

Vykonávané úlohy:

**STARTTASK S PRIORITOU 5**
  Úloha vytvorí všetky ďalšie potrebné úlohy a následne prejde do stavu *Waiting*. Permanentný stav *Waiting* je realizovaný nekonečným cyklom, v ktorom je volaná funkcia ``OSTimeDlyHMSM()``.

**KEYBOARDTASK S PRIORITOU 8**
    Úloha obsluhujúca klávesnicu, vykonávaná každých 100 ms. Po stlačení klávesy so symbolom ``#`` invertuje hodnotu signálu IN a volaním funkcie ``OSTaskResume(7)`` implicitne prepína na úlohu obsluhujúcu FB.

**DISPLAYTASK S PRIORITOU 9**
  Úloha obsluhujúca display, vykonávaná každých 500 ms. Výstupné dáta sú vypisované na display.

**BELTSTASK S PRIORITOU 7**
  Úloha obsluhujúca pás FB, vykonávaná každých X s. Frekvencia vykonávania sa nastavuje vo vstupných premenných ``beltSecs`` a ``beltMilisecs``. Ak je systém práve zapnutý a signál IN je nastavený na hodnotu ``True``, potom prvý materiál príde na koniec FB po 50s. Všetok nasledujúci materiál potom prichádza s nastavenou frekvenciou. Táto úloha sa stará aj o prechod do počiatočného stavu po 5000ms nečinnosti na FB.

**CONTROLTASK S PRIORITOU 5**
  Úloha obsluhujúca robota (ramená, otáčanie) a lis L, vykonávaná každú 1 s. Ako prvé sa v úlohe nastavia príznaky ovládajúce činnosť robota a lisu. Počas nastavovania príznakov sú ramená robota aktívne, čo znamená, že uchopenie materiálu je vykonávané už v tejto časti. Následne je spúšťaný lis L nasledovaný rotáciou ramien.


EXPERIMENTY
==================
ÚVAHA A VÝPOČTY
----------------
Maximálny počet materiálu na FB za predpokladu, že nakladací robot vkladá materiál na FB v rovnakom intervale:

``dĺžka pásu / max počet / rýchlosť pásu = frekvencia generovania materiálu``

Ak max. počet = 5
  ``500 / 5 / 10 = 10 s``

Ak max. počet = 10
  ``500 / 10 / 10 = 5 s``

Ak max. počet = 20
  ``500 / 20 / 10 = 2,5 s``

------------

Minimálne vzdialenosti materiálu od seba na FB:

``min vzdialenosť / rýchlosť pásu = frekvencia generovania materiálu``

Ak min. vzdialenosť = 25 cm
  ``25 / 10 = 2,5 s``

Ak min. vzdialenosť = 50 cm
  ``50 / 10 = 5 s``

------------

V našom prípade teda pre frekvenciu generovania platí:

``max. počet 10 == min. vzdialenosť 50 cm``

``max. počet 20 == min. vzdialenosť 25 cm``

------------

Je však potrebné venovať sa úvahe, kde pri maximálnom povolenom počte materiálu na FB je možná situácia, že uvažovaný nakladací robot naloží materiál hneď za seba, čím vznikne frekvencia 1s pre pevný počet materiálu nasledovaný časovou medzerou.

VÝSLEDKY EXPERIMENTOV
----------------------

Čas, za ktorý bolo vyrobených 5 kusov:

=====  =====  =====  =====
         5 kusov
--------------------------
 10s     5s    2,5s   1s
=====  =====  =====  =====
1:42   1:28   1:27   1:26
=====  =====  =====  =====

------------

Počet kusov vyrobených za 5 minút:

=====  =====
 počet kusov
------------
 5s    2,5s
=====  =====
  39    39
=====  =====

------------

Z výsledkov vidíme, že zvýšením frekvencie sa zastavíme na hranici 39 kusov za 5 minút. To znamená, źe optimálne anstavenie je frekvencia 5s.