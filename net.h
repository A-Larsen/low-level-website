/* 
 * Copyright (C) 2024  Austin Larsen
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef NET_H
#define NET_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <errno.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "perr.h"

typedef struct 
_NetKV 
{
    char *key;
    char *value;
} 
NetKV;

typedef struct _Response 
{
    char path[128];
    char method[10];
    char message[2048];
} Response;

typedef struct
_NetServer
{
    SSL_CTX *ctx;
	struct addrinfo *bind_address;
    int socket;
    char pemFile[32];
    SSL * ssl;
    Response *responses;
    uint8_t reslen;
}
NetServer;

void netParse_alloc(out, outlen, netkv, len)
    char **out;
    uint16_t *outlen;
    NetKV *netkv;
    uint8_t len;
{
    *out = NULL;
    char *outp = NULL;
    *outlen = 0;

    for (int i = 0; i < len; ++i) {
        char *key = netkv[i].key;
        char *value = netkv[i].value;
        int keylen = strlen(key);
        int valuelen = strlen(value);

        uint16_t llen = keylen + valuelen + 2;
        *outlen += llen;

        if (*out == NULL) {
            *out = malloc(*outlen);
            outp = *out;
        } else {
            *out = realloc(*out, *outlen);
        }

        memcpy(outp, key, keylen);
        outp += keylen;

        memcpy(outp, "=", 1);
        outp += 1;

        memcpy(outp,value, valuelen);
        outp += valuelen;

        if (i != len - 1) {
            memcpy(outp, "&", 1);
            outp += 1;
        }
    }
}

void netAssign(netServer, path, method, message)
    NetServer *netServer;
    char *path;
    char *method;
    char *message;
{
    uint8_t i = netServer->reslen;
    netServer->reslen++;

    if (i == 0) {
        netServer->responses = malloc(sizeof(Response));
    } else {
        netServer->responses = realloc(netServer->responses,
                                       sizeof(Response) * (i + 1));
    }

    memset(netServer->responses[i].path, 0, sizeof(netServer->responses->path));
    memset(netServer->responses[i].method, 0,
           sizeof(netServer->responses[i].method));
    memset(netServer->responses[i].message, 0,
           sizeof(netServer->responses[i].message));
    memcpy(netServer->responses[i].path, path, strlen(path));
    memcpy(netServer->responses[i].method, method, strlen(method));
    memcpy(netServer->responses[i].message, message, strlen(message));
}

int netInit(netServer, pemFile)
    NetServer *netServer;
    char *pemFile;
{
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
    netServer->reslen = 0;

    memset(netServer->pemFile, 0, 32);
    memcpy(netServer->pemFile, pemFile, strlen(pemFile));


    netServer->ctx = SSL_CTX_new(TLS_server_method());
	if (!netServer->ctx) {
		PERROR("SSL_CTX_new() failed");
		return 1;
	}

	if (!SSL_CTX_use_certificate_file
            (netServer->ctx, 
             netServer->pemFile, SSL_FILETYPE_PEM) || 
             !SSL_CTX_use_PrivateKey_file
             (netServer->ctx, netServer->pemFile, 
             SSL_FILETYPE_PEM)) {
		PERROR("SSL_CTX_use_certificate_file() failed");
		ERR_print_errors_fp(stderr);
		return 2;
	}

	printf("Configuring local address...\n");

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;   // IPv4
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_flags = AI_PASSIVE;

	/* struct addrinfo *bind_address; */
	getaddrinfo(0, "443", &hints, 
                &netServer->bind_address);

	printf("Creating socket...\n");
    netServer->socket = socket(netServer->bind_address->ai_family, 
                               netServer->bind_address->ai_socktype,
			                   netServer->bind_address->ai_protocol);
	if (netServer->socket < 0) {
		PERROR("socket() failed");
		return 3;
	}
    printf("Binding socket to local address...\n");

	if (bind(netServer->socket, netServer->bind_address->ai_addr,
                netServer->bind_address->ai_addrlen)) {
		PERROR("bind() failed");
		return 4;
	}

    return 0;
}
int netListen(netServer)
    NetServer *netServer;
{
	printf("Listening..\n");

	if (listen(netServer->socket, 10) < 0) {
		PERROR("listen() failed");
		return 5;
	}

	while(1) {
		printf("Waiting for connection...\n");
		struct sockaddr_storage client_address;
		socklen_t client_len = sizeof(client_address) + 1;

		int socket_client = accept(netServer->socket, 
				(struct sockaddr *)&client_address, &client_len);

		if (socket_client < 0) {
            PERROR("accept() failed");
			return 1;
		}

		printf("Client is connected...");
		char address_buffer[100];
		getnameinfo((struct sockaddr *)&client_address, client_len,
				address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
		printf("%s\n", address_buffer);

        netServer->ssl = SSL_new(netServer->ctx);
		if (!netServer->ctx) {
			PERROR("SSL_new() failed");
			return 1;
		}

		SSL_set_fd(netServer->ssl, socket_client);

		if (SSL_accept(netServer->ssl) <= 0) {
			fprintf(stderr, "SSL_accept() failed.\n");
			ERR_print_errors_fp(stderr);

			SSL_shutdown(netServer->ssl);
			close(socket_client);
			SSL_free(netServer->ssl);
			continue;
		}

		printf("SSL connection using %s\n", SSL_get_cipher(netServer->ssl));

		printf("Reading request...\n");
		char request[1024];
        memset(request, 0, 1024);
		/* int bytes_received = SSL_read(netServer->ssl, request, 1024); */
		SSL_read(netServer->ssl, request, 1024);
		/* printf("Received %d bytes.\n", bytes_received); */
        printf("request: %s\n", request);

        printf("=======\n");

        char *p = request;
        char *line = p;
        while (*p && *p != '\r') ++p;
        *p = '\0';
        printf("\n'%s'\n", line);

        bool isGet = strstr(line, "GET") != NULL;
        bool isPost = strstr(line, "POST") != NULL;
        if (isGet) {
            p  = line += 4;
        }else if (isPost) {
            p  = line += 5;
        }
        char *path = p;
        while (*p && *p != ' ') ++p;
        *p = '\0';
        printf("path: '%s'\n", path);

        for (int i = 0; i < netServer->reslen; ++i) {
            if ((!strcmp(netServer->responses[i].method, "GET") && isGet)) {
                if (!strcmp(netServer->responses[i].path, path)) {
                    printf("sup\n");
                    /* int bytes_sent = SSL_write(netServer->ssl, */
                    SSL_write(netServer->ssl,
                            netServer->responses[i].message,
                            strlen(netServer->responses[i].message));
                }
            }
            if ((!strcmp(netServer->responses[i].method, "POST") && isPost)) {
                if (!strcmp(netServer->responses[i].path, path)) {
                    printf("POST REQUEST!!!\n");
                    SSL_write(netServer->ssl,
                            netServer->responses[i].message,
                            strlen(netServer->responses[i].message));
                }
            }
        }

        /* printf("=======\n"); */
        /* for (; line < bytes_received; ++i) { */
        /*     /1* while (line[i] != '\n') line++; *1/ */
        /*     /1* line[i] = '\0'; *1/ */

        /*     /1* printf("line: %s\n", line); *1/ */
        /* } */

		/* printf("Sending response...\n"); */

        /* int bytes_sent = SSL_write(netServer->ssl, response, strlen(response)); */
		/* printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response)); */

		/* printf("Closing connection...\n"); */

        SSL_shutdown(netServer->ssl);
		close(socket_client);
		SSL_free(netServer->ssl);
	}
    return 0;
}

int netClose(netServer)
    NetServer netServer;
{
    free(netServer.responses);
    close(netServer.socket);
    SSL_CTX_free(netServer.ctx);
    return 0;
}
#endif // NET_H
