#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>

#define MAXPATH_SIZE 4096
#define MAXREQUEST_SIZE 2038

void error_msg(const char *msg){
    fprintf(stderr, "ERROR: ");
    fprintf(stderr, "%s", msg);
    exit(1);
}

FILE* get_fp(char* f_name, char* mode)
{
  FILE* ret = NULL;
  char working_dir[1024];
  memset(working_dir, 0, (sizeof(char) * 1024));
  if(getcwd(working_dir, (sizeof(char) * 1024)) == NULL)
    {
      perror("Cwd error");
      exit(1);
    }

  DIR* dir = opendir(working_dir);
  struct dirent* entry;

  while((entry = readdir(dir)) != NULL)
    {
      if(strcasecmp(f_name, entry->d_name) == 0)
	{
	  ret = fopen(entry->d_name, mode);
	  break;
	}
    }
  return ret;
}

void sig_handler(int sig)
{
  /*if (sig == SIGINT ){}
  else if(sig == SIGQUIT ){}
  else if (sig == SIGTERM){}
  else if (sig == SIGPIPE){}
  exit(0);*/
}

int main(int argc, char ** argv){
    /*if (signal(SIGINT, sig_handler) == SIG_ERR) {
    	printf("ERROR: cannot catch SIGINT\n");
    }*/
    sigaction(SIGPIPE, &(struct sigaction) {sig_handler}, NULL);
    
    int m_skfd = 0;
    int n_skfd = 0;
    struct addrinfo hints, *infoptr;
    struct sockaddr_storage c_addr;
    socklen_t addr_len = sizeof(c_addr);;
    memset(&hints, 0, sizeof hints);
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if( getaddrinfo(NULL, "6555", &hints, &infoptr) != 0 ){
    	error_msg(" can't getaddrinfo \n");
    }

    m_skfd = socket(infoptr->ai_family, infoptr->ai_socktype, infoptr->ai_protocol);

    if (m_skfd < 0) {
    	error_msg("Can't create socket\n");
    }
    
    int yes = 1;

    if (setsockopt(m_skfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    	error_msg("unable to setsockopt\n");
    }

    if( bind(m_skfd, infoptr->ai_addr, infoptr->ai_addrlen) < 0){
    	error_msg("unable to binding.\n");
    }
    
    if (listen(m_skfd, 5) == -1) {
        error_msg("unable to listen\n");
    }

    while(1){
	n_skfd = accept(m_skfd, (struct sockaddr *) &c_addr, &addr_len);
	
        if (n_skfd < 0){
            error_msg("unable to accept");
	}

        char buffer[MAXREQUEST_SIZE];
        memset(buffer, 0, MAXREQUEST_SIZE);  

        // Get client's message
        int read_client_msg = read(n_skfd, buffer, MAXREQUEST_SIZE);
        if (read_client_msg < 0){
	    close(m_skfd);
	    close(n_skfd); 
            error_msg("unable reading from socket");
	}

        fprintf(stdout, "%s", buffer);
        // Process HTTP request.
        char f_name[MAXPATH_SIZE]; // Maximum pathname length in Linux.
        memset(f_name, 0, MAXPATH_SIZE);
        int filename_index = 0;
	char f_type[5];
	memset(f_type, 0, 5);
	//int f_type = -1; // .html, .htm, .jpeg, .gif, or .jpg
        int filetype_index = 0;
	int in_filetype = 0;

	//
	// This section reserved for getting the name 
	// and type of the file
	//

	int num_spaces = 0;
	int buffer_index = 5;
	//count the number of spaces after GET
	while(buffer[buffer_index] != '\n' && buffer_index < MAXREQUEST_SIZE)
	  {
	    if(buffer[buffer_index] == ' ')
	      num_spaces++;
	    
	    buffer_index++;
	  }

	//parse the string
	buffer_index = 5;
	int counter = 0;
	int space_flag = 0;
	while(buffer[buffer_index] != '\n' && read_client_msg > 0)
	  {
	    if(buffer[buffer_index] == ' ')
	      {
		counter++;
		if(counter == num_spaces)
		  {
		    break;
		  }
	      }
	    else if(buffer[buffer_index] == '.')
	      {
		in_filetype = 1;
		f_name[filename_index] = '.';
		filename_index++;
	      }
	    else
	      {
		if(in_filetype)
		  {
		    f_type[filetype_index] = buffer[buffer_index];
		    filetype_index++;
		  }
		
		if(strncmp(&buffer[buffer_index], "%20", 3) == 0)
		  {
		    f_name[filename_index] = ' ';
		    space_flag = 1;
		  }
		else
		  f_name[filename_index] = buffer[buffer_index];
		    
		filename_index++;  
	      }
	    
	    if(space_flag)
	      {
		buffer_index += 3;
		space_flag = 0;
	      }
	    else
	      buffer_index++;
	  }

	f_name[filename_index] = '\0';
	f_type[filetype_index] = '\0';
	
	FILE* file = get_fp(f_name, "r");
	if(file == NULL){
	  char err[48] = "HTTP /1.1 404 NOT FOUND\r\n\r\n";
	  int err_size = strlen(err);
	  write(n_skfd, err, err_size);
	  char fun_msg[48] ="<h1> NOT FOUND </h1>\n";
	  int fun_size = strlen(fun_msg);
	  write(n_skfd, fun_msg, fun_size);
	  char fun_msg2[128] = "<p> The requested URL /";
	  strcat(fun_msg2, f_name);
	  strcat(fun_msg2, " was not found on this server </p>");
	  int fun_size2 = strlen(fun_msg2);
	  write(n_skfd, fun_msg2, fun_size2);
	  close(n_skfd);
	  continue;
	}
	
	char resp_status[48] = "HTTP/1.1 200 OK\r\n";

	//https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
	//GET CONTENT LENGTH
   	fseek(file,0,SEEK_END);
    	unsigned long f_size = ftell(file);
	//fprintf(stderr, "f_size: %d", f_size);
	fseek(file,0,SEEK_SET);
	char* entity = malloc(f_size+1);
	long entity_size = fread(entity, 1, f_size, file);
	fclose(file);
	entity[f_size] = 0;

	char content_len[48] = "Content-Length: ";
	char con_size[11];
	snprintf (con_size, 11, "%lu", f_size);
	strcat(content_len, con_size);
	strcat(content_len, "\r\n");
	
	//GET SERVER
	char server[32] = "Server: Ubuntu\r\n";

	//GET CUURENT TIME
	time_t rn;
  	time(&rn);
  	struct tm* current_time = gmtime(&rn);
  	char curr_time_buf[48];
	memset(curr_time_buf, 0, 48);
  	strftime(curr_time_buf, 48, "%a, %d %b %Y %T %Z", current_time);
	char curr_time[48] = "Date: ";
	strcat(curr_time, curr_time_buf);
	strcat(curr_time, "\r\n");

	//GET LAST MODIFIED TIME
	//https://stackoverflow.com/questions/10446526/get-last-modified-time-of-file-in-linux
	struct tm* last_edit_time;
    	struct stat t;
    	stat(f_name, &t);
    	last_edit_time = gmtime(&(t.st_mtime));
    	char last_time_buf[48];
	memset(last_time_buf, 0 ,48);
    	strftime(last_time_buf, 48, "%a, %d %b %Y %T %Z", last_edit_time);
        char last_time[48] = "Last-Modified: ";
	strcat(last_time, last_time_buf);
	strcat(last_time, "\r\n");
	
	//GET CONTENT TYPE
	char content_type[32];
	if(strcmp( f_type, "html\0") == 0){
	  strcpy(content_type, "Content-Type: text/html\r\n");
	}
	else if (strcasecmp( f_type,"htm\0") == 0){
	  strcpy(content_type, "Content-Type: text/htm\r\n");
        }
	else if (strcasecmp(f_type,"txt\0") == 0){
	  strcpy(content_type, "Content-Type: text/txt\r\n");
        }
	else if (strcasecmp(f_type,"jpg\0") == 0){
	  strcpy(content_type, "Content-Type: image/jpg\r\n");
        }
	else if (strcasecmp(f_type,"jpeg\0") == 0){
	  strcpy(content_type, "Content-Type: image/jpeg\r\n");
        }
	else if (strcasecmp(f_type,"png\0") == 0){
	  strcpy(content_type, "Content-Type: image/png\r\n");
        }
	else if (strcasecmp(f_type,"gif\0") == 0){
	  strcpy(content_type, "Content-Type: image/gif\r\n");
        }
	else{
	  strcpy(content_type, "Content-Type: \r\n");
	}
	
	//Connection close
	char conn_close[32] = "Connection: Closed\r\n";
	char empty_line[2] = "\r\n";

	//Response message
	write(n_skfd, resp_status, strlen(resp_status));
  	write(n_skfd, conn_close, strlen(conn_close));
	write(n_skfd, curr_time, strlen(curr_time));
	write(n_skfd, server, strlen(server));
	write(n_skfd, last_time, strlen(last_time));
	write(n_skfd, content_len, strlen(content_len));
	write(n_skfd, content_type, strlen(content_type));	
	write(n_skfd, empty_line, strlen(empty_line));
	write(n_skfd, entity, f_size);
	close(n_skfd);
    }
    
    close(m_skfd);
    exit(0);
}

