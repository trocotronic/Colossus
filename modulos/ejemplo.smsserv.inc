SmsServ {
	/*
	 * Minutos que debe esperar un usuario antes de que pueda mandar otro SMS
	 */
	espera 5;
	
	/*
	 * Con este par�metro activo, s�lo se pondr�n mandar SMS entre nicks. Es decir, el comando ser� exclusivo para usuarios de la red.
	 * No se podr�n mandar SMS especificando el n�mero de destino, s�lo el nick.
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
		 * Env�a un SMS
		 * Nivel por defecto: 1.
		 * Comando por defecto: SEND.
		 */
		send;
		
		/*
		 * Permite fijar las opciones como el n�mero de tel�fono asociado a cada nick.
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
	 * ID de usuario �nica e intransferible que se te da al darte de alta en www.redyc.com
	 */
	id "tu-id";
};

/*
 * Recuerde que puede utilizar los bloques funcion {} o alias { } para personalizar estos comandos.
 *
 * www.redyc.com
 */
 