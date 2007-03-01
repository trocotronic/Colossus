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
	{ "ver" , ESVer , N1 , "Examina tus notas para un día determinado." , ESHVer } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

char *santos[] = {
	"Santa María: Madre de Dios, San Telémaco, San Fulgencio" ,
	"San Basilio Magno , San Gregorio Nacianzeno y San Macario" ,
	"Santa Genoveva" ,
	"San Roberto, Santa Isabel Seton y Santa Ángela de Foligno" ,
	"San Simeón Estilita y Beato Juan Nepomuceno" ,
	"La Epifanía y Los Reyes Magos, Beata Rafaela María Porras" ,
	"San Raimundo De Peñafort y San Carlos de Sezze" ,
	"San Severino, San Luciano, Santa Peggy y Santa Gúdula" ,
	"San Adrián de Canterbury, San Eulogio, San Julián, Santa Marciana y Beata Alicia Le Clerc" ,
 	"San Guillermo de Bourges, San Pedro de Orséolo y San Gonzalo" ,
	"San Vital de Gaza, San Paulino de Aquilea, San Higinio y San Teodosio" ,
	"Santa Margarita Bourgeoy, San Arcadio, San Benito Biscop, San Ailred y Santa Cesarina" ,
	"San Hilario, Santa Yvette y Beato Hildemaro" ,
	"San Félix de Nola y San Juan de Ribera" ,
	"San Alejandro el Acemeta, San Mauro, San Pablo de Tebas y San Remigio" ,
	"San Marcelo I, Santos Berardo, Odón, Pedro, Adjunto y Acurso, San Honorato" ,
	"San Antonio, Santa Rosalina de Villeneuve y San Amalberto" ,
	"Santa Prisca (Priscila) y San Liberto" ,
	"Santos Mario, Marta y sus Hijos Audifax y Abaco, San Canuto y San Macario" ,
	"San Fabián, San Sebastián y San Eutimio" ,
	"Santa Inés, virgen y mártir" ,
	"San Vicente y Beata Laura Vicuña" ,
	"San Ildefonso, San Bernardo y San Juan El Limosnero" ,
	"San Francisco de Sales obispo y doctor de la Iglesia" ,
	"La Conversión de San Pablo, Apóstol" ,
	"Santos Timoteo y Tito, obispos" ,
	"Santa Angela De Merici, virgen" ,
	"Santo Tomás De Aquino presbítero y doctor de la Iglesia" ,
	"San Sulpicio Severo, San Gildas o Gildosio y San Pedro Nolasco" ,
	"Santa Batilde y San Lesmes, San Fulgencio de Ruspe, Santa Jacinta de Mariscotti" ,
	"San Juan Bosco, presbítero" ,
	"Santa Emma, Santa Viridiana, Beato Andrés, San Raúl, Santa Alicia y Beata Ella..." ,
	"La Presentación del Señor, Santa Juana, San Cornelio, Beato Juan Teófano y Santa Catalina de Ricci" ,
	"San Blas y San Oscar" ,
	"San Gilberto, San Teófilo y Santa Juana de Francia" ,
	"Santa Agueda y San Felipe de Jesús" ,
	"San Pablo Miki, San Amando y San Gastón..." ,
	"Beata Eugenia Smet" ,
	"San Jerónimo Emiliano, Beata Jacqueline" ,
	"San Miguel Febres Cordero y Santa Apolonia" ,
	"Santa Escolástica y Beato Arnaldo Cattaneo" ,
	"Nuestra Señora de Lourdes, San Benito de Aniano y San Adolfo" ,
	"Santa Eulalia, mártir" ,
	"San Gregorio II" ,
	"San Cirilo, San Metodio, San Valentín y San Marón" ,
	"Santa Jorgina, San Faustino y San Jovita, San Claudio de la Colombière" ,
	"Santos Elias Jeremias, Isaias Samuel y Daniel, San Onésimo, Santa Juliana y San Macario" ,
	"Los Siete Santos Fundadores de la Orden de los Siervos de la Virgen María" ,
	"San Flaviano, Santa Bernardita y Beato Francisco Regis Clet" ,
	"San Conrado de Plasencia y San Álvaro de Córdoba" ,
	"San Euquerio de Orleans, Beata Amada, Beatos Francisco y Jacinta Marto" ,
	"San Pedro Damián obispo y doctor de la Iglesia" ,
	"La Cátedra de San Pedro, Santa Margarita de Cortona, Santa Isabel de Francia..." ,
	"San Policarpo, obispo y mártir" ,
	"San Vartán y compañeros" ,
	"Beato Sebastián de Aparicio, San Avertano y el Beato Romeo, San Etelberto y Santa Jacinta" ,
	"San Néstor y San Porfirio" ,
	"San Leandro, Santa Honorina y San Gabriel" ,
	"San Román y san Lupicino, Santa Antonieta y San Hilario" ,
	"Santa Eudoxia, San David y San Albino" ,
	"Beato Carlos El Bueno y San Nicolás de Flue" ,
	"Santa Katharine Drexel, San Guenolé, Santos Marino y Asterio" ,
	"San Casimiro" ,
	"San Juan José de la Cruz" ,
	"Santa María de la Providencia" ,
	"Santas Perpetua y Felicitas, San Pablo el Simple" ,
	"San Juan de Dios, Santos Filemón, Apolonio y Arieno" ,
	"Santa Francisca Romana y San Gregorio de Nisa" ,
	"Santa Anastacia la Patricia, San Juan de Mata y Mártires de Sebaste" ,
	"San Eulogio" ,
	"Santa Fina, San Inocencia I, San Maximiliano y San Pol de León" ,
	"Santos Rodrigo y Salomón, Beato Ángel de Pisa y San Humberto" ,
	"Santa Matilde" ,
	"Santa Luisa de Marillac y San Clemente María Hofbauer" ,
	"San Julián de Anazarba, San Cristódulo y Santa Benita" ,
	"San Patricio, obispo" ,
	"San Cirilo de Jerusalén, San Salvador de Horta y San Alejandro" ,
	"San José, Esposo de la Virgen María" ,
	"Santa Fotina la Samaritana, San Heriberto, San Martín Dumiense y Beato Marcel Callo" ,
	"San Nicolás de Flue" ,
	"Santa Lea" ,
	"Santo Toribio de Mogrovejo y Beata Sibila" ,
	"Santa Catalina de Suecia" ,
	"La Anunciación del Señor" ,
	"San Ludgerio" ,
	"Beato Peregrino o Pelegrino" ,
	"San Guntrano o Gontrán, rey de Borgoña" ,
	"San Eustasio" ,
	"San Juan Clímaco" ,
	"San Benjamín" ,
	"San Hugo" ,
	"San Francisco de Paula, ermitaño" ,
	"San Ricardo, Obispo" ,
	"San Isidoro obispo y doctor de la Iglesia" ,
	"San Vicente Ferrer, presbítero" ,
	"San Prudencio, San Marcelino y Beata Pierina Morosini" ,
	"San Juan Bautista de la Salle y Beato Germán" ,
	"Santa Julia Billiard, San Dionisio, San Gualterio (Walter), Beata Constanza" ,
	"Santa Casilda, San Vadim y San Lorenzo de Irlanda" ,
	"San Juan Bautista Velásquez y martires, San Fulberto, San Dimas y Beato Antonio Neyrot" ,
	"San Estanislao, obispo y mártir" ,
	"Santa Gemma Galgani, San Julio, San Zenón, San Sabas y Santa Teresa de los Andes" ,
	"San Martín I, San Hermenegildo, San Marte y Beata Ida" ,
	"Santa Lidia o Liduvina" ,
	"San Paterno De Vannes y Beato Lucio" ,
	"Santa Engracia y San Benito José Labre" ,
	"San Esteban Harding, San Aniceto, Beata Catalina Tekakwitha y Beata María De La Encarnación" ,
	"San Francisco Solano, San Perfecto y San Apolonio" ,
	"Santa Inés de Montepulciano, Santa Emma, San Expedito, San Wernerio y San León IX" ,
	"San Telmo, San Marcelino, San Teotimio, San Gerardo, Beata Odette y Santa Hildegunda" ,
	"San Anselmo y San Conrado de Parzham" ,
	"Santa María Egipciaca y Santa Oportuna" ,
	"San Jorge, San Aniano, Beato Egidio de Asís" ,
	"San Fidel de Sigmaringa, presbítero y mártir" ,
	"San Marcos y San Pedro de Betancur" ,
	"San Tarcisio, San Isidoro, San Pascasio, Beata Alida y San Riquerio" ,
	"Nuestra Señora de Montserrat, Santa Zita y San Antimio" ,
	"San Pedro Chanel, San Luis María, Santa Teodora, San Dídimo, San Vital y Santa Valeria" ,
	"Santa Catalina de Siena, virgen y doctora de la Iglesia" ,
	"San Pío V, San José Benito Cottolengo, San Roberto San Adjutorio y Beata Rosamunda" ,
	"San José Obrero, San Segismundo y Beata María Leonia Paradis" ,
	"San Atanasio, Santa Zoe, Santos Exuperio, Teódulo y Ciriaco" ,
	"La Santa Cruz y San Juvenal" ,
	"Santos Felipe y Santiago, San Gotardo, San Florián y San Silvano" ,
	"San Antonino, San Hilario de Arles, San Ángel y Santa Judith" ,
	"Santo Domingo Savio, San Evodio, San Mariano y Beata Prudencia" ,
	"Santa Flavia Domitila y Beata Gisela" ,
	"Job, San Pedro de Tarantasia, San Desiderio y Beata María Droste zu Vischering" ,
	"Santa María Mazzarelllo y San Gregorio Ostiense" ,
	"San Juan de Ávila y Santa Solange" ,
	"Santa Juana de Arco, San Mamerto, Santa Estrella, San Mayolo, San Francisco de Jerónimo..." ,
	"Santos Nereo, San Aquileo, San Pancracio, Santo Domingo, San Epifanio, San Felipe, San Germán..." ,
	"Nuestra Señora de Fátima, San Andrés F., San Servasio, Santa Rolanda, Santa Magdalena y Santa Inés" ,
	"San Matías, San Pacomio, San Miguel Garicoïts, Santa Aglae y San Bonifacio, San Isidoro, San Poncio" ,
	"San Isidro Labrador, Santa Dionisia, Santos Pablo y Andrés, San Victorino, San Aquileo, Santa Juana" ,
	"San Juan Nepomuceno, San Andrés Bóbola, San Simón Stock, San Ubaldo, Santos Alipio y Posidio" ,
	"San Pascual Bailón, Beata Antonia Mesina y Beato Pedro Ouen-Yen" ,
	"San Juan I, San Leonardo Murialdo, San Félix, Santa Rafaela, Santa María Josefa, Beata Blandina..." ,
	"San Celestino V, San Dunstan, San Ivón, San Urbano I, Beata María Bernarda, Beato Francisco Coll" ,
	"San Bernardino de Siena, San Protasio Chong Kurbo y Beata Columba de Rieti" ,
	"Santa Gisela, San Eugenio de Mazenod, Beato Jacinto María Cormier y Mártires de México" ,
	"Santa Rita de Casia, Santa Julia , San Lorenzo Ngon, Beato Juan Forest y Beata María Dominica" ,
	"San Juan Bautista de Rossi y Santa Juana Antida Thouret" ,
	"María Auxiliadora , San Donaciano, San Rogaciano y Mártires Coreanos" ,
	"San Beda, Santos Cristóbal Magallanes y Agustín Caloca, San Gregorio VII, Santa María Magdalena..." ,
	"San Felipe Neri, Santa Mariana, San José Chang, San Juan Doa y Mateo Nguyên, San Pedro Mártir Sans" ,
	"San Agustín de Canterbury, San Atanasio Bazzekuketta, Santa Bárbara Kim y Bárbara Yi, San Gonzalo" ,
	"Santa Ripsimena, San Germán de París, San Pablo Han y Beata Margarita Pole" ,
	"Santos Sisinio, Martorio y Alejandro, Beato Ricardo Thirkeld, Beata Úrsula Ledochowska" ,
	"San Fernando, Beatos Lucas Kirby, Guillermo Filby, Lorenzo Richardson, Tomás Cottam,Beato Matías..." ,
	"La Visitación de la Santísima Virgen María, Santa Petronila, Beatos Félix, Roberto, Tomás y Nicolás" ,
	"San Justino, San Pánfilo, San Renán, San Caprasio, Beato Aníbal y Beato Juan Bautista Scalabrini" ,
	"Santos Marcelino y Pedro, Santa Blandina, San Potino y Santo Domingo Ninh" ,
	"San Carlos Lwanga , Beato Juan XXIII, Santa Mariana de Jesús, San Kevin y San Pablo Duong" ,
	"Santa Clotilde, San Ascanio Caracciolo, Santa Vicenta Gerosa y Beato Felipe Smaldone" ,
	"San Bonifacio, Santo Domingo Toái y Santo Domingo Huyen" ,
	"San Norberto, San Marcelino Champagnat, Santos Pedro Dung, Pedro Thuan , Vicente Duong, Beato Rafael" ,
	"San Gilberto, San Pedro de Córdoba y Beata María Teresa de Soubiran" ,
	"San Medardo, Beata María del Divino Corazón y Beato Santiago Barthieu" ,
	"San Efrén, San Columba, Beato José de Anchieta y Beata Diana Dandalo" ,
	"Santa Margarita de Escocia, Santa Olivia y Beata Ana María Taigi" ,
	"San Bernabé, Santa Alicia, Santa María Rosa Molas, Santa Paula Frassinetti y Beata María Schininá" ,
	"San Juan de Sahagún, San Onofre, San Gaspar Bortoni, Beata Mercedes de Jesús, Beato Guido" ,
	"San Antonio de Padua, San Agustín Phan Viet Huy y San Nicolás Bui Duc The" ,
	"San Metodio, San Valero, San Rufino y Eliseo" ,
	"Santa Germana, San Vito, Santa María Micaela, Santa Bárbara, Beato Luis María, Beata Yolanda" ,
	"San Juan Francisco, San Ciro, San Aureliano, Santa Ma. Teresa, Santo Domingo Nguyen y compañeros" ,
	"San Gregorio Barbarigo, San Rainiero, San Heberto y San Pedro Da" ,
	"Santa Isabel de Shönau, Santa Juliana y Beata Osanna" ,
	"San Romualdo, Santos Gervasio y Protasio, San Remigio Isoré, San Modesto, Beata Miguelina Metelli" ,
	"San Silverio y Santa Florentina" ,
	"San Luis Gonzaga, San José Isabel Flores, San Anton María Schwartz, San Jacobo Kern y Santa Restitua" ,
	"Santo Tomás Moro, San Paulino De Nola y San Juan Fisher" ,
	"San Gaspar de Búfalo, San José Cafasso, Beato Bernhard Lichtenberg y Beata María Rafaela" ,
	"La Natividad de San Juan Bautista y San Juan Yuan Zaide" ,
	"San Guillermo De Vercelli, San Próspero, Santa Leonor y San Salomón" ,
	"San Josemaría Escrivá de Balaguer, San José María Robles, y San José María Ma-Tai-Shun" ,
	"Nuestra Señora del Perpetuo Socorro, San Cirilo, San Ladislao, San José Hien, Santo Tomás Toan..." ,
	"San Ireneo, San Jerónimo Lu Tingmey, Santa María Tou-Tchao-Cheu, Santa Vicenta Gerosa..." ,
	"San Pedro y San Pablo, Santos María Tian de Du, Magdalena Du Fengju, Pablo Wu Kiunan..." ,
	"Los Primeros Santos Mártires, San Raimundo y San Pedro Li Quanzhu, San Vicente Dô Yên" ,
	"San Simeón, San Teodorico, San Servando, Santos Justino Orona y Atilano Cruz , Beato Junípero..." ,
	"San Otón de Bamberg, San Martiniano, San Procesio y Beata Eugenia Joubert" ,
	"Santo Tomás, San Felipe Phan, San José Nguyen, Santos Pedro y Juan Bautista Zhao, Beato Raimundo..." ,
	"Nuestra Señora del Refugio, Santa Isabel de Portugal, Santa Berta, San Valentín de Berrio-Ochoa..." ,
	"San Antonio María, San Miguel de los Santos,Santas Teresa Jinjie y Rosa Chen Anjie, Beato Elías" ,
	"Santa María Goretti, San Pedro Wang Zuolong, Beata Nazaria Ignacia y Beata María Teresa Ledóchowska" ,
	"Santos Raúl Milner y Rogelio Dickenson, San Vilibaldo, San Fermín, San Marcos Ji Tianxiang..." ,
	"San Edgar, San Procopio, San Kilián, San Teobaldo, Isaías, San Juan Wu" ,
	"Santa Verónica, Mártires de Orange, Santos Teodorico y Nicasio, San Gregorio Grassi, San Joaquín Ho" ,
	"San Cristóbal, Santos Antonio Nguyen Quynh y Pedro Nguyen Khac, Beato Pacífico y Mártires de Damasco" ,
	"San Benito, Santa Olga, Santas Ana Xin de An, María Guo de An, Ana Jiao de An y María An Linghua" ,
	"San Juan Gualberto, San Ignacio-Clemente, Santa Inés Le Thi, San Pedro Khan , Beato Oliverio Plunket" ,
	"San Enrique, San Eugenio de Cartago, Profeta Joel , Santa Mildred y Beato Carlos Manuel Rodríguez" ,
	"San Camilo De Lelis, San Francisco Solano, San Juan Wang, Beato Guillermo Repin y Beato Ghebre" ,
	"San Buenaventura, San Donald, San Andrés Nam-Thuong, San Pedro Tuan, Beata Ana María, Beato Pedro" ,
	"Nuestra Señora del Carmen, Santa María Magdalena Postel, San Milón, Santos Lang Yang y Pablo Lang" ,
	"San Alejo, Santa Marcelina, Las Beatas Carmelitas de Compiègne y San Pedro Liu Ziyn" ,
	"San Federico, San Arnulfo de Metz, Santa Marina y Santo Domingo Dinh Dat" ,
	"San Arsenio, Santa Áurea, Santas Justa y Rufina, Santa Isabel Blan de Qin y San Simón Qin" ,
	"San Aurelio, Santa Margarita, Profeta Elías, San José María Díaz Sanjurjo, San León Ignacio Mangin" ,
	"San Lorenzo de Brindisi, San Víctor, San Alberico Crescitelli y San José Wang-Yumel" ,
	"Santa María Magdalena , San Vandrilio, Santas Ana Wang, Lucía Wang-Wang, María Wang y San Andrés W." ,
	"Santa Brígida, San Apolinar, Beatos Manuel Pérez, Nicéforo, Vicente Díaz, Pedro Ruiz y compañeros" ,
	"Santa Verónica Giuliani, Mártires de Guadalajara, Santa Cristina, San José Fernández" ,
	"Santiago Apóstol, Beatos Dionisio, Federico, Primo, Jerónimo, Juan de la Cruz, María, Pedro, Félix.." ,
	"San Joaquín y Santa Ana, Santa Bartolomea Capitanio y Beato Jorge Preca" ,
	"Santa Natalia, San Aurelio, Beato Sebastián de Aparicio y Beato Tito Brandsman" ,
	"Santa María Josefa Rosello, San Melchor García Sampedro, Beato Pedro Poveda y Beata Alfonsa" ,
	"Santa Marta, San Olaf , San Lupo de Troyes, Santa Julia, San José Zhang Wenlan..." ,
	"San Pedro Crisólogo, San Germán de Auxerre, San Leopoldo, Santa María de Jesús Sacramentado Venegas" ,
	"San Ignacio de Loyola, Santos Pedro Doan Cong Quy y Manuel Phung" ,
	"San Alfonso María de Ligorio, San Bernardo Vu Van Duê, Santo Domingo Hanh y Beata María Estela" ,
	"San Eusebio de Vercelli y Beato Francisco Calvo" ,
	"San Pedro Julián Eymard, Santa Lidia y Santa Juana de Chantal" ,
	"San Juan María Vianney, San Aristaco, Beato Federico Janssoon, Beato Gonzalo Gonzalo" ,
	"La Dedicación de la Basílica de Santa María la Mayor, San Abel, Santa Afra y San Osvaldo" ,
	"La Transfiguración del Señor, Santos Justo y Pastor, Beato Octaviano y Beata Ma.Francisca de Jesús" ,
	"San Sixto II, San Cayetano, San Donato y San Miguel de la Mora" ,
	"Santo Domingo, San Ciríaco, San Pablo Ge Tingzhu, Beata María de la Cruz y Beata María Margarita" ,
	"Santa Teresa Benedicta de la Cruz, San Oswaldo, Santa Otilia, Santos Juliano, Marciano, Fotio..." ,
	"San Lorenzo, Diácono y Mártir" ,
	"Santa Clara, San Géry, Santa Susana y Beato Mauricio Tornay" ,
	"San Benilde Romançon, Santa Hilaria, San Eleazar, Santos Antonio Pedro, Santiago Do Mai Nam..." ,
	"San Ponciano, San Hipólito, San Estanislao de Kostka, San Juan Berchmans, San Benildo..." ,
	"San Maximiliano María Kolbe, Santa Atanasia, Beato Everardo, Beata Isabel Renzi" ,
	"Asunción de la Santísima Virgen María, San Arnulfo, San Tarsicio, San Luis Batis Sáinz..." ,
	"San Esteban de Hungría, Beato Bartolomé Laurel, San Roque..." ,
	"San Jacinto de Cracovia, San Liberato y San Agapito" ,
	"Santa Elena" ,
	"San Juan Eudes, Beatos Pedro Zuñiga y Luis Flores" ,
	"San Bernardo, San Filiberto y Profeta Samuel" ,
	"San Pío X, San Sidonio Apolinar, Santos Cristóbal y Leovigildo..." ,
	"Nuestra Señora María Reina, San Sigfrido y San Sinforiano" ,
	"San Felipe Benicio" ,
	"San Bartolomé y San Audoeno" ,
	"San Luis, San José de Calasanz, San Ginés, Beato Tomás de Kempis, Beato Luis Urbano Lanaspa" ,
	"San Cesáreo, Santa Teresa de Jesús Jornet e Ibars y Beato Junípero Serra" ,
	"Santa Mónica, San Guerín y San Amadeo" ,
	"San Agustín, San Hermes, San Moisés el Etíope, Beatos Constantino Fernández y Francisco Monzón" ,
	"El Martirio de San Juan Bautista , Beato Edmundo Ignacio Rice y Beata Teresa Bracco" ,
	"Santa Rosa de Lima, San Fiacre, Beato Alfredo Ildefonso, Beatos Diego Ventaja y Manuel Medina" ,
	"San Ramón Nonato, San Arístides, Beatos Emigdio, Amalio y Valerio Bernardo" ,
	"San Gil" ,
	"Beato Bartolomé Gutierrez" ,
	"San Gregorio Magno, Beatos Juan Pak, María Pak, Bárbara Kouen, Bárbara Ni, María Ni e Inés Kim" ,
	"Santa Rosalía, Santa Irma y San Marino" ,
	"San Lorenzo Justiniano, Santa Raisa, Santos Pedro Nguyen Van Tu y José Hoang Luong Canh" ,
	"Santa Eva de Dreux, San Magno, San Beltrán, San Eleuterio, Beato Contardo Ferrini" ,
	"Santa Regina, San Columbano y Beato Juan Bautista Mazzuconi" ,
	"Natividad de la Santísima Virgen María y Beato Federico Ozanam" ,
	"San Pedro Claver, San Audemaro, San Auberto, Beato Santiago Laval, Beata Serafina, Beato Alano..." ,
	"San Nicolás de Tolentino y Beato Francisco Garate" ,
	"San Pafnucio, San Adelfo, Santa Vinciana y San Juan Gabriel Perboyre" ,
	"San Guido, Beata Victoria Fornari y Beato Apolinar Franco" ,
	"San Juan Crisóstomo y San Amado" ,
	"Exaltación de la Santa Cruz, San Materno, Santa Notburga y Beato Gabriel Taurino Dufresse" ,
	"Nuestra Señora de los Dolores y Beato Rolando" ,
	"San Cornelio y Cipriano, Santa Edith, Santa Ludmila, Beatos Juan Bautista y Jacinto de los Ángeles" ,
	"San Roberto Belarmino, San Lamberto y Santa Hildegarda" ,
	"San José de Cupertino, Santa Ricarda, San Juan Macías, Santo Domingo Doà Trach..." ,
	"San José Ma. de Yermo y Parrés, San Jenaro, Santa María de Cervelló, Santa Emilia de Rodat" ,
	"San Andrés Kim Taegon, San Pablo Chong Hasang, San Pedro de Arbués y San Juan Carlos Cornay" ,
	"San Mateo, Apóstol y Evangelista" ,
	"San Mauricio, Santo Tomás de Villanueva,San Carlos Navarro Miquel, Beato José Aparicio Sanz..." ,
	"San Constancio, San Pío de Pietrelcina, Santa Tecla, San Andrés Fournet, Beatos Cristobal, Antonio.." ,
	"Nuestra Señora de la Merced" ,
	"San Fermín, San Carlos de Sezze, Beato Germán y Beato José Benito Dusmet" ,
	"Santos Cosme y Damián, San Nilo, San Isaac Jogues, San Sebastián Nam, Santa Teresa Couderc..." ,
	"San Vicente de Paúl, presbítero" ,
	"San Wenceslao, San Lorenzo Ruiz, San Fausto, San Exuperio y Beato Francisco Castelló Aleu" ,
	"Los Santos Arcángeles Miguel, Gabriel y Rafael, San Alarico" ,
	"San Jerónimo, Beato Conrado de Urach y Beato Federico Albert" ,
	"Santa Teresa del Niño Jesús" ,
	"Los Santos Ángeles Custodios y San Leodegario" ,
	"San Francisco De Borja, Santos Evaldo el Moreno y Evaldo el Rubio..." ,
	"San Francisco de Asís, Santa Aurea y Beato Michel Callo" ,
	"San Mauro y San Plácido, Santa Flora, Santa Faustina Kowalska, Beato Bartolomé Longo..." ,
	"San Bruno, Santa Fe, Beata Marie Rose Durocher, Beata María Ana Mogas, Beato Isidoro de Loor.." ,
	"Nuestra Señora del Rosario, San Artaldo, San Augusto, San Sergio" ,
	"Santa Pelagia, San Juan Calabria, Santa Thais y Santa Edwiges" ,
	"San Dionisio, San Juan Leonardi, San Luis Beltrán, Santos Inocencio, Jaime Hilario..." ,
	"Santo Tomás De Villanueva y San Virgilio" ,
	"Santa Soledad Torres Acosta, San Fermín, San Pedro Le Tuy, Beato Juan XXIII y Beata María de Jesús" ,
	"Nuestra Señora del Pilar, San Serafín, San Wilfrido..." ,
	"San Geraldo, San Teófilo de Antioquía y San Eduardo" ,
	"San Calixto I, Papa y mártir" ,
	"Santa Teresa de Jesús, virgen y doctora de la Iglesia" ,
	"Santa Eduviges, Santa Margarita María Alacoque, San Beltrán, San Galo, Beata Margarita de Youville" ,
	"San Ignacio de Antioquía, obispo y mártir" ,
	"San Lucas, Evangelista" ,
 	"San Juan de Brébeuf, San Isaac Jogues , San Lucas del Espíritu Santo, San Pablo de la Cruz..." ,
	"Santa Irene, Santa Bertilia, Beata Adelina y Beato Contardo Ferrini" ,
	"San Hilarión, Santa Úrsula , Santa Celina, San Gerardo María Mayela y San Antonio María Gianelli" ,
	"San Felipe De Heraclea, Santas Elodia y Nunilón, Beato Timoteo Giaccardo" ,
	"San Juan de Capistrano, San Servando, San Germán, San Pablo Tông Viêt Buong, Beato Arnoldo Reche" ,
	"San Antonio María Claret, San José Lê Dang Thi, Beato Rafael Guízar, Beato Luis Guanella..." ,
	"Santos Crispín y Crispiniano, Cuarenta Mártires de Inglaterra y Gales..." ,
	"Beata Bona De Armagnac" ,
	"San Frumencio, San Demetrio Bassarabov, Santo Dominguito.." ,
	"Santos Simón y Judas, San Rodrigo Aguilar Alemán" ,
	"San Narciso, San Marcelo el Centurión, Santa Ermelinda y Beato Miguel Rúa" ,
	"Santa Dorotea Swartz, San Alonso Rodríguez y Beata Bienvenida" ,
	"Todos los Santos" ,
	"Todos los fieles difuntos" ,
	"San Martín de Porres, San Huberto, San Guénael , Santa Silvia, San Pedro José Almató..." ,
	"San Carlos Borromeo, San Emérico, San Amancio y San Jesse" ,
	"San Zacarías y Santa Isabel, San Celestino, San Bertila, Beato Guido Ma. Conforti..." ,
	"San Leonardo, San Severo, San Alejandro, San José Kong y Beata Josefa Naval Girbés" ,
	"San Ernesto, San Engelberto y Beato Francisco Palau y Quer" ,
	"Beato Juan Duns Scot, San Godofredo, San Andrés Avelino, Beata Isabel de la Trinidad..." ,
	"La Dedicación de la Basílica de Letrán" ,
	"San León Magno, San Baudilio y Santa Natalena" ,
	"San Martín de Tours, obispo" ,
	"San Josafat, San Emiliano, San Margarito Flores y Santa Agustina Livia Petrantoni" ,
	"San Diego de Alcalá, San Leandro, San Eugenio, San Homobono, San Estanislao de Koska..." ,
	"San Serapión, San Esteban Teodoro Cuenot y Beata Magdalena Morano" ,
	"San Alberto Magno, San José Pignatelli, San Malo..." ,
	"Santa Gertrudis, San Omar, San José Moscati y Beato Aurelio María" ,
	"Santa Isabel de Hungría, Santa Hilda y San Gregorio de Tours" ,
	"La Dedicación de las Basílicas de San Pedro y San Pablo, San Odón, Santa Filipina Rosa Duchesne..." ,
	"San Tanguy y San Rafael Kalinoswki de San José" ,
	"San Edmundo, San Roque de Santa Cruz, Santos Octavo, Adventorio y Solutorio..." ,
	"La Presentación de la Santísima Virgen María, San Gelasio, B. Ma. del Buen Pastor" ,
	"Santa Cecilia y San Filemón" ,
	"Beato Miguel Agustin Pro, San Clemente I, San Columbano..." ,
	"San Andrés Dung-Lac, Santas Flora y María y otros" ,
	"Santa Catalina Labouré" ,
	"San Leonardo De Puerto Mauricio, Beata Delfina, San Eleázar..." ,
	"San Máximo, San Severino y San Virgilio" ,
	"San Hilario, Santa Quieta, San Santiago de la Marca y San Andrés Trân Van Trông" ,
	"San Gregorio Taumaturgo, San Saturnino, San Radbodo..." ,
	"San Andrés, San Zósimo y San José Marchand" ,
	"San Eloy, Santa Florencia, San Edmundo Campion ..." ,
	"Santa Bibiana (Viviana)" ,
	"San Francisco Javier y San Galgano Guidotti" ,
	"San Juan Damasceno, Santa Bárbara, San Osmondo y B. Adolfo Colping" ,
	"San Sabás, Santa Atalía y San Geraldo" ,
	"San Nicolás, Santa Dionisia y compañeros mártires, Beata Carmen Sallés" ,
	"San Ambrosio" ,
	"La Inmaculada Concepción De La Virgen María" ,
	"San Pedro Fourier, San Juan Diego y Beato Bernardino Silvestrelli" ,
	"La Virgen De Loreto, San Romarico y Santa Valeria" ,
	"San Dámaso I, San Daniel y Beata Maravillas de Jesús" ,
	"Nuestra Señora de Guadalupe" ,
	"Santa Lucía y Santa Odilia" ,
	"San Juan de la Cruz y San Fortunato" ,
	"Santa Cristiana (Nina), Santa Maria de la Rosa y Beato Carlos Steeb" ,
	"Santa Adelaida (o Alicia) y San Everardo" ,
	"Santa Olimpia y San Lázaro" ,
	"San Flavio, San Winebaldo y San Modesto" ,
	"Santa Sametana y Beato Urbano V" ,
	"Santo Domingo De Silos" ,
	"San Pedro Canisio, presbítero y doctor de la Iglesia" ,
	"Santa Francisca Javiera Cabrini y Beato Graciano" ,
	"San Juan Cancio, San Thorlakur y Beato Armando" ,
	"San Charbel Majluf, San Gregorio, San Justo y San Viator..." ,
	"Natividad De Nuestro Señor Jesucristo" ,
	"San Esteban y San Nicomedes" ,
	"San Juan y Santa Fabiola" ,
	"Los Santos Inocentes y San Gaspar del Búfalo" ,
	"Santo Tomás Becket, obispo y mártir" ,
	"La Sagrada Familia de Jesús, María y José" ,
	"San Silvestre I, Santa Melania , Santa Colomba y San Mario" 
};
Opts horos[] = {
	{ 21 , "Capricornio" } ,
	{ 21 , "Acuario" } ,
	{ 20 , "Piscis" } ,
	{ 20 , "Aries" } ,
	{ 20 , "Tauro" } ,
	{ 21 , "Géminis" } ,
	{ 22 , "Cáncer" } ,
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
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}
	//mod->activo = 1;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(NoteServ).nombre);
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
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(NoteServ).nombre);
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
		Responde(cl, CLI(noteserv), ES_ERR_PARA, fc->com, "[dd/mm/yy [hh:mm]] [nº] nota");
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
	Responde(cl, CLI(noteserv), "Se ha añadido esta entrada correctamente.");
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
		Responde(cl, CLI(noteserv), ES_ERR_SNTX, "Formato de fecha incorrecto: dia/mes/año");
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
		Responde(cl, CLI(noteserv), "Todas las entradas para el día \00312%s\003 han sido eliminadas.", param[1]);
	}
	else if (params == 3)
	{
		SQLQuery("DELETE FROM %s%s WHERE fecha = %lu AND item='%s'", PREFIJO, ES_SQL, tt, cl->nombre);
		Responde(cl, CLI(noteserv), "Todas las entradas para el día \00312%s\003 a las \00312%s\003 han sido eliminadas.", param[1], param[2]);
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
	else if (!strcasecmp(param[1], "MAÑANA"))
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
		Responde(cl, CLI(noteserv), ES_ERR_SNTX, "Formato de fecha incorrecto: día/mes/año|HOY|MAÑANA");
		return 1;
	}
	if (!(res = SQLQuery("SELECT * FROM %s%s WHERE fecha >= %lu AND fecha < %lu AND item='%s'", PREFIJO, ES_SQL, tt, tt+86400, cl->nombre)))
	{
		Responde(cl, CLI(noteserv), ES_ERR_EMPT, "No existen entradas para este día.");
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
