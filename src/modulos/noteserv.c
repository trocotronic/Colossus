/*
 * $Id: noteserv.c,v 1.5 2007-03-01 15:30:22 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/noteserv.h"

NoteServ *noteserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, noteserv->hmod, NULL)

BOTFUNC(ESEntrada);
BOTFUNCHELP(ESHEntrada);
BOTFUNC(ESSalida);
BOTFUNCHELP(ESHSalida);
BOTFUNC(ESVer);
BOTFUNCHELP(ESHVer);

void ESSet(Conf *, Modulo *);
int ESTest(Conf *, int *);
int ESSigSQL		();
int CompruebaNotas();

Timer *timercomp = NULL;

static bCom noteserv_coms[] = {
	{ "entrada" , ESEntrada , N1 , "Inserta una entrada en tu agenda." , ESHEntrada } ,
	{ "salida" , ESSalida , N1 , "Elimina una entrada de tu agenda." , ESHSalida } ,
	{ "ver" , ESVer , N1 , "Examina tus notas para un d�a determinado." , ESHVer } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

char *santos[] = {
	"Santa Mar�a: Madre de Dios, San Tel�maco, San Fulgencio" ,
	"San Basilio Magno , San Gregorio Nacianzeno y San Macario" ,
	"Santa Genoveva" ,
	"San Roberto, Santa Isabel Seton y Santa �ngela de Foligno" ,
	"San Sime�n Estilita y Beato Juan Nepomuceno" ,
	"La Epifan�a y Los Reyes Magos, Beata Rafaela Mar�a Porras" ,
	"San Raimundo De Pe�afort y San Carlos de Sezze" ,
	"San Severino, San Luciano, Santa Peggy y Santa G�dula" ,
	"San Adri�n de Canterbury, San Eulogio, San Juli�n, Santa Marciana y Beata Alicia Le Clerc" ,
 	"San Guillermo de Bourges, San Pedro de Ors�olo y San Gonzalo" ,
	"San Vital de Gaza, San Paulino de Aquilea, San Higinio y San Teodosio" ,
	"Santa Margarita Bourgeoy, San Arcadio, San Benito Biscop, San Ailred y Santa Cesarina" ,
	"San Hilario, Santa Yvette y Beato Hildemaro" ,
	"San F�lix de Nola y San Juan de Ribera" ,
	"San Alejandro el Acemeta, San Mauro, San Pablo de Tebas y San Remigio" ,
	"San Marcelo I, Santos Berardo, Od�n, Pedro, Adjunto y Acurso, San Honorato" ,
	"San Antonio, Santa Rosalina de Villeneuve y San Amalberto" ,
	"Santa Prisca (Priscila) y San Liberto" ,
	"Santos Mario, Marta y sus Hijos Audifax y Abaco, San Canuto y San Macario" ,
	"San Fabi�n, San Sebasti�n y San Eutimio" ,
	"Santa In�s, virgen y m�rtir" ,
	"San Vicente y Beata Laura Vicu�a" ,
	"San Ildefonso, San Bernardo y San Juan El Limosnero" ,
	"San Francisco de Sales obispo y doctor de la Iglesia" ,
	"La Conversi�n de San Pablo, Ap�stol" ,
	"Santos Timoteo y Tito, obispos" ,
	"Santa Angela De Merici, virgen" ,
	"Santo Tom�s De Aquino presb�tero y doctor de la Iglesia" ,
	"San Sulpicio Severo, San Gildas o Gildosio y San Pedro Nolasco" ,
	"Santa Batilde y San Lesmes, San Fulgencio de Ruspe, Santa Jacinta de Mariscotti" ,
	"San Juan Bosco, presb�tero" ,
	"Santa Emma, Santa Viridiana, Beato Andr�s, San Ra�l, Santa Alicia y Beata Ella..." ,
	"La Presentaci�n del Se�or, Santa Juana, San Cornelio, Beato Juan Te�fano y Santa Catalina de Ricci" ,
	"San Blas y San Oscar" ,
	"San Gilberto, San Te�filo y Santa Juana de Francia" ,
	"Santa Agueda y San Felipe de Jes�s" ,
	"San Pablo Miki, San Amando y San Gast�n..." ,
	"Beata Eugenia Smet" ,
	"San Jer�nimo Emiliano, Beata Jacqueline" ,
	"San Miguel Febres Cordero y Santa Apolonia" ,
	"Santa Escol�stica y Beato Arnaldo Cattaneo" ,
	"Nuestra Se�ora de Lourdes, San Benito de Aniano y San Adolfo" ,
	"Santa Eulalia, m�rtir" ,
	"San Gregorio II" ,
	"San Cirilo, San Metodio, San Valent�n y San Mar�n" ,
	"Santa Jorgina, San Faustino y San Jovita, San Claudio de la Colombi�re" ,
	"Santos Elias Jeremias, Isaias Samuel y Daniel, San On�simo, Santa Juliana y San Macario" ,
	"Los Siete Santos Fundadores de la Orden de los Siervos de la Virgen Mar�a" ,
	"San Flaviano, Santa Bernardita y Beato Francisco Regis Clet" ,
	"San Conrado de Plasencia y San �lvaro de C�rdoba" ,
	"San Euquerio de Orleans, Beata Amada, Beatos Francisco y Jacinta Marto" ,
	"San Pedro Dami�n obispo y doctor de la Iglesia" ,
	"La C�tedra de San Pedro, Santa Margarita de Cortona, Santa Isabel de Francia..." ,
	"San Policarpo, obispo y m�rtir" ,
	"San Vart�n y compa�eros" ,
	"Beato Sebasti�n de Aparicio, San Avertano y el Beato Romeo, San Etelberto y Santa Jacinta" ,
	"San N�stor y San Porfirio" ,
	"San Leandro, Santa Honorina y San Gabriel" ,
	"San Rom�n y san Lupicino, Santa Antonieta y San Hilario" ,
	"Santa Eudoxia, San David y San Albino" ,
	"Beato Carlos El Bueno y San Nicol�s de Flue" ,
	"Santa Katharine Drexel, San Guenol�, Santos Marino y Asterio" ,
	"San Casimiro" ,
	"San Juan Jos� de la Cruz" ,
	"Santa Mar�a de la Providencia" ,
	"Santas Perpetua y Felicitas, San Pablo el Simple" ,
	"San Juan de Dios, Santos Filem�n, Apolonio y Arieno" ,
	"Santa Francisca Romana y San Gregorio de Nisa" ,
	"Santa Anastacia la Patricia, San Juan de Mata y M�rtires de Sebaste" ,
	"San Eulogio" ,
	"Santa Fina, San Inocencia I, San Maximiliano y San Pol de Le�n" ,
	"Santos Rodrigo y Salom�n, Beato �ngel de Pisa y San Humberto" ,
	"Santa Matilde" ,
	"Santa Luisa de Marillac y San Clemente Mar�a Hofbauer" ,
	"San Juli�n de Anazarba, San Crist�dulo y Santa Benita" ,
	"San Patricio, obispo" ,
	"San Cirilo de Jerusal�n, San Salvador de Horta y San Alejandro" ,
	"San Jos�, Esposo de la Virgen Mar�a" ,
	"Santa Fotina la Samaritana, San Heriberto, San Mart�n Dumiense y Beato Marcel Callo" ,
	"San Nicol�s de Flue" ,
	"Santa Lea" ,
	"Santo Toribio de Mogrovejo y Beata Sibila" ,
	"Santa Catalina de Suecia" ,
	"La Anunciaci�n del Se�or" ,
	"San Ludgerio" ,
	"Beato Peregrino o Pelegrino" ,
	"San Guntrano o Gontr�n, rey de Borgo�a" ,
	"San Eustasio" ,
	"San Juan Cl�maco" ,
	"San Benjam�n" ,
	"San Hugo" ,
	"San Francisco de Paula, ermita�o" ,
	"San Ricardo, Obispo" ,
	"San Isidoro obispo y doctor de la Iglesia" ,
	"San Vicente Ferrer, presb�tero" ,
	"San Prudencio, San Marcelino y Beata Pierina Morosini" ,
	"San Juan Bautista de la Salle y Beato Germ�n" ,
	"Santa Julia Billiard, San Dionisio, San Gualterio (Walter), Beata Constanza" ,
	"Santa Casilda, San Vadim y San Lorenzo de Irlanda" ,
	"San Juan Bautista Vel�squez y martires, San Fulberto, San Dimas y Beato Antonio Neyrot" ,
	"San Estanislao, obispo y m�rtir" ,
	"Santa Gemma Galgani, San Julio, San Zen�n, San Sabas y Santa Teresa de los Andes" ,
	"San Mart�n I, San Hermenegildo, San Marte y Beata Ida" ,
	"Santa Lidia o Liduvina" ,
	"San Paterno De Vannes y Beato Lucio" ,
	"Santa Engracia y San Benito Jos� Labre" ,
	"San Esteban Harding, San Aniceto, Beata Catalina Tekakwitha y Beata Mar�a De La Encarnaci�n" ,
	"San Francisco Solano, San Perfecto y San Apolonio" ,
	"Santa In�s de Montepulciano, Santa Emma, San Expedito, San Wernerio y San Le�n IX" ,
	"San Telmo, San Marcelino, San Teotimio, San Gerardo, Beata Odette y Santa Hildegunda" ,
	"San Anselmo y San Conrado de Parzham" ,
	"Santa Mar�a Egipciaca y Santa Oportuna" ,
	"San Jorge, San Aniano, Beato Egidio de As�s" ,
	"San Fidel de Sigmaringa, presb�tero y m�rtir" ,
	"San Marcos y San Pedro de Betancur" ,
	"San Tarcisio, San Isidoro, San Pascasio, Beata Alida y San Riquerio" ,
	"Nuestra Se�ora de Montserrat, Santa Zita y San Antimio" ,
	"San Pedro Chanel, San Luis Mar�a, Santa Teodora, San D�dimo, San Vital y Santa Valeria" ,
	"Santa Catalina de Siena, virgen y doctora de la Iglesia" ,
	"San P�o V, San Jos� Benito Cottolengo, San Roberto San Adjutorio y Beata Rosamunda" ,
	"San Jos� Obrero, San Segismundo y Beata Mar�a Leonia Paradis" ,
	"San Atanasio, Santa Zoe, Santos Exuperio, Te�dulo y Ciriaco" ,
	"La Santa Cruz y San Juvenal" ,
	"Santos Felipe y Santiago, San Gotardo, San Flori�n y San Silvano" ,
	"San Antonino, San Hilario de Arles, San �ngel y Santa Judith" ,
	"Santo Domingo Savio, San Evodio, San Mariano y Beata Prudencia" ,
	"Santa Flavia Domitila y Beata Gisela" ,
	"Job, San Pedro de Tarantasia, San Desiderio y Beata Mar�a Droste zu Vischering" ,
	"Santa Mar�a Mazzarelllo y San Gregorio Ostiense" ,
	"San Juan de �vila y Santa Solange" ,
	"Santa Juana de Arco, San Mamerto, Santa Estrella, San Mayolo, San Francisco de Jer�nimo..." ,
	"Santos Nereo, San Aquileo, San Pancracio, Santo Domingo, San Epifanio, San Felipe, San Germ�n..." ,
	"Nuestra Se�ora de F�tima, San Andr�s F., San Servasio, Santa Rolanda, Santa Magdalena y Santa In�s" ,
	"San Mat�as, San Pacomio, San Miguel Garico�ts, Santa Aglae y San Bonifacio, San Isidoro, San Poncio" ,
	"San Isidro Labrador, Santa Dionisia, Santos Pablo y Andr�s, San Victorino, San Aquileo, Santa Juana" ,
	"San Juan Nepomuceno, San Andr�s B�bola, San Sim�n Stock, San Ubaldo, Santos Alipio y Posidio" ,
	"San Pascual Bail�n, Beata Antonia Mesina y Beato Pedro Ouen-Yen" ,
	"San Juan I, San Leonardo Murialdo, San F�lix, Santa Rafaela, Santa Mar�a Josefa, Beata Blandina..." ,
	"San Celestino V, San Dunstan, San Iv�n, San Urbano I, Beata Mar�a Bernarda, Beato Francisco Coll" ,
	"San Bernardino de Siena, San Protasio Chong Kurbo y Beata Columba de Rieti" ,
	"Santa Gisela, San Eugenio de Mazenod, Beato Jacinto Mar�a Cormier y M�rtires de M�xico" ,
	"Santa Rita de Casia, Santa Julia , San Lorenzo Ngon, Beato Juan Forest y Beata Mar�a Dominica" ,
	"San Juan Bautista de Rossi y Santa Juana Antida Thouret" ,
	"Mar�a Auxiliadora , San Donaciano, San Rogaciano y M�rtires Coreanos" ,
	"San Beda, Santos Crist�bal Magallanes y Agust�n Caloca, San Gregorio VII, Santa Mar�a Magdalena..." ,
	"San Felipe Neri, Santa Mariana, San Jos� Chang, San Juan Doa y Mateo Nguy�n, San Pedro M�rtir Sans" ,
	"San Agust�n de Canterbury, San Atanasio Bazzekuketta, Santa B�rbara Kim y B�rbara Yi, San Gonzalo" ,
	"Santa Ripsimena, San Germ�n de Par�s, San Pablo Han y Beata Margarita Pole" ,
	"Santos Sisinio, Martorio y Alejandro, Beato Ricardo Thirkeld, Beata �rsula Ledochowska" ,
	"San Fernando, Beatos Lucas Kirby, Guillermo Filby, Lorenzo Richardson, Tom�s Cottam,Beato Mat�as..." ,
	"La Visitaci�n de la Sant�sima Virgen Mar�a, Santa Petronila, Beatos F�lix, Roberto, Tom�s y Nicol�s" ,
	"San Justino, San P�nfilo, San Ren�n, San Caprasio, Beato An�bal y Beato Juan Bautista Scalabrini" ,
	"Santos Marcelino y Pedro, Santa Blandina, San Potino y Santo Domingo Ninh" ,
	"San Carlos Lwanga , Beato Juan XXIII, Santa Mariana de Jes�s, San Kevin y San Pablo Duong" ,
	"Santa Clotilde, San Ascanio Caracciolo, Santa Vicenta Gerosa y Beato Felipe Smaldone" ,
	"San Bonifacio, Santo Domingo To�i y Santo Domingo Huyen" ,
	"San Norberto, San Marcelino Champagnat, Santos Pedro Dung, Pedro Thuan , Vicente Duong, Beato Rafael" ,
	"San Gilberto, San Pedro de C�rdoba y Beata Mar�a Teresa de Soubiran" ,
	"San Medardo, Beata Mar�a del Divino Coraz�n y Beato Santiago Barthieu" ,
	"San Efr�n, San Columba, Beato Jos� de Anchieta y Beata Diana Dandalo" ,
	"Santa Margarita de Escocia, Santa Olivia y Beata Ana Mar�a Taigi" ,
	"San Bernab�, Santa Alicia, Santa Mar�a Rosa Molas, Santa Paula Frassinetti y Beata Mar�a Schinin�" ,
	"San Juan de Sahag�n, San Onofre, San Gaspar Bortoni, Beata Mercedes de Jes�s, Beato Guido" ,
	"San Antonio de Padua, San Agust�n Phan Viet Huy y San Nicol�s Bui Duc The" ,
	"San Metodio, San Valero, San Rufino y Eliseo" ,
	"Santa Germana, San Vito, Santa Mar�a Micaela, Santa B�rbara, Beato Luis Mar�a, Beata Yolanda" ,
	"San Juan Francisco, San Ciro, San Aureliano, Santa Ma. Teresa, Santo Domingo Nguyen y compa�eros" ,
	"San Gregorio Barbarigo, San Rainiero, San Heberto y San Pedro Da" ,
	"Santa Isabel de Sh�nau, Santa Juliana y Beata Osanna" ,
	"San Romualdo, Santos Gervasio y Protasio, San Remigio Isor�, San Modesto, Beata Miguelina Metelli" ,
	"San Silverio y Santa Florentina" ,
	"San Luis Gonzaga, San Jos� Isabel Flores, San Anton Mar�a Schwartz, San Jacobo Kern y Santa Restitua" ,
	"Santo Tom�s Moro, San Paulino De Nola y San Juan Fisher" ,
	"San Gaspar de B�falo, San Jos� Cafasso, Beato Bernhard Lichtenberg y Beata Mar�a Rafaela" ,
	"La Natividad de San Juan Bautista y San Juan Yuan Zaide" ,
	"San Guillermo De Vercelli, San Pr�spero, Santa Leonor y San Salom�n" ,
	"San Josemar�a Escriv� de Balaguer, San Jos� Mar�a Robles, y San Jos� Mar�a Ma-Tai-Shun" ,
	"Nuestra Se�ora del Perpetuo Socorro, San Cirilo, San Ladislao, San Jos� Hien, Santo Tom�s Toan..." ,
	"San Ireneo, San Jer�nimo Lu Tingmey, Santa Mar�a Tou-Tchao-Cheu, Santa Vicenta Gerosa..." ,
	"San Pedro y San Pablo, Santos Mar�a Tian de Du, Magdalena Du Fengju, Pablo Wu Kiunan..." ,
	"Los Primeros Santos M�rtires, San Raimundo y San Pedro Li Quanzhu, San Vicente D� Y�n" ,
	"San Sime�n, San Teodorico, San Servando, Santos Justino Orona y Atilano Cruz , Beato Jun�pero..." ,
	"San Ot�n de Bamberg, San Martiniano, San Procesio y Beata Eugenia Joubert" ,
	"Santo Tom�s, San Felipe Phan, San Jos� Nguyen, Santos Pedro y Juan Bautista Zhao, Beato Raimundo..." ,
	"Nuestra Se�ora del Refugio, Santa Isabel de Portugal, Santa Berta, San Valent�n de Berrio-Ochoa..." ,
	"San Antonio Mar�a, San Miguel de los Santos,Santas Teresa Jinjie y Rosa Chen Anjie, Beato El�as" ,
	"Santa Mar�a Goretti, San Pedro Wang Zuolong, Beata Nazaria Ignacia y Beata Mar�a Teresa Led�chowska" ,
	"Santos Ra�l Milner y Rogelio Dickenson, San Vilibaldo, San Ferm�n, San Marcos Ji Tianxiang..." ,
	"San Edgar, San Procopio, San Kili�n, San Teobaldo, Isa�as, San Juan Wu" ,
	"Santa Ver�nica, M�rtires de Orange, Santos Teodorico y Nicasio, San Gregorio Grassi, San Joaqu�n Ho" ,
	"San Crist�bal, Santos Antonio Nguyen Quynh y Pedro Nguyen Khac, Beato Pac�fico y M�rtires de Damasco" ,
	"San Benito, Santa Olga, Santas Ana Xin de An, Mar�a Guo de An, Ana Jiao de An y Mar�a An Linghua" ,
	"San Juan Gualberto, San Ignacio-Clemente, Santa In�s Le Thi, San Pedro Khan , Beato Oliverio Plunket" ,
	"San Enrique, San Eugenio de Cartago, Profeta Joel , Santa Mildred y Beato Carlos Manuel Rodr�guez" ,
	"San Camilo De Lelis, San Francisco Solano, San Juan Wang, Beato Guillermo Repin y Beato Ghebre" ,
	"San Buenaventura, San Donald, San Andr�s Nam-Thuong, San Pedro Tuan, Beata Ana Mar�a, Beato Pedro" ,
	"Nuestra Se�ora del Carmen, Santa Mar�a Magdalena Postel, San Mil�n, Santos Lang Yang y Pablo Lang" ,
	"San Alejo, Santa Marcelina, Las Beatas Carmelitas de Compi�gne y San Pedro Liu Ziyn" ,
	"San Federico, San Arnulfo de Metz, Santa Marina y Santo Domingo Dinh Dat" ,
	"San Arsenio, Santa �urea, Santas Justa y Rufina, Santa Isabel Blan de Qin y San Sim�n Qin" ,
	"San Aurelio, Santa Margarita, Profeta El�as, San Jos� Mar�a D�az Sanjurjo, San Le�n Ignacio Mangin" ,
	"San Lorenzo de Brindisi, San V�ctor, San Alberico Crescitelli y San Jos� Wang-Yumel" ,
	"Santa Mar�a Magdalena , San Vandrilio, Santas Ana Wang, Luc�a Wang-Wang, Mar�a Wang y San Andr�s W." ,
	"Santa Br�gida, San Apolinar, Beatos Manuel P�rez, Nic�foro, Vicente D�az, Pedro Ruiz y compa�eros" ,
	"Santa Ver�nica Giuliani, M�rtires de Guadalajara, Santa Cristina, San Jos� Fern�ndez" ,
	"Santiago Ap�stol, Beatos Dionisio, Federico, Primo, Jer�nimo, Juan de la Cruz, Mar�a, Pedro, F�lix.." ,
	"San Joaqu�n y Santa Ana, Santa Bartolomea Capitanio y Beato Jorge Preca" ,
	"Santa Natalia, San Aurelio, Beato Sebasti�n de Aparicio y Beato Tito Brandsman" ,
	"Santa Mar�a Josefa Rosello, San Melchor Garc�a Sampedro, Beato Pedro Poveda y Beata Alfonsa" ,
	"Santa Marta, San Olaf , San Lupo de Troyes, Santa Julia, San Jos� Zhang Wenlan..." ,
	"San Pedro Cris�logo, San Germ�n de Auxerre, San Leopoldo, Santa Mar�a de Jes�s Sacramentado Venegas" ,
	"San Ignacio de Loyola, Santos Pedro Doan Cong Quy y Manuel Phung" ,
	"San Alfonso Mar�a de Ligorio, San Bernardo Vu Van Du�, Santo Domingo Hanh y Beata Mar�a Estela" ,
	"San Eusebio de Vercelli y Beato Francisco Calvo" ,
	"San Pedro Juli�n Eymard, Santa Lidia y Santa Juana de Chantal" ,
	"San Juan Mar�a Vianney, San Aristaco, Beato Federico Janssoon, Beato Gonzalo Gonzalo" ,
	"La Dedicaci�n de la Bas�lica de Santa Mar�a la Mayor, San Abel, Santa Afra y San Osvaldo" ,
	"La Transfiguraci�n del Se�or, Santos Justo y Pastor, Beato Octaviano y Beata Ma.Francisca de Jes�s" ,
	"San Sixto II, San Cayetano, San Donato y San Miguel de la Mora" ,
	"Santo Domingo, San Cir�aco, San Pablo Ge Tingzhu, Beata Mar�a de la Cruz y Beata Mar�a Margarita" ,
	"Santa Teresa Benedicta de la Cruz, San Oswaldo, Santa Otilia, Santos Juliano, Marciano, Fotio..." ,
	"San Lorenzo, Di�cono y M�rtir" ,
	"Santa Clara, San G�ry, Santa Susana y Beato Mauricio Tornay" ,
	"San Benilde Roman�on, Santa Hilaria, San Eleazar, Santos Antonio Pedro, Santiago Do Mai Nam..." ,
	"San Ponciano, San Hip�lito, San Estanislao de Kostka, San Juan Berchmans, San Benildo..." ,
	"San Maximiliano Mar�a Kolbe, Santa Atanasia, Beato Everardo, Beata Isabel Renzi" ,
	"Asunci�n de la Sant�sima Virgen Mar�a, San Arnulfo, San Tarsicio, San Luis Batis S�inz..." ,
	"San Esteban de Hungr�a, Beato Bartolom� Laurel, San Roque..." ,
	"San Jacinto de Cracovia, San Liberato y San Agapito" ,
	"Santa Elena" ,
	"San Juan Eudes, Beatos Pedro Zu�iga y Luis Flores" ,
	"San Bernardo, San Filiberto y Profeta Samuel" ,
	"San P�o X, San Sidonio Apolinar, Santos Crist�bal y Leovigildo..." ,
	"Nuestra Se�ora Mar�a Reina, San Sigfrido y San Sinforiano" ,
	"San Felipe Benicio" ,
	"San Bartolom� y San Audoeno" ,
	"San Luis, San Jos� de Calasanz, San Gin�s, Beato Tom�s de Kempis, Beato Luis Urbano Lanaspa" ,
	"San Ces�reo, Santa Teresa de Jes�s Jornet e Ibars y Beato Jun�pero Serra" ,
	"Santa M�nica, San Guer�n y San Amadeo" ,
	"San Agust�n, San Hermes, San Mois�s el Et�ope, Beatos Constantino Fern�ndez y Francisco Monz�n" ,
	"El Martirio de San Juan Bautista , Beato Edmundo Ignacio Rice y Beata Teresa Bracco" ,
	"Santa Rosa de Lima, San Fiacre, Beato Alfredo Ildefonso, Beatos Diego Ventaja y Manuel Medina" ,
	"San Ram�n Nonato, San Ar�stides, Beatos Emigdio, Amalio y Valerio Bernardo" ,
	"San Gil" ,
	"Beato Bartolom� Gutierrez" ,
	"San Gregorio Magno, Beatos Juan Pak, Mar�a Pak, B�rbara Kouen, B�rbara Ni, Mar�a Ni e In�s Kim" ,
	"Santa Rosal�a, Santa Irma y San Marino" ,
	"San Lorenzo Justiniano, Santa Raisa, Santos Pedro Nguyen Van Tu y Jos� Hoang Luong Canh" ,
	"Santa Eva de Dreux, San Magno, San Beltr�n, San Eleuterio, Beato Contardo Ferrini" ,
	"Santa Regina, San Columbano y Beato Juan Bautista Mazzuconi" ,
	"Natividad de la Sant�sima Virgen Mar�a y Beato Federico Ozanam" ,
	"San Pedro Claver, San Audemaro, San Auberto, Beato Santiago Laval, Beata Serafina, Beato Alano..." ,
	"San Nicol�s de Tolentino y Beato Francisco Garate" ,
	"San Pafnucio, San Adelfo, Santa Vinciana y San Juan Gabriel Perboyre" ,
	"San Guido, Beata Victoria Fornari y Beato Apolinar Franco" ,
	"San Juan Cris�stomo y San Amado" ,
	"Exaltaci�n de la Santa Cruz, San Materno, Santa Notburga y Beato Gabriel Taurino Dufresse" ,
	"Nuestra Se�ora de los Dolores y Beato Rolando" ,
	"San Cornelio y Cipriano, Santa Edith, Santa Ludmila, Beatos Juan Bautista y Jacinto de los �ngeles" ,
	"San Roberto Belarmino, San Lamberto y Santa Hildegarda" ,
	"San Jos� de Cupertino, Santa Ricarda, San Juan Mac�as, Santo Domingo Do� Trach..." ,
	"San Jos� Ma. de Yermo y Parr�s, San Jenaro, Santa Mar�a de Cervell�, Santa Emilia de Rodat" ,
	"San Andr�s Kim Taegon, San Pablo Chong Hasang, San Pedro de Arbu�s y San Juan Carlos Cornay" ,
	"San Mateo, Ap�stol y Evangelista" ,
	"San Mauricio, Santo Tom�s de Villanueva,San Carlos Navarro Miquel, Beato Jos� Aparicio Sanz..." ,
	"San Constancio, San P�o de Pietrelcina, Santa Tecla, San Andr�s Fournet, Beatos Cristobal, Antonio.." ,
	"Nuestra Se�ora de la Merced" ,
	"San Ferm�n, San Carlos de Sezze, Beato Germ�n y Beato Jos� Benito Dusmet" ,
	"Santos Cosme y Dami�n, San Nilo, San Isaac Jogues, San Sebasti�n Nam, Santa Teresa Couderc..." ,
	"San Vicente de Pa�l, presb�tero" ,
	"San Wenceslao, San Lorenzo Ruiz, San Fausto, San Exuperio y Beato Francisco Castell� Aleu" ,
	"Los Santos Arc�ngeles Miguel, Gabriel y Rafael, San Alarico" ,
	"San Jer�nimo, Beato Conrado de Urach y Beato Federico Albert" ,
	"Santa Teresa del Ni�o Jes�s" ,
	"Los Santos �ngeles Custodios y San Leodegario" ,
	"San Francisco De Borja, Santos Evaldo el Moreno y Evaldo el Rubio..." ,
	"San Francisco de As�s, Santa Aurea y Beato Michel Callo" ,
	"San Mauro y San Pl�cido, Santa Flora, Santa Faustina Kowalska, Beato Bartolom� Longo..." ,
	"San Bruno, Santa Fe, Beata Marie Rose Durocher, Beata Mar�a Ana Mogas, Beato Isidoro de Loor.." ,
	"Nuestra Se�ora del Rosario, San Artaldo, San Augusto, San Sergio" ,
	"Santa Pelagia, San Juan Calabria, Santa Thais y Santa Edwiges" ,
	"San Dionisio, San Juan Leonardi, San Luis Beltr�n, Santos Inocencio, Jaime Hilario..." ,
	"Santo Tom�s De Villanueva y San Virgilio" ,
	"Santa Soledad Torres Acosta, San Ferm�n, San Pedro Le Tuy, Beato Juan XXIII y Beata Mar�a de Jes�s" ,
	"Nuestra Se�ora del Pilar, San Seraf�n, San Wilfrido..." ,
	"San Geraldo, San Te�filo de Antioqu�a y San Eduardo" ,
	"San Calixto I, Papa y m�rtir" ,
	"Santa Teresa de Jes�s, virgen y doctora de la Iglesia" ,
	"Santa Eduviges, Santa Margarita Mar�a Alacoque, San Beltr�n, San Galo, Beata Margarita de Youville" ,
	"San Ignacio de Antioqu�a, obispo y m�rtir" ,
	"San Lucas, Evangelista" ,
 	"San Juan de Br�beuf, San Isaac Jogues , San Lucas del Esp�ritu Santo, San Pablo de la Cruz..." ,
	"Santa Irene, Santa Bertilia, Beata Adelina y Beato Contardo Ferrini" ,
	"San Hilari�n, Santa �rsula , Santa Celina, San Gerardo Mar�a Mayela y San Antonio Mar�a Gianelli" ,
	"San Felipe De Heraclea, Santas Elodia y Nunil�n, Beato Timoteo Giaccardo" ,
	"San Juan de Capistrano, San Servando, San Germ�n, San Pablo T�ng Vi�t Buong, Beato Arnoldo Reche" ,
	"San Antonio Mar�a Claret, San Jos� L� Dang Thi, Beato Rafael Gu�zar, Beato Luis Guanella..." ,
	"Santos Crisp�n y Crispiniano, Cuarenta M�rtires de Inglaterra y Gales..." ,
	"Beata Bona De Armagnac" ,
	"San Frumencio, San Demetrio Bassarabov, Santo Dominguito.." ,
	"Santos Sim�n y Judas, San Rodrigo Aguilar Alem�n" ,
	"San Narciso, San Marcelo el Centuri�n, Santa Ermelinda y Beato Miguel R�a" ,
	"Santa Dorotea Swartz, San Alonso Rodr�guez y Beata Bienvenida" ,
	"Todos los Santos" ,
	"Todos los fieles difuntos" ,
	"San Mart�n de Porres, San Huberto, San Gu�nael , Santa Silvia, San Pedro Jos� Almat�..." ,
	"San Carlos Borromeo, San Em�rico, San Amancio y San Jesse" ,
	"San Zacar�as y Santa Isabel, San Celestino, San Bertila, Beato Guido Ma. Conforti..." ,
	"San Leonardo, San Severo, San Alejandro, San Jos� Kong y Beata Josefa Naval Girb�s" ,
	"San Ernesto, San Engelberto y Beato Francisco Palau y Quer" ,
	"Beato Juan Duns Scot, San Godofredo, San Andr�s Avelino, Beata Isabel de la Trinidad..." ,
	"La Dedicaci�n de la Bas�lica de Letr�n" ,
	"San Le�n Magno, San Baudilio y Santa Natalena" ,
	"San Mart�n de Tours, obispo" ,
	"San Josafat, San Emiliano, San Margarito Flores y Santa Agustina Livia Petrantoni" ,
	"San Diego de Alcal�, San Leandro, San Eugenio, San Homobono, San Estanislao de Koska..." ,
	"San Serapi�n, San Esteban Teodoro Cuenot y Beata Magdalena Morano" ,
	"San Alberto Magno, San Jos� Pignatelli, San Malo..." ,
	"Santa Gertrudis, San Omar, San Jos� Moscati y Beato Aurelio Mar�a" ,
	"Santa Isabel de Hungr�a, Santa Hilda y San Gregorio de Tours" ,
	"La Dedicaci�n de las Bas�licas de San Pedro y San Pablo, San Od�n, Santa Filipina Rosa Duchesne..." ,
	"San Tanguy y San Rafael Kalinoswki de San Jos�" ,
	"San Edmundo, San Roque de Santa Cruz, Santos Octavo, Adventorio y Solutorio..." ,
	"La Presentaci�n de la Sant�sima Virgen Mar�a, San Gelasio, B. Ma. del Buen Pastor" ,
	"Santa Cecilia y San Filem�n" ,
	"Beato Miguel Agustin Pro, San Clemente I, San Columbano..." ,
	"San Andr�s Dung-Lac, Santas Flora y Mar�a y otros" ,
	"Santa Catalina Labour�" ,
	"San Leonardo De Puerto Mauricio, Beata Delfina, San Ele�zar..." ,
	"San M�ximo, San Severino y San Virgilio" ,
	"San Hilario, Santa Quieta, San Santiago de la Marca y San Andr�s Tr�n Van Tr�ng" ,
	"San Gregorio Taumaturgo, San Saturnino, San Radbodo..." ,
	"San Andr�s, San Z�simo y San Jos� Marchand" ,
	"San Eloy, Santa Florencia, San Edmundo Campion ..." ,
	"Santa Bibiana (Viviana)" ,
	"San Francisco Javier y San Galgano Guidotti" ,
	"San Juan Damasceno, Santa B�rbara, San Osmondo y B. Adolfo Colping" ,
	"San Sab�s, Santa Atal�a y San Geraldo" ,
	"San Nicol�s, Santa Dionisia y compa�eros m�rtires, Beata Carmen Sall�s" ,
	"San Ambrosio" ,
	"La Inmaculada Concepci�n De La Virgen Mar�a" ,
	"San Pedro Fourier, San Juan Diego y Beato Bernardino Silvestrelli" ,
	"La Virgen De Loreto, San Romarico y Santa Valeria" ,
	"San D�maso I, San Daniel y Beata Maravillas de Jes�s" ,
	"Nuestra Se�ora de Guadalupe" ,
	"Santa Luc�a y Santa Odilia" ,
	"San Juan de la Cruz y San Fortunato" ,
	"Santa Cristiana (Nina), Santa Maria de la Rosa y Beato Carlos Steeb" ,
	"Santa Adelaida (o Alicia) y San Everardo" ,
	"Santa Olimpia y San L�zaro" ,
	"San Flavio, San Winebaldo y San Modesto" ,
	"Santa Sametana y Beato Urbano V" ,
	"Santo Domingo De Silos" ,
	"San Pedro Canisio, presb�tero y doctor de la Iglesia" ,
	"Santa Francisca Javiera Cabrini y Beato Graciano" ,
	"San Juan Cancio, San Thorlakur y Beato Armando" ,
	"San Charbel Majluf, San Gregorio, San Justo y San Viator..." ,
	"Natividad De Nuestro Se�or Jesucristo" ,
	"San Esteban y San Nicomedes" ,
	"San Juan y Santa Fabiola" ,
	"Los Santos Inocentes y San Gaspar del B�falo" ,
	"Santo Tom�s Becket, obispo y m�rtir" ,
	"La Sagrada Familia de Jes�s, Mar�a y Jos�" ,
	"San Silvestre I, Santa Melania , Santa Colomba y San Mario" 
};
Opts horos[] = {
	{ 21 , "Capricornio" } ,
	{ 21 , "Acuario" } ,
	{ 20 , "Piscis" } ,
	{ 20 , "Aries" } ,
	{ 20 , "Tauro" } ,
	{ 21 , "G�minis" } ,
	{ 22 , "C�ncer" } ,
	{ 22 , "Leo" } ,
	{ 21 , "Virgo" } ,
	{ 22 , "Libra" } ,
	{ 21 , "Escorpio" } ,
	{ 22 , "Sagitario" } ,
};

ModInfo MOD_INFO(NoteServ) = {
	"NoteServ" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};
int MOD_CARGA(NoteServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El m�dulo ha sido compilado para la versi�n %i y usas la versi�n %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}
	//mod->activo = 1;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuraci�n de %s", mod->archivo, MOD_INFO(NoteServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(NoteServ).nombre))
			{
				if (!ESTest(modulo.seccion[0], &errores))
					ESSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuraci�n de %s no ha pasado el test", mod->archivo, MOD_INFO(NoteServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(NoteServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		ESSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(NoteServ)()
{
	BorraSenyal(SIGN_SQL, ESSigSQL);
	ApagaCrono(timercomp);
	BotUnset(noteserv);
	return 0;
}
int ESTest(Conf *config, int *errores)
{
	int error_parcial = 0;
	Conf *eval;
	if ((eval = BuscaEntrada(config, "funcion")))
		error_parcial += TestComMod(eval, noteserv_coms, 1);
	*errores += error_parcial;
	return error_parcial;
}
void ESSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!noteserv)
		noteserv = BMalloc(NoteServ);
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, noteserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, noteserv_coms);
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
		}
	}
	else
		ProcesaComsMod(NULL, mod, noteserv_coms);
	InsertaSenyal(SIGN_SQL, ESSigSQL);
	timercomp = IniciaCrono(0, 60, CompruebaNotas, NULL);
	BotSet(noteserv);
}
time_t CreaTime(int dia, int mes, int ano, int hora, int min)
{
	struct tm *ttm;
	time_t tt = time(0);
	ttm = localtime(&tt);
	ttm->tm_sec = 0;
	ttm->tm_min = min;
	ttm->tm_hour = hora;
	ttm->tm_mday = dia;
	ttm->tm_mon = mes-1;
	ttm->tm_year = ano-1900;
	return mktime(ttm);
}
BOTFUNCHELP(ESHEntrada)
{
	return 0;
}
BOTFUNCHELP(ESHSalida)
{
	return 0;
}
BOTFUNCHELP(ESHVer)
{
	return 0;
}
BOTFUNC(ESEntrada)
{
	int dia, mes, ano, hora = 0, min = 0, avisa = 0, pm = 1;
	time_t tt;
	char *m_c;
	if (params < 2)
	{
		Responde(cl, CLI(noteserv), ES_ERR_PARA, fc->com, "[dd/mm/yy [hh:mm]] [n�] nota");
		return 1;
	}
	if (sscanf(param[1], "%i/%i%/%i", &dia, &mes, &ano) != 3)
	{
		struct tm *ttm;
		time_t ahora = time(0);
		ttm = localtime(&ahora);
		dia = ttm->tm_mday;
		mes = ttm->tm_mon+1;
		ano = ttm->tm_year+1900;
	}
	else
		pm++;
	if (ano < 100)
		ano += 2000;
	if (param[2] && sscanf(param[2], "%i:%i", &hora, &min) == 2)
		pm++;
	if (params == pm)
	{
		Responde(cl, CLI(noteserv), ES_ERR_EMPT, "Debes especificar alguna nota");
		return 1;
	}
	tt = CreaTime(dia, mes, ano, hora, min);
	avisa = atoi(param[pm]);
	m_c = SQLEscapa(Unifica(param, params, avisa? pm+1 : pm, -1));
	SQLQuery("INSERT INTO %s%s (item,fecha,avisar,nota) VALUES ('%s',%lu,%lu,'%s')", PREFIJO, ES_SQL, cl->nombre, tt, tt-avisa, m_c);
	Free(m_c);
	Responde(cl, CLI(noteserv), "Se ha a�adido esta entrada correctamente.");
	return 0;
}
BOTFUNC(ESSalida)
{
	int dia, mes, ano, hora = 0, min = 0;
	time_t tt;
	if (params < 2)
	{
		Responde(cl, CLI(noteserv), ES_ERR_PARA, fc->com, "dd/mm/yy [hh:mm]");
		return 1;
	}
	if (sscanf(param[1], "%i/%i%/%i", &dia, &mes, &ano) != 3)
	{
		Responde(cl, CLI(noteserv), ES_ERR_SNTX, "Formato de fecha incorrecto: dia/mes/a�o");
		return 1;
	}
	if (param[2] && sscanf(param[2], "%i:%i", &hora, &min) != 2)
	{
		Responde(cl, CLI(noteserv), ES_ERR_SNTX, "Formato de hora incorrecto: hora:minutos");
		return 1;
	}
	if (ano < 100)
		ano += 2000;
	tt = CreaTime(dia, mes, ano, hora, min);
	if (params == 2)
	{
		SQLQuery("DELETE FROM %s%s WHERE fecha >= %lu AND fecha < %lu AND item='%s'", PREFIJO, ES_SQL, tt, tt+86400, cl->nombre);
		Responde(cl, CLI(noteserv), "Todas las entradas para el d�a \00312%s\003 han sido eliminadas.", param[1]);
	}
	else if (params == 3)
	{
		SQLQuery("DELETE FROM %s%s WHERE fecha = %lu AND item='%s'", PREFIJO, ES_SQL, tt, cl->nombre);
		Responde(cl, CLI(noteserv), "Todas las entradas para el d�a \00312%s\003 a las \00312%s\003 han sido eliminadas.", param[1], param[2]);
	}
	return 0;
}
BOTFUNC(ESVer)
{
	int dia, mes, ano, m = 0;
	time_t tt;
	SQLRes res;
	SQLRow row;
	if (params < 2 || !strcasecmp(param[1], "HOY"))
	{
		struct tm *ttm;
		time_t ahora = time(0);
		ttm = localtime(&ahora);
		tt = CreaTime(ttm->tm_mday, ttm->tm_mon+1, ttm->tm_year+1900, 0, 0);
	}
	else if (!strcasecmp(param[1], "MA�ANA"))
	{
		struct tm *ttm;
		time_t ahora = time(0)+86400;
		ttm = localtime(&ahora);
		tt = CreaTime(ttm->tm_mday, ttm->tm_mon+1, ttm->tm_year+1900, 0, 0);
	}
	else if (sscanf(param[1], "%i/%i%/%i", &dia, &mes, &ano) == 3)
		tt = CreaTime(dia, mes, ano, 0, 0);
	else
	{
		Responde(cl, CLI(noteserv), ES_ERR_SNTX, "Formato de fecha incorrecto: d�a/mes/a�o|HOY|MA�ANA");
		return 1;
	}
	if (!(res = SQLQuery("SELECT * FROM %s%s WHERE fecha >= %lu AND fecha < %lu AND item='%s'", PREFIJO, ES_SQL, tt, tt+86400, cl->nombre)))
	{
		Responde(cl, CLI(noteserv), ES_ERR_EMPT, "No existen entradas para este d�a.");
		return 1;
	}
	while ((row = SQLFetchRow(res)))
	{
		struct tm *ttm;
		time_t tt = atoul(row[1]);
		ttm = localtime(&tt);
		if (!m)
		{
			char *signo;
			if (ttm->tm_mday > horos[ttm->tm_mon].opt)
			{
				if (ttm->tm_mon+1 < 12)
					signo = horos[ttm->tm_mon+1].item;
				else
					signo = horos[0].item;
			}
			else
				signo = horos[ttm->tm_mon].item;
			strftime(buf, sizeof(buf), "\00312%a\003,\00312 %d %b %Y\003", ttm);
			Responde(cl, CLI(noteserv), "%s (%s)", buf, signo);
			Responde(cl, CLI(noteserv), "%s", santos[ttm->tm_yday]);
			Responde(cl, CLI(noteserv), " ");
			m = 1;
		}
		if (ttm->tm_hour && ttm->tm_min)
			Responde(cl, CLI(noteserv), "\00312%02i:%02i\003 - %s", ttm->tm_hour, ttm->tm_min, row[3]);
		else
			Responde(cl, CLI(noteserv), "- %s", row[3]);
	}
	SQLFreeRes(res);
	return 0;
}
int ESSigSQL()
{
	if (!SQLEsTabla(ES_SQL))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"item varchar(255), "
  			"fecha int4, "
  			"avisar int4, "
  			"nota varchar(255), "
  			"KEY item (item) "
			");", PREFIJO, ES_SQL);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, ES_SQL);
	}
	SQLCargaTablas();
	return 0;
}
int CompruebaNotas()
{
	time_t t = time(0), tr;
	SQLRes res;
	SQLRow row;
	Cliente *al;
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE fecha < %lu", PREFIJO, ES_SQL, t)))
	{
		while ((row = SQLFetchRow(res)))
		{
			if ((al = BuscaCliente(row[0])))
			{
				Responde(al, CLI(noteserv), "Tienes una tarea pendiente", buf);
				Responde(al, CLI(noteserv), "- %s", row[3]);
			}
		}
		SQLFreeRes(res);
	}
	SQLQuery("DELETE FROM %s%s WHERE fecha < %lu", PREFIJO, ES_SQL, t);
	if ((res = SQLQuery("SELECT * FROM %s%s WHERE (avisar < %lu AND avisar > 0)", PREFIJO, ES_SQL, t)))
	{
		while ((row = SQLFetchRow(res)))
		{
			if ((al = BuscaCliente(row[0])))
			{
				tr = atoi(row[1]);
				strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M", localtime(&tr));
				Responde(al, CLI(noteserv), "Recordatorio: tienes una tarea para el \00312%s", buf);
				Responde(al, CLI(noteserv), "- %s", row[3]);
			}
		}
		SQLFreeRes(res);
	}
	SQLQuery("UPDATE %s%s SET avisar=0 WHERE (avisar < %lu AND avisar > 0)", PREFIJO, ES_SQL, t);
	return 0;
}
