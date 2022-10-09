#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define BUF_SIZE 1000

void fill_header(char *header, int status, long len, char *type) ;
void content_type(char *ct_type, char *uri);
void error_handler(int asock, int num);
void http_handler(int asock);

int main(int argc, char **argv) 
{
    	int port, pid;
    	int lsock, asock;

    	struct sockaddr_in sin;
    	socklen_t sin_len;

    	if (argc < 2) 
	{
		printf("[ERROR] Port Error\n");
		exit(0);
    	}

    	port = atoi(argv[1]);

    	if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Socket Error\n");
		exit(1);
    	}

	memset((char *)&sin, '\0', sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);	

    	if (bind(lsock, (struct sockaddr *)&sin, sizeof(sin))) 
	{
		perror("[ERROR] Bind Error\n");
		exit(1);
    	}

    	if (listen(lsock, 10)) 
	{
		perror("[ERROR] Listen Error\n");
		exit(1);
    	}

    	signal(SIGCHLD, SIG_IGN);

    	while (1) 
	{
        	printf("LOADING...\n");
        	asock = accept(lsock, (struct sockaddr *)&sin, &sin_len);

        	if (asock < 0) 
		{
            		perror("[ERROR] Accept Error\n");
            		continue;
        	}

        	pid = fork();

        	if (pid == 0)
		{ 
            		close(lsock);
			http_handler(asock);
			close(asock);
            		exit(0);
        	}
        	else if (pid != 0)
			close(asock);
        	else if (pid < 0)
			perror("[ERROR] Fork Error\n");
	}
}

void fill_header(char *header, int status, long len, char *type) 
{
    	char status_text[40];

    	switch (status) 
	{
		case 200:
		    	strcpy(status_text, "OK");
			break;
		case 404:
		    	strcpy(status_text, "Not Found");
			break;
		case 500:
		default:
		    	strcpy(status_text, "Internal Server Error");
			break;
    	}

   	sprintf(header, "HTTP/1.1 %d %s\nContent-Length: %ld\nContent-Type: %s\n\n", status, status_text, len, type);
}

void content_type(char *ct_type, char *uri) 
{
	char *ext = strrchr(uri, '.');

    	if (!strcmp(ext, ".html")) 
        	strcpy(ct_type, "text/html");
    	else if (!strcmp(ext, ".jpg")) 
        	strcpy(ct_type, "image/jpg");
    	else if (!strcmp(ext, ".css"))
        	strcpy(ct_type, "text/css");
    	else
		strcpy(ct_type, "text/plain");
}

void error_handler(int asock, int num) 
{
    	char header[BUF_SIZE];

	if (num == 404)
	{
		fill_header(header, num, sizeof("<h1>404 Not Found</h1>\n"), "text/html");
	    	write(asock, header, strlen(header));
	    	write(asock, "<h1>404 Not Found</h1>\n", sizeof("<h1>404 Not Found</h1>\n"));
	}
	else if (num == 500)
	{
		fill_header(header, num, sizeof("<h1>500 Internal Server Error</h1>\n"), "text/html");
	    	write(asock, header, strlen(header));
	    	write(asock, "<h1>500 Internal Server Error</h1>\n", sizeof("<h1>500 Internal Server Error</h1>\n"));
	}
    	
}

void http_handler(int asock) 
{
    	char header[BUF_SIZE];
    	char buf[BUF_SIZE];

	char *method = strtok(buf, " ");
    	char *uri = strtok(NULL, " ");

    	if (read(asock, buf, BUF_SIZE) < 0 || method == NULL || uri == NULL) 
	{
        	error_handler(asock, 500);
		return;
    	}

    	printf("[INFO] Handling Request: method = %s, URI = %s\n", method, uri);
    
    	char safe_uri[BUF_SIZE];
    	char *local_uri;
    	struct stat st;

    	strcpy(safe_uri, uri);
    	if (!strcmp(safe_uri, "/"))
		strcpy(safe_uri, "/yena.html");
    
    	local_uri = safe_uri + 1;
    	if (stat(local_uri, &st) < 0) 
	{
		error_handler(asock, 404);
		return;
    	}

    	int fd = open(local_uri, O_RDONLY);
    	if (fd < 0) 
	{
		error_handler(asock, 500); 
		return;
    	}

    	int ct_len = st.st_size;
    	char ct_type[40];

    	content_type(ct_type, local_uri);
    	fill_header(header, 200, ct_len, ct_type);
    	write(asock, header, strlen(header));

    	int cnt;

    	while ((cnt = read(fd, buf, BUF_SIZE)) > 0)
        	write(asock, buf, cnt);
}