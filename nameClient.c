

/*
*  C Implementation: nameClient
*
* Description: 
*
*
* Author: MCarmen de Toro <mc@mc>, (C) 2015
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include "nameClient.h"

/**
 * Function that sets the field addr->sin_addr.s_addr from a host name 
 * address.
 * @param addr struct where to set the address.
 * @param host the host name to be converted
 * @return -1 if there has been a problem during the conversion process.
 */
int setaddrbyname(struct sockaddr_in *addr, char *host)
{
  struct addrinfo hints, *res;
	int status;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM; 
 
  if ((status = getaddrinfo(host, NULL, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }
  
  addr->sin_addr.s_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
  
  freeaddrinfo(res);
    
  return 0;  
}

/**
 * Function that gets the dns_file name and port options from the program 
 * execution.
 * @param argc the number of execution parameters
 * @param argv the execution parameters
 * @param reference parameter to set the host name.
 * @param reference parameter to set the port. If no port is specified 
 * the DEFAULT_PORT is returned.
 */
int getProgramOptions(int argc, char* argv[], char *host, int *_port)
{
  int param;
  *_port = DEFAULT_PORT;

  // We process the application execution parameters.
	while((param = getopt(argc, argv, "h:p:")) != -1){
		switch((char) param){		
			case 'h':
				strcpy(host, optarg);				
				break;
			case 'p':
				// Donat que hem inicialitzat amb valor DEFAULT_PORT (veure common.h) 
				// la variable port, aquest codi nomes canvia el valor de port en cas
				// que haguem especificat un port diferent amb la opcio -p
				*_port = atoi(optarg);
				break;				
			default:
				printf("Parametre %c desconegut\n\n", (char) param);
				return -1;
		}
	}
	
	return 0;
}

/**
 * Shows the menu options. 
 */
void printa_menu()
{
		// Mostrem un menu perque l'usuari pugui triar quina opcio fer

		printf("\nAplicatiu per la gestió d'un DNS Server\n");
		printf("  0. Hola mon!\n");
		printf("  1. Llistat dominis\n");
		printf("  2. Consulta domini\n");
		printf("  3. Alta Ip\n");
		printf("  4. Alta Ips\n");
		printf("  5. Modificacio Ip\n");
		printf("  6. Baixa Ip\n");
		printf("  7. Baixa Domini\n");
		printf("  8. Sortir\n\n");
		printf("Escolliu una opcio: ");
}

void process_hello(int s) //0
{
	char msg[MAX_BUFF_SIZE] = "";
	char opcode[MAX_BUFF_SIZE];
	stshort(MSG_HELLO_RQ, opcode);

	send(s, opcode, 2, 0);
	recv(s, msg, 14, 0);

	char * print = strdup(msg + 2);

	// unsigned short rebut;
	// rebut = ldshort(msg);

	printf("\n%s", print);

	printf("\n");
}

/**
 * Function that sends a list request receives the list and displays it.
 * @param s The communications socket. 
 */
void process_list_operation(int s, char buffer[]) //1
{
  //char buffer[DNS_TABLE_MAX_SIZE];
  char opcode[MAX_BUFF_SIZE];
  stshort(MSG_LIST_RQ, opcode);
  int msg_size;

  //TODO: uncomment sendOpCodeMSG(s, MSG_LIST_RQ); //remember to implement sendOpCode in common.c

  send(s, opcode, 2, 0);
  memset(buffer, 0, MAX_BUFF_SIZE);
  //TODO: rebre missatge LIST
  msg_size = recv(s, buffer, MAX_BUFF_SIZE, 0);
  //TODO: Descomentar la següent línia
  printDNSTableFromAnArrayOfBytes(buffer+sizeof(short), msg_size-sizeof(short));
}

void process_showDomain(int s, char host[], char buffer[]) //2
{
	//char host[MAX_HOST_SIZE];
	printf("Introdueix el host: \n");
	scanf("%s", host);

	//char buffer[DNS_TABLE_MAX_SIZE];
	char opcode[MAX_BUFF_SIZE];
	char printer[MAX_BUFF_SIZE];
	memset(printer, 0, MAX_BUFF_SIZE);
	stshort(MSG_DOMAIN_RQ, opcode);
	memcpy(opcode + 2, host, MAX_HOST_SIZE + 3);
	send(s, opcode, MAX_HOST_SIZE + 3, 0);
	int mesure = recv(s, buffer, MAX_BUFF_SIZE, 0);
	int offset = 0;
	unsigned short codi;
	codi = ldshort(buffer);
	if (codi == MSG_OP_ERR) {
		printf("No s'ha trobat el DNS");
	}
	else {
		int nIPS = (mesure - 2) / 4;
		printf("%d", nIPS);
		for (int index = 0; index < nIPS; index++) {
			struct _IP *ip_address = malloc(sizeof(struct _IP));
			(ip_address->IP) = ldaddr(buffer + offset + 2);
			strcat(printer, inet_ntoa((ip_address->IP)));
			offset += 4;
		}
		printf("\n%s\n", buffer);
	}
}

void process_addIP(int s, char host[]) //3
{
	struct _IP *ip_address = malloc(sizeof(struct _IP));
	//char host[MAX_HOST_SIZE];
	char NEW_IP[MAX_BUFF_SIZE];
	memset(NEW_IP, 0, MAX_BUFF_SIZE);
	char byte[1];
	printf("A quin host voleu afegir la IP: ");
	scanf("%s", host);
	int longit = strlen(host);
	char opcode[MAX_BUFF_SIZE];

	stshort(MSG_ADD_DOMAIN, opcode);
	strcpy(opcode + 2, host);
	stshort(0, byte);
	strcpy(opcode + 2 + longit, byte);

	printf("IP a afegir: ");
	scanf("%s", NEW_IP);

	inet_aton(NEW_IP, &(ip_address->IP));

	memcpy(opcode + strlen(host) + 3, &(ip_address->IP), 4);

	send(s, opcode, strlen(host) + 7, 0);
}

void process_addMultipleIPS(int s, char host[]) //4
{
	//char host[MAX_HOST_SIZE];
	char NEW_IP[MAX_BUFF_SIZE];
	char NEW_IPS[MAX_BUFF_SIZE];
	memset(NEW_IP, 0, MAX_BUFF_SIZE);
	memset(NEW_IPS, 0, MAX_BUFF_SIZE);
	char byte[1];
	printf("A quin host voleu afegir la IP: ");
	scanf("%s", host);
	int longit = strlen(host);
	int nIps = 0;
	char opcode[MAX_BUFF_SIZE];

	stshort(MSG_ADD_DOMAIN, opcode);
	strcpy(opcode + 2, host);
	stshort(0, byte);
	strcpy(opcode + 2 + longit, byte);

	int condicio = 1;
	while (condicio != 0) {
		struct _IP *ip_address = malloc(sizeof(struct _IP));
		printf("IP a afegir: ");
		scanf("%s", NEW_IP);
		inet_aton(NEW_IP, &(ip_address->IP));
		//printf("%s", inet_ntoa((ip_address->IP)));
		memcpy(NEW_IPS + nIps * 4, &(ip_address->IP), 4);
		//send(s, NEW_IPS, 100, 0);
		printf("Vols seguir afegint IPs? (Y = 1/N = 0) \n");
		scanf("%d", &condicio);
		nIps++;
	}

	memcpy(opcode + 3 + longit, NEW_IPS, nIps * 4);

	send(s, opcode, longit + 3 + nIps * 4, 0);
}

void process_change(int s, char host[], char buffer[]) //5
{
	struct _IP *new_address = malloc(sizeof(struct _IP));
	struct _IP *old_address = malloc(sizeof(struct _IP));
	//char host[MAX_HOST_SIZE];
	char NEW_IP[MAX_BUFF_SIZE];
	char OLD_IP[MAX_BUFF_SIZE];
	//char buffer[DNS_TABLE_MAX_SIZE];
	memset(buffer, 0, MAX_BUFF_SIZE);
	memset(NEW_IP, 0, MAX_BUFF_SIZE);
	memset(OLD_IP, 0, MAX_BUFF_SIZE);
	char byte[1];
	printf("Quin host voleu modificar: ");
	scanf("%s", host);
	int longit = strlen(host);
	char opcode[MAX_BUFF_SIZE];

	stshort(MSG_CHANGE_DOMAIN, opcode);
	strcpy(opcode + 2, host);
	stshort(0, byte);
	strcpy(opcode + 2 + longit, byte);

	printf("IP vella: ");
	scanf("%s", OLD_IP);
	inet_aton(OLD_IP, &(old_address->IP));

	printf("IP nova: ");
	scanf("%s", NEW_IP);
	inet_aton(NEW_IP, &(new_address->IP));

	memcpy(opcode + longit + 3, &(old_address->IP), 4);
	memcpy(opcode + longit + 7, &(new_address->IP), 4);

	send(s, opcode, longit + 3 + 4 + 4, 0);

	recv(s, buffer, /*sizeof(buffer)*/MAX_BUFF_SIZE, 0);
	unsigned short codi;
	codi = ldshort(buffer);
	if (codi == MSG_OP_ERR) {
		printf("No s'ha trobat el DNS o la IP");
	}
	else {
		printf("\n%s\n", buffer);
	}
}

void process_deleteIP(int s, char host[], char buffer[]) //6
{
	struct _IP *ip_address = malloc(sizeof(struct _IP));
	//char buffer[DNS_TABLE_MAX_SIZE];
	//char host[MAX_HOST_SIZE];
	char NEW_IP[MAX_BUFF_SIZE];
	memset(NEW_IP, 0, MAX_BUFF_SIZE);
	char byte[1];
	printf("De quin host voleu eliminar la IP: ");
	scanf("%s", host);
	int longit = strlen(host);
	char opcode[MAX_BUFF_SIZE];

	stshort(MSG_DEL_IP, opcode);
	strcpy(opcode + 2, host);
	stshort(0, byte);
	strcpy(opcode + 2 + longit, byte);

	printf("IP a eliminar: ");
	scanf("%s", NEW_IP);

	inet_aton(NEW_IP, &(ip_address->IP));

	memcpy(opcode + strlen(host) + 3, &(ip_address->IP), 4);

	send(s, opcode, strlen(host) + 7, 0);

	recv(s, buffer, MAX_BUFF_SIZE, 0);
	unsigned short codi;
	codi = ldshort(buffer);
	if (codi == MSG_OP_ERR) {
		printf("No s'ha trobat el DNS");
	}
	else {
		printf("\n%s\n", buffer);
	}
	//system("tput clear");
}

void process_deleteDomain(int s, char host[], char buffer[]) //7
{
	//char host[MAX_HOST_SIZE];
	printf("Introdueix el host: \n");
	scanf("%s", host);

	//char buffer[DNS_TABLE_MAX_SIZE];
	char opcode[MAX_BUFF_SIZE];
	stshort(MSG_DEL_DOMAIN, opcode);
	memcpy(opcode + 2, host, strlen(host) + 3);

	send(s, opcode, strlen(host) + 3, 0);

	recv(s, buffer, MAX_BUFF_SIZE, 0);
	unsigned short codi;
	codi = ldshort(buffer);
	if (codi == MSG_OP_ERR) {
		printf("No s'ha trobat l'element indicat");
	}
	else {
		printf("\n%s\n", buffer);
	}
}

/** 
 * Function that process the menu option set by the user by calling 
 * the function related to the menu option.
 * @param s The communications socket
 * @param option the menu option specified by the user.
 */
void process_menu_option(int s, int option)
{
	char host[MAX_HOST_SIZE];
	char buffer[DNS_TABLE_MAX_SIZE];
	memset(host, 0, MAX_BUFF_SIZE);
	memset(buffer, 0, MAX_BUFF_SIZE);

	switch(option){
    // Opció HELLO
	case MENU_OP_HELLO:
	{
		process_hello(s);
		break;
	}

	case MENU_OP_LIST:
	{
		process_list_operation(s, buffer);
		break;
	}

	case MENU_OP_DOMAIN_RQ:
	{
		process_showDomain(s, host, buffer);
		break;
	}

	case MENU_OP_ADD_DOMAIN_IP:
	{
		process_addIP(s, host);
		break;
	}

	case MENU_OP_ADD_DOMAIN_IPS:
	{
		process_addMultipleIPS(s, host);
		break;
	}
	case MENU_OP_CHANGE:
	{
		process_change(s, host, buffer);
		break;
	}

	case MENU_OP_DELETE_IP:
	{
		process_deleteIP(s, host, buffer);
		break;
	}

	case MENU_OP_DELETE_DOMAIN:
	{
		process_deleteDomain(s, host, buffer);
		break;
	}
    case MENU_OP_FINISH:
      //TODO:
      break;
                
    default:
          printf("Invalid menu option\n");
  		}
}

int main(int argc, char *argv[])
{
	int port; // variable per al port inicialitzada al valor DEFAULT_PORT (veure common.h)
	char host[MAX_HOST_SIZE]; // variable per copiar el nom del host des de l'optarg
	int option = 0; // variable de control del menu d'opcions
	int ctrl_options;
  
    ctrl_options = getProgramOptions(argc, argv, host, &port); //dels arguments que passem, destriem el host i el port

	// Comprovem que s'hagi introduit un host. En cas contrari, terminem l'execucio de
	// l'aplicatiu	
	if(ctrl_options < 0){
		perror("No s'ha especificat el nom del servidor\n\n");
		return -1;
	}


 //TODO: setting up the socket for communication
	struct sockaddr_in address; //declaració de l'adreça
	address.sin_family = AF_INET;
	address.sin_port = htons(port); //port obtingut anteriorment
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	setaddrbyname((struct sockaddr_in*)&address, host); //quan executem el client, especifiquem quin es el
	//host on està el servidor amb -h (host)
	//amb setaddrbyname, assignem la ip del host a l'adreça a partir del host

	//socklen_t addrlen = sizeof(address);
	int s_cli = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //es crea el socket del client
	
	connect(s_cli, (struct sockaddr*)&address, sizeof(address)); //connectar servidor i client amb arguments
	//(socket), (sockaddr), socklen

  do{
      printa_menu();
		  // getting the user input.
	  scanf("%d",&option);
	  printf("\n\n"); 
	  process_menu_option(s_cli, option);

  }while(option != MENU_OP_FINISH); //end while(opcio)
    // TODO

  return 0;
}

