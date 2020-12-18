/*
*  C Implementation: nameServer
*
* Description: 
*
*
* Author: MCarmen de Toro <mc@mc>, (C) 2015
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include "nameServer.h"



/* Reads a line ended with \n from the file pointer.  */
/* Return: a line ended not with an EOL but with a 0 or NULL if the end of the
file is reached */
char *readLine(FILE *file, char *line, int sizeOfLine)
{
  
  int line_length;

  if (fgets(line, sizeOfLine, file) != NULL)
  {
    line_length = strlen(line)-1;
    line[line_length] = 0;    
  } 
  else
  {
    line = NULL;
  }

  return line;
}


/**
 * Creates a DNSEntry variable from the content of a file line and links it 
 * to the DNSTable. 
 * @param line the line from the file to be parsed
 * @param delim the character between tokens.
 */
struct _DNSEntry* buildADNSEntryFromALine(char *line, char *token_delim)
{
  
  char *token;
  struct _IP *ip_struct = malloc(sizeof(struct _IP));
  struct _IP *last_ip_struct;
  struct _DNSEntry* dnsEntry = malloc(sizeof(struct _DNSEntry)); 
  int firstIP = 1;
 

  //getting the domain name
  token = strtok(line, token_delim);
  strcpy(dnsEntry->domainName, token);
  dnsEntry->numberOfIPs = 0;

  //getting the Ip's
  while ((token = strtok(NULL, token_delim)) != NULL)
  {
    ip_struct = malloc(sizeof(struct _IP));
    inet_aton((const char*)token, &(ip_struct->IP));
    ip_struct->nextIP = NULL;
    (dnsEntry->numberOfIPs)++;
    if (firstIP == 1)
    {
      dnsEntry->first_ip = ip_struct;
      last_ip_struct = ip_struct;
      firstIP = 0;
    }
    else
    {
      last_ip_struct->nextIP = ip_struct;
      last_ip_struct = ip_struct;
    }
  }  
    
    return dnsEntry;
}

/* Reads a file with the dns information and loads into a _DNSTable structure.
Each line of the file is a DNS entry. 
RETURNS: the DNS table */
struct _DNSTable* loadDNSTableFromFile(char *fileName)
{
  FILE *file;
  char line[1024];
  struct _DNSEntry *dnsEntry;
  struct _DNSEntry *lastDNSEntry;
  struct _DNSTable *dnsTable = malloc(sizeof(struct _DNSTable)); 
  int firstDNSEntry = 1;

  file = fopen(fileName, "r");
  if (file==NULL)
  {
    perror("Problems opening the file");
    printf("Errno: %d \n", errno);
  }
  else
  {
    //reading the following entries in the file
    while(readLine(file, line, sizeof(line)) != NULL)
    {
      dnsEntry = buildADNSEntryFromALine(line, " ");
      dnsEntry->nextDNSEntry = NULL;
      if (firstDNSEntry == 1)
      {
        dnsTable->first_DNSentry = dnsEntry;
        lastDNSEntry = dnsEntry;
        firstDNSEntry = 0;
      }
      else
      {
        lastDNSEntry->nextDNSEntry = dnsEntry;
        lastDNSEntry = dnsEntry;        
      }  
    } 
      
    
    fclose(file);
  }
  
  return dnsTable;
}


/**
 * Calculates the number of bytes of the DNS table as a byte array format. 
 * It does not  include the message identifier. 
 * @param dnsTable a pointer to the DNSTable in memory.
 */
int getDNSTableSize(struct _DNSTable* dnsTable)
{
  int table_size = 0;
  int numberOfIPs_BYTES_SIZE = sizeof(short);
  
  
  struct _DNSEntry *dnsEntry;

  dnsEntry = dnsTable->first_DNSentry;
  if(dnsEntry != NULL)
  {
    do
    {    
      table_size +=  ( strlen(dnsEntry->domainName) + SPACE_BYTE_SIZE +
        numberOfIPs_BYTES_SIZE + (dnsEntry->numberOfIPs * sizeof (in_addr_t)) );
    }while((dnsEntry=dnsEntry->nextDNSEntry) != NULL);
  }
 

  return table_size; 
}



