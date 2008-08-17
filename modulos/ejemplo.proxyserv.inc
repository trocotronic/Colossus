ProxyServ {
	/*
	 * Tiempo que dura el GLINE cuando se detecta que es proxy.
	 * Valor: Número entero positivo que expresa minutos.
	 */
	tiempo "minutos";
	
	/*
	 * Máximo de entradas a mostrar cuando se listen los hosts permitidos mediante el comando HOST. Se utiliza para no floodear.
	 * Valor: Número entero positivo mayor que cero.
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
		 * Gestiona los hosts que están exentos del escáner.
		 * Nivel por defecto: 4.
		 * Comando por defecto: HOST.
		 */
		host;
	};
	
	/*
	 * Puertos a escanear y, si se prefiere, verificar un protocolo.
	 * Si no se especifica un protocolo, sólo dará positivo en caso de que el puerto esté abierto.
	 * Si se especifica un protocolo, se mirará que, además de que esté abierto, también responda al protocolo especificado.
	 * Protocolos soportados: 
	 * 	- HTTP: Protocolo Proxy estándar vía CONNECT. Es el más común.
	 *	- SOCKS4
	 *	- SOCKS5
	 * 	- ROUTER: Routers pirateados.
	 *	- WINGATE
	 *	- POST: Protocolo Proxy vía POST.
	 *	- GET: Protocolo Proxy vía GET.
	 * En caso de que algún dé positivo, el usuario será GLINEado.
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
	 * Esta IP es la que se pide al proxy que conecte. Generalmente, será una IP que conecte a un servidor IRC.
	 * Si usas la IP "127.0.0.1", Colossus detectará tu IP pública y la usará con tal efecto. Este caso se usa si el servidor IRC y Colossus corren bajo la misma máquina.
	 */
	scan_ip "83.35.165.4";
	
	/*
	 * Puerto que se pide al proxy que conecte. Generalmente, será un puerto que tenga un servicio IRC.
	 */
	scan_puerto "6667";
	
	/*
	 * Patrón de búsqueda que indicará que realmente se trata de un proxy o no.
	 * Este patrón contiene una cadena que se muestra al conectar al servidor IRC.
	 * Pueden añadirse hasta 15 patrones.
	 */
	patron "Buscando tu host";
	patron "Looking up your hostname";
	
	/*
	 * Con este parámetro activo, la razón del GLINE en caso de que dé positivo contendrá los puertos que han sido detectados como proxy junto a su protocolo.
	 */
	quits_detallados;
	
	/*
	 * Con este parámetro activo, se mirará si la IP que se conecta es un proxy o no mediante el sistema OPM.
	 */
	lista_online;
	
	/*
	 * Con este parámetro activo, se forzará la desconexión del proxy nada más detectarlo. Acelera el proceso pero aporta menos información. 
	 * Es útil si está el parámetro quits_detallados desactivado.
	 */
	forzar_salida;
};

/*
 * Recuerde que puede utilizar los bloques funcion {} o alias { } para personalizar estos comandos.
 *
 * www.redyc.com
 */
 