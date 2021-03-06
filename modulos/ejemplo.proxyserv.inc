ProxyServ {
	/*
	 * Tiempo que dura el GLINE cuando se detecta que es proxy.
	 * Valor: N�mero entero positivo que expresa minutos.
	 */
	tiempo "minutos";

	/*
	 * M�ximo de entradas a mostrar cuando se listen los hosts permitidos mediante el comando HOST. Se utiliza para no floodear.
	 * Valor: N�mero entero positivo mayor que cero.
	 */
	maxlist "30";

	funciones {

		/*
		 * Muestra la ayuda.
		 * Nivel por defecto: 4.
		 * Comando por defecto: HELP.
		 */
		help;

		/*
		 * Gestiona los hosts que est�n exentos del esc�ner.
		 * Nivel por defecto: 4.
		 * Comando por defecto: HOST.
		 */
		host;
	};

	/*
	 * Puertos a escanear y, si se prefiere, verificar un protocolo.
	 * Si no se especifica un protocolo, s�lo dar� positivo en caso de que el puerto est� abierto.
	 * Si se especifica un protocolo, se mirar� que, adem�s de que est� abierto, tambi�n responda al protocolo especificado.
	 * Protocolos soportados:
	 * 	- HTTP: Protocolo Proxy est�ndar v�a CONNECT. Es el m�s com�n.
	 *	- SOCKS4
	 *	- SOCKS5
	 * 	- ROUTER: Routers pirateados.
	 *	- WINGATE
	 *	- POST: Protocolo Proxy v�a POST.
	 *	- GET: Protocolo Proxy v�a GET.
	 * En caso de que alg�n d� positivo, el usuario ser� GLINEado.
	 * NOTA: No abusar de los puertos, puesto que ralentiza el programa notablemente.
	 */
	puertos {
		1080 SOCKS4,SOCKS5;
		8080 HTTP,GET;
		3128 HTTP,GET;
		81 HTTP,POST,GET;
		80 HTTP,POST,GET;
		23 ROUTER,WINGATE;
	};

	/*
	 * Archivo que contiene una lista de IPs consideradas proxy.
	 * Cada IP debe ir separada de la siguiente por un INTRO (\r\n).
	 */
	lista "proxys.txt";

	/*
	 * Esta IP es la que se pide al proxy que conecte. Generalmente, ser� una IP que conecte a un servidor IRC.
	 * Si usas la IP "127.0.0.1", Colossus detectar� tu IP p�blica y la usar� con tal efecto. Este caso se usa si el servidor IRC y Colossus corren bajo la misma m�quina.
	 */
	scan_ip "83.35.165.4";

	/*
	 * Puerto que se pide al proxy que conecte. Generalmente, ser� un puerto que tenga un servicio IRC.
	 */
	scan_puerto "6667";

	/*
	 * Patr�n de b�squeda que indicar� que realmente se trata de un proxy o no.
	 * Este patr�n contiene una cadena que se muestra al conectar al servidor IRC.
	 * Pueden a�adirse hasta 15 patrones.
	 */
	patron "Buscando tu host";
	patron "Looking up your hostname";

	/*
	 * Con este par�metro activo, la raz�n del GLINE en caso de que d� positivo contendr� los puertos que han sido detectados como proxy junto a su protocolo.
	 */
	quits_detallados;

	/*
	 * Con este par�metro activo, se mirar� si la IP que se conecta es un proxy o no mediante el sistema OPM.
	 */
	lista_online;

	/*
	 * Ignora a todos aquellos nicks que son IRCops.
	 */
	ignorar_opers;

	/*
	 * Ignora a todos aquellos nicks que est�n registrados e identificados.
	 */
	ignorar_nicks_id;
};

/*
 * Recuerde que puede utilizar los bloques funcion {} o alias { } para personalizar estos comandos.
 *
 * www.redyc.com
 */