/*Return a pointer to the last character copied in next_DNSEntry_ptr + 1 */
/**
 * Converts the DNSEntry passed as a parameter into a byte array pointed by 
 * next_DNSEntry_ptr. The representation will be 
 * domain_name\0number_of_ips[4byte_ip]*]. 
 * @param dnsEntry the DNSEntry to be converted to a Byte Array.
 * @param next_DNSEntry_ptr a pointer to Byte Array where to start copying 
 * the DNSEntry. The pointer moves to the end of the ByteArray representation.
 */
void dnsEntryToByteArray(struct _DNSEntry* dnsEntry, char **next_DNSEntry_ptr)
{
  
  struct _IP* pIP;
 
  fflush(stdout);
  
  strcpy(*next_DNSEntry_ptr, dnsEntry->domainName);
  //we leave one 0 between the name and the number of IP's of the domain
  *next_DNSEntry_ptr += (strlen(dnsEntry->domainName) + 1);
  stshort(dnsEntry->numberOfIPs, *next_DNSEntry_ptr);
  *next_DNSEntry_ptr += sizeof(short);
  if((pIP = dnsEntry->first_ip) != NULL)
  {    
    do    
    { 
      staddr(pIP->IP, *next_DNSEntry_ptr);      
      *next_DNSEntry_ptr += sizeof(in_addr_t);
    }while((pIP = pIP->nextIP) != NULL);
  }
 
}


/*Dumps the dnstable into a byte array*/
/*@Return a pointer to the byte array representing the DNS table */
/*@param dnsTable the table to be serialized into an array of byes */
/*@param _tableSize reference parameter that will be filled with the table size*/
char *dnsTableToByteArray(struct _DNSTable* dnsTable, int *_tableSize)
{ 
  int tableSize = getDNSTableSize(dnsTable);
  *_tableSize = tableSize;

  char *dns_as_byteArray = malloc(tableSize);
  char *next_dns_entry_in_the_dns_byteArray_ptr = dns_as_byteArray;
  struct _DNSEntry *dnsEntry;

  
  bzero(dns_as_byteArray, tableSize);
  
  dnsEntry = dnsTable->first_DNSentry;
  do
  {
    dnsEntryToByteArray(dnsEntry, &next_dns_entry_in_the_dns_byteArray_ptr);
  }while((dnsEntry=dnsEntry->nextDNSEntry) != NULL);

  return dns_as_byteArray;
  
}

/**
 * Function that gets the dns_file name and port options from the program 
 * execution.
 * @param argc the number of execution parameters
 * @param argv the execution parameters
 * @param reference parameter to set the dns_file name.
 * @param reference parameter to set the port. If no port is specified 
 * the DEFAULT_PORT is returned.
 */
