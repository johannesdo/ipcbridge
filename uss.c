#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <pthread.h>


#define SOCKET_PATH "/tmp/ipcbridge.unix"

#define BUFLEN 1024

static volatile int srv_sock;
static volatile int doloop = 1;
static volatile fd_set active_fd_set;

pthread_t t;
pthread_attr_t attr;
pthread_mutex_t mutex_active_fd_set = PTHREAD_MUTEX_INITIALIZER;

#define HMSG (const char *)"SIGINT .. shutting down\n"
void handler(int signum)
{
   write(STDOUT_FILENO, HMSG, strlen(HMSG));
   doloop = 0;
}


static void * write_loop()
{
   int i;
   fd_set write_fd_set;
  
   char wr_buf[BUFLEN];

   while(doloop)
   {
      // TODO isn't killed by signal handler..
      fgets(wr_buf, BUFLEN, stdin);   
      
      //mutex
      pthread_mutex_lock(&mutex_active_fd_set);
      write_fd_set = active_fd_set;
      pthread_mutex_unlock(&mutex_active_fd_set);

      for (i = 0; i < FD_SETSIZE; ++i)
         if (i != srv_sock && FD_ISSET(i, &write_fd_set))
         {
            send(i, wr_buf, strnlen(wr_buf, BUFLEN), 0);
         }

   }

   return NULL;
}

static int start_write_loop(void)
{
   /* initialize pthread attributes */
   if(pthread_attr_init(&attr))
   {
      perror("pthread_attr_init");
      return 1;
   }

   if(pthread_create(&t, &attr, write_loop, NULL))
   {
      perror("pthread_create");
      return 2;
   }

   return 0;
}


static int stop_write_loop(void)
{
   /* wait for server to finish */
   pthread_join(t, NULL);

   if(pthread_attr_destroy(&attr))
   {
      perror("pthread_attr_destroy");
      return 1;
   }

   return 0;
}


int main()
{
   int i, new, nbytes;
   struct sockaddr_un s;
   char buf[BUFLEN];
   fd_set read_fd_set;

   struct sigaction sa;

   struct timeval select_tv;


   srv_sock = socket(AF_UNIX, SOCK_STREAM, 0);
   if (srv_sock < 0) {
      perror("socket()");
      exit(1);
   }

   s.sun_family = AF_UNIX;
   strncpy(s.sun_path, SOCKET_PATH, strlen(SOCKET_PATH));

   if (bind(srv_sock, (struct sockaddr *) &s, sizeof(struct sockaddr_un))) {
       perror("bind()");
       exit(2);
   }
   printf("listening on %s, %d\n", s.sun_path, srv_sock);
   listen(srv_sock, 5);

   /* install shutdown handler */
   sa.sa_handler = handler;
   sigemptyset(&sa.sa_mask);
   /* SA_RESTART -> resume/restart read and write libc calls */
   sa.sa_flags = 0;

   if (sigaction(SIGINT, &sa, NULL) == -1)
   {
      perror("sigaction()");
      exit(3);
   }


   
   FD_ZERO (&active_fd_set);
   FD_SET (srv_sock, &active_fd_set);

   start_write_loop();

   /* loop to accept new connections */
   while (doloop) {

      //modfied only in this thread - no mutex needed
      //pthread_mutex_lock(&mutex_active_fd_set);
      read_fd_set = active_fd_set;
      //pthread_mutex_unlock(&mutex_active_fd_set);

      select_tv.tv_sec = 1;
      select_tv.tv_usec = 0;
      if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, &select_tv) < 0)
      {
         perror("select()");
         break;
      }

      /* service all pending sockets */
      for (i = 0; i < FD_SETSIZE; ++i)
         if (FD_ISSET (i, &read_fd_set))
         {

            /* accept new connection */
            if (i == srv_sock)
            {
               if ( (new = accept(srv_sock, 0, 0)) );
               {
                   perror("accept()");
               }

               pthread_mutex_lock(&mutex_active_fd_set);
               FD_SET (new, &active_fd_set);
               pthread_mutex_unlock(&mutex_active_fd_set);
            }
            /* handle incoming message */
            else
            {
               nbytes = read (i, buf, BUFLEN - 1);
               if (nbytes < 0)
               {
                  perror("read()");
               }
               else if (nbytes == 0)
               {
                  /* end of connection */
                  close (i);

                  pthread_mutex_lock(&mutex_active_fd_set);
                  FD_CLR (i, &active_fd_set);
                  pthread_mutex_unlock(&mutex_active_fd_set);
               }
               else
               {
                  /* data read */
                  buf[nbytes] = '\0';
                  fprintf(stdout, "%s", buf);
                  fflush(stdout);
               }
            }
         }
   }

   /* clean up */

   close(srv_sock);
   unlink(SOCKET_PATH);

   stop_write_loop();

   return 0;

}
