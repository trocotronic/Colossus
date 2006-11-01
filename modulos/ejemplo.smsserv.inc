SmsServ {
	/*
	 * Minutos que debe esperar un usuario antes de que pueda mandar otro SMS
	 */
	espera 5;
	
	/*
	 * Con este parámetro activo, sólo se pondrán mandar SMS entre nicks. Es decir, el comando será exclusivo para usuarios de la red.
	 * No se podrán mandar SMS especificando el número de destino, sólo el nick.
	 * Operadores exentos.
	 */
	restringido;
	
	funciones {
		
		/*
		 * Muestra la ayuda.
		 * Nivel por defecto: 1.
		 * Comando por defecto: HELP.
		 */
		help;
		
		/*
		 * Envía un SMS
		 * Nivel por defecto: 1.
		 * Comando por defecto: SEND.
		 */
		send;
		
		/*
		 * Permite fijar las opciones como el número de teléfono asociado a cada nick.
		 * Nivel por defecto: 1.
		 * Comando por defecto: SET.
		 */
		set;
		
		/*
		 * Permite gestionar la lista de usuarios que pueden o no pueden enviar SMS a cada nick.
		 * Nivel por defecto: 1.
		 * Comando por defecto: LISTA.
		 */
		lista;
		
		/*
		 * Muestra el saldo que le queda a la red.
		 * Nivel por defecto: 4.
		 * Comando por defecto: SALDO.
		 */
		saldo;
	};
	funcion send {
		nivel OPER;
	};
	
	/*
	 * ID de usuario única e intransferible que se te da al darte de alta en www.redyc.com
	 */
	id "tu-id";
};

/*
 * Recuerde que puede utilizar los bloques funcion {} o alias { } para personalizar estos comandos.
 *
 * www.redyc.com
 */
 