int getProgramOptions(int argc, char* argv[], char *dns_file, int *_port) //funcio amb la qual s'obte el port i el nom nom del fitxer
{
  int param;
   *_port = DEFAULT_PORT;

  // We process the application execution parameters.
	while((param = getopt(argc, argv, "f:p:")) != -1){
		switch((char) param){		
			case 'f':
				strcpy(dns_file, optarg);				
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
 * Function that generates the array of bytes with the dnsTable data and 
 * sends it.
 * @param s the socket connected to the client.
 * @param dnsTable the table with all the domains
 */
void process_LIST_RQ_msg(int sock, struct _DNSTable *dnsTable)
{
  char *dns_table_as_byteArray;
  char *msg;
  int dns_table_size;
  int msg_size = sizeof(short);
  

  dns_table_as_byteArray = dnsTableToByteArray(dnsTable, &dns_table_size);
  
  msg_size += dns_table_size;
  
  msg = malloc(msg_size);
  //TODO: set the operation code and the table data

  memset(msg, 0, msg_size);

  stshort(MSG_LIST, msg);
  memcpy(msg + sizeof(short), dns_table_as_byteArray, dns_table_size);

  
  //TODO: send the message
  
  send(sock, msg, msg_size, 0);
}

/** 
 * Receives and process the request from a client.
 * @param s the socket connected to the client.
 * @param dnsTable the table with all the domains
 * @return 1 if the user has exit the client application therefore the 
 * connection whith the client has to be closed. 0 if the user is still 
 * interacting with the client application.
 */
int process_msg(int sock, struct _DNSTable *dnsTable)
{
  unsigned short op_code;
  char buffer[MAX_BUFF_SIZE];
  int done = 0;
  int sizeBuf =  recv(sock, buffer, sizeof(buffer), 0);
  //send(sock, p, 8, 0);
  
 // int p = sizeof(buffer);
  op_code = ldshort(buffer); //obtenim l'op_code

  switch(op_code) //switch de l'op_code per executar el codi que pertoqui
  {
    case MSG_HELLO_RQ:
	{
		//TODO
		char buf[MAX_BUFF_SIZE];
		memset(buf, 0, sizeof(buf));

		stshort(MSG_HELLO, buf);

		strcpy(buf + 2, "HELLO WORLD");
		send(sock, buf, 14, 0);
	}

      break;  
    case MSG_LIST_RQ:
      process_LIST_RQ_msg(sock, dnsTable);
      break;

    case MSG_DOMAIN_RQ:
	{
		char * host = strdup(buffer + 2);

		char msg[MAX_BUFF_SIZE];
		memset(msg, 0, sizeof(msg));
		
		char * comp;
		
		printf("%s", host);
		int offset = 0;
		int nIPS = 0;

		struct _DNSEntry *dnsEntry = dnsTable->first_DNSentry;
		int Trobat = 0;
		while (dnsEntry != NULL && Trobat == 0) {
			comp = dnsEntry->domainName;
			if (strcmp(comp, host) == 0) {
				struct _IP* ip_address;
				ip_address = dnsEntry->first_ip;
				stshort(MSG_DOMAIN, msg);
				while (ip_address != NULL) {
					nIPS = dnsEntry->numberOfIPs;
					memcpy(msg + 2 + offset, &(ip_address->IP), dnsEntry->numberOfIPs);
					//staddr(ip_address->IP, msg + 4 * index);
					//strncat(msg, " ", 1);
					ip_address = ip_address->nextIP;
					offset += 4;
				}
				Trobat = 1;
			}
			dnsEntry = dnsEntry->nextDNSEntry;
		}
		if (Trobat == 0) {
			stshort(MSG_OP_ERR, msg);
			strcpy(msg + 2, "ERROR");
			send(sock, msg, 8, 0);
		}
		else {
			send(sock, msg, 2 + nIPS * 4 + 2, 0);
		}
		
		break;
	}

	case MSG_ADD_DOMAIN:
	{
		struct _IP *ip_aux = malloc(sizeof(struct _IP));
		struct _IP *ip_last = malloc(sizeof(struct _IP));
		struct _DNSEntry* dnsEntry = malloc(sizeof(struct _DNSEntry));
		struct _DNSEntry* LastdnsEntry = malloc(sizeof(struct _DNSEntry));

		char * host = strdup(buffer + 2);

		int nIPS = (sizeBuf - strlen(host) - 3) / 4;
		char * ips = malloc(nIPS * 4);
		memcpy(ips, buffer + strlen(host) + 3, nIPS * 4);

		char * comp;

		dnsEntry = dnsTable->first_DNSentry;
		int Trobat = 0;
		int offset = 0;
		while (dnsEntry != NULL && Trobat == 0) {
			comp = dnsEntry->domainName;
			if (strcmp(comp, host) == 0) {
				ip_aux = dnsEntry->first_ip;
				while (ip_aux != NULL) {
					ip_last = ip_aux;
					ip_aux = ip_aux->nextIP;
				}
				
				for (int index = 0; index < nIPS; index++) {
					struct _IP *ip_address = malloc(sizeof(struct _IP));
					(ip_address->IP) = ldaddr(ips + offset);
					ip_last->nextIP = ip_address;
					dnsEntry->numberOfIPs++;
					ip_address->nextIP = NULL;
					ip_last = ip_address;
					offset += 4;
				}
				Trobat = 1;
			}
			LastdnsEntry = dnsEntry;
			dnsEntry = dnsEntry->nextDNSEntry;
		}

		if (Trobat == 0) {
			struct _DNSEntry *newEntry = malloc(sizeof(struct _DNSEntry));
			newEntry->nextDNSEntry = NULL;
			memcpy(newEntry->domainName, host, strlen(host));
			LastdnsEntry->nextDNSEntry = newEntry;
			for (int index = 0; index < nIPS; index++) {

				struct _IP *ip_address = malloc(sizeof(struct _IP));
				(ip_address->IP) = ldaddr(ips + offset);
				newEntry->numberOfIPs++;
				ip_address->nextIP = NULL;

				if (index == 0) {
					newEntry->first_ip = ip_address;
				}
				else {
					ip_aux = newEntry->first_ip;
					while (ip_aux != NULL) {
						ip_last = ip_aux;
						ip_aux = ip_aux->nextIP;
					}
					ip_last->nextIP = ip_address;
				}
				offset += 4;
			}

		}
			

		break;
	}

	case MSG_CHANGE_DOMAIN:
	{
		struct _IP *ip_aux = malloc(sizeof(struct _IP));
		struct _DNSEntry* dnsEntry = malloc(sizeof(struct _DNSEntry));
		struct _IP *new_address = malloc(sizeof(struct _IP));
		struct _IP *old_address = malloc(sizeof(struct _IP));

		char msg[MAX_BUFF_SIZE];
		memset(msg, 0, sizeof(msg));

		char * host = strdup(buffer + 2);
		char ips[MAX_BUFF_SIZE];
		strcpy(ips, strdup(buffer + strlen(host) + 3));

		(old_address->IP) = ldaddr(ips);
		(new_address->IP) = ldaddr(ips + 4);

		char * comp;

		dnsEntry = dnsTable->first_DNSentry;

		int Trobat = 0;
		while (dnsEntry != NULL && Trobat == 0) {

			comp = dnsEntry->domainName;

			if (strcmp(comp, host) == 0) {

				ip_aux = dnsEntry->first_ip;

				while (ip_aux != NULL && Trobat == 0) {	

					char OLD_IP[MAX_BUFF_SIZE];
					memset(OLD_IP, 0, sizeof(OLD_IP));
					strcpy(OLD_IP, inet_ntoa(old_address->IP));

					char * comp2 = inet_ntoa(ip_aux->IP);
					if (strcmp(comp2, OLD_IP) == 0) {
						inet_aton(inet_ntoa(new_address->IP), &(ip_aux->IP));
						Trobat = 1;
					}
					ip_aux = ip_aux->nextIP;

				}
			}
			dnsEntry = dnsEntry->nextDNSEntry;
		}

		if (Trobat == 0) {
			stshort(MSG_OP_ERR, msg);
			strcpy(msg + 2, "ERROR");
			send(sock, msg, 8, 0);
		}
		else {
			stshort(1, msg);
			strcpy(msg + 2, "NOTERROR");
			send(sock, msg, 10, 0);
		}

		Trobat = 0;

		break;
	}

	case MSG_DEL_IP:
	{
		struct _IP *ip_aux = malloc(sizeof(struct _IP));
		struct _DNSEntry* dnsEntry = malloc(sizeof(struct _DNSEntry));
		struct _IP *previousIP = malloc(sizeof(struct _IP));
		struct _IP *old_address = malloc(sizeof(struct _IP));

		char msg[MAX_BUFF_SIZE];
		memset(msg, 0, sizeof(msg));

		char * host = strdup(buffer + 2);
		char ip[MAX_BUFF_SIZE];
		strcpy(ip, strdup(buffer + strlen(host) + 3));

		(old_address->IP) = ldaddr(ip);

		char * comp;

		dnsEntry = dnsTable->first_DNSentry;
		//free(dnsEntry->first_ip);
		int index = 0;
		int Trobat = 0;
		while (dnsEntry != NULL && Trobat == 0) {
			comp = dnsEntry->domainName;
			if (strcmp(comp, host) == 0) {
				ip_aux = dnsEntry->first_ip;

				while (ip_aux != NULL && Trobat == 0) {
					char OLD_IP[MAX_BUFF_SIZE];
					memset(OLD_IP, 0, sizeof(OLD_IP));
					strcpy(OLD_IP, inet_ntoa(old_address->IP));

					char * comp2 = inet_ntoa(ip_aux->IP);

					if (strcmp(comp2, OLD_IP) == 0) {
						if (index == 0) { //primera IP
							dnsEntry->first_ip = ip_aux->nextIP;
							ip_aux->nextIP = NULL;
						}
						else {
							previousIP->nextIP = ip_aux->nextIP;
							ip_aux->nextIP = NULL;
						}

						free(ip_aux);
						Trobat = 1;
						dnsEntry->numberOfIPs--;
					}
					previousIP = ip_aux;
					ip_aux = ip_aux->nextIP;
					index++;
				}
			}
			dnsEntry = dnsEntry->nextDNSEntry;
		}

		if (Trobat == 0) {
			stshort(MSG_OP_ERR, msg);
			strcpy(msg + 2, "ERROR");
			send(sock, msg, 8, 0);
		}
		else {
			stshort(1, msg);
			strcpy(msg + 2, "NOTERROR");
			send(sock, msg, 10, 0);
		}

		Trobat = 0;

		break;
	}

	case MSG_DEL_DOMAIN:
	{
		char * host = strdup(buffer + 2);

		char msg[MAX_BUFF_SIZE];
		memset(msg, 0, sizeof(msg));

		char * comp;
		int index = 0;

		printf("%s", host);

		struct _DNSEntry *dnsEntry = dnsTable->first_DNSentry;
		struct _DNSEntry *previousEntry = dnsTable->first_DNSentry;
		int Trobat = 0;
		while (dnsEntry != NULL && Trobat == 0) {
			comp = dnsEntry->domainName;
			if (strcmp(comp, host) == 0) {
				//borrar dnsEntry
				if (index == 0) {
					dnsTable->first_DNSentry = dnsEntry->nextDNSEntry;
				}
				else {
					previousEntry->nextDNSEntry = dnsEntry->nextDNSEntry;
				}
				free(dnsEntry);
				Trobat = 1;
			}
			index++;
			previousEntry = dnsEntry;
			dnsEntry = dnsEntry->nextDNSEntry;
		}
		if (Trobat == 0) {
			stshort(MSG_OP_ERR, msg);
			strcpy(msg + 2, "ERROR");
			send(sock, msg, 8, 0);
		}
		else {
			send(sock, "NOTERROR", 8, 0);
		}

		break;
	}
                     
    case MSG_FINISH:
	{
		close(sock);
		done = 1;
		break;
	}
      
    default:
      perror("Message code does not exist.\n");
  } 
  
  return done;
}

int main (int argc, char * argv[])
{
  struct _DNSTable *dnsTable;
  int port ; //enter que identifica el port
  int pid; //enter que identifica el pid del proces
  char dns_file[MAX_FILE_NAME_SIZE] ; //nom del fitxer amb els dns
  int finish = 0; //enter per acabar l'acceptacio de peticions
  
  getProgramOptions(argc, argv, dns_file, &port); //funcio amb la qual obtenim el port i el nom del fitxer. 
  //Aquests s'especifiquen amb arguments -p (port) -f (fitxer)
  
  dnsTable = loadDNSTableFromFile(dns_file); //funcio per convertir el fitxer a dnsTable

  printDNSTable(dnsTable); //printa la dnsTable (hosts)
  
  //TODO: setting up the socket for communication
  int s_serv = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //funcio que crea el host, amb arguments domini, tipus i protocol (TCP)
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);
  int s2;
  struct sockaddr_in addr; //es defineix la variable (sockaddr_in)
  //on afegirem el port

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port); //afegim el port a l'adreça
  addr.sin_addr.s_addr = htonl(INADDR_ANY);//posem la IP localhost

  bind(s_serv, (struct sockaddr*)&addr, sizeof(addr)); //s'associa el port i la IP al socket
  listen(s_serv, MAX_QUEUED_CON);
  
  
  
  while(1) {  
	  s2 = accept(s_serv, (struct sockaddr*)&address, &addrlen); //acceptem la connexió per part del client

	  if ((pid = fork()) == 0) { //child process
		  close(s_serv); //si es el fill, tanquem el socket del servidor

		  while (finish != 1) {
			  finish = process_msg(s2, dnsTable); //procesem fins que process_msg retorni 1 (el client fica l'opció de tancar connexió)
		  }
		  close(s2); //es tanca el socket de la connexió
		  exit(0); //s'elimina el procés fill
	  }
	  else {
		  close(s2); //si es el pare, tanca el socket de la connexió i accepta clients en bucle
	  }
  }
 
  return 0;
}


