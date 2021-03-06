StatServ {
	funciones {
		
		/*
		 * Obtiene la versi�n de los usuarios. NO QUITAR.
		 */
		VERSION;

		/*
		 * Muestra la ayuda.
		 * Nivel por defecto: 1.
		 * Comando por defecto: HELP.
		 */
		HELP;

		/*
		 * Muestra las estad�sticas actuales
		 * Nivel por defecto: 2.
		 * Comando por defecto: STATS.
		 */
		STATS;
	};

	/*
	 * Refresco de las estad�sticas en segundos.
	 * No ponga un valor muy peque�o puesto que consumir� recursos frecuentemente.
	 */
	refresco "60";

	/*
	 * Los distintos templates que debe cargar. NO LOS MODIFIQUE SI NO EST� SEGURO DE ELLO
	 */
	template "html/StatServ/stats.global.users.php.tpl";
	template "html/StatServ/stats.global.chans.php.tpl";
	template "html/StatServ/stats.global.opers.php.tpl";
	template "html/StatServ/stats.global.servers.php.tpl";
	template "html/StatServ/stats.versions.php.tpl";
	template "html/StatServ/stats.tlds.php.tpl";
	template "html/StatServ/stats.html.tpl";
};


/*
 * Recuerde que puede utilizar los bloques funcion {} o alias { } para personalizar estos comandos.
 *
 * www.redyc.com
 */
 