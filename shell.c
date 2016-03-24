#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>
#include<pwd.h>
#include<string.h>
#include<ctype.h>
#include<signal.h>
#include<fcntl.h>
#define delimiters " \t\r\n\a"                                         // Defining all the delimiters

/*STRUCTURE TO STORE ALL THE PROCESSES*/
typedef struct job{
	pid_t pid ;
	char *name ;

}job ;

job jobs[100000];
int no_of_jobs = 0;
int is_background;
int flag;
int wflag;
int fd_in =0;
int terminal;

/*FUNCTION TO DELETE THE PROCESSES AFTER IT HAS BEEN TERMINATED*/
void delete_job(pid_t pid)
{
	int i;
	int to_delete;
	for(i=0;i<no_of_jobs;i++)
	{
		if(jobs[i].pid == pid)
		{
			to_delete = i;
			break;
		}
	}
	//printf("\n%s with pid: %d exited normally\n",jobs[i].name,jobs[i].pid);
	while(i<no_of_jobs-1)
	{
		jobs[i].pid = jobs[i+1].pid;
		strcpy(jobs[i].name,jobs[i+1].name);
		i++;
	}
	no_of_jobs--;
}

/*FUNCTION TO HANDLE THE SIGNALS*/
void signal_handler(int signo)
{
	if (signo == SIGUSR1 || signo == SIGSTOP || signo == SIGKILL || signo == SIGINT)
		sleep(0);

	else if (signo == SIGTSTP)
	{
		if(no_of_jobs == 0)
		{
			sleep(0);
			printf("\n");
		}
		else
		{
			printf("\n");
			flag = 1;
			kill(jobs[no_of_jobs - 1].pid,SIGSTOP);
		}
	}
	else if (signo == SIGCHLD && wflag!=0)
	{
		int status;
		pid_t pid=waitpid(-1,&status,WNOHANG);
		if(pid>0)
		{
			int i;
			sleep(0);
			for(i=0;i<no_of_jobs;i++)
			{
				if(jobs[i].pid == pid)
					break;
			}
			printf("\n%s with pid:%d has exited normally\n",jobs[i].name,pid);
			delete_job(pid);
		}
	}
}

/* THIS	FUNCTION EXECUTES ALL THE COMMANDS PASSED */
int execute(char **arguments,int size)
{
	pid_t pid, wpid;
	int i,status,should_a = 0,should_o = 0,out,in,should_in=0;
	char outfile[1000];
	char infile[1000];

	/*CHECKING FOR REDIRECTIONS*/
	for(i=0;i<=size;i++)
	{
		if(arguments[i]!=NULL)
		{
			if(strcmp(arguments[i],">")==0)
			{

				should_o = 1;
				strcpy(outfile,arguments[i+1]);
				arguments[i] = NULL;
				arguments[i+1] = NULL;
				break;
			}
			else if(strcmp(arguments[i],">>")==0)
			{
				should_a = 1;
				strcpy(outfile,arguments[i+1]);
				arguments[i]=NULL;
				arguments[i+1]=NULL;
				break;
			}
			if(strcmp(arguments[i],"<")==0)
			{
				should_in = 1;
				strcpy(infile,arguments[i+1]);
				arguments[i] = NULL;
				arguments[i+1] = NULL;
			}
		}
	}


	/* FOR THE "CD" COMMAND */
	if(strcmp(arguments[0],"cd")==0)
	{
		if (arguments[1] == NULL) 
		{
			printf("Target Not Specified\n");
		}
		else
		{
			if (chdir(arguments[1]) != 0) 
			{
				printf("TARGET NOT FOUND\n");
			}
		}
	}

	/* FOR THE "QUIT" COMMAND */

	else if(strcmp(arguments[0],"quit")==0)
	{
		return 0;
	}

	/* FOR THE "ECHO" COMMAND */

	else if(strcmp(arguments[0],"echo")==0)
	{
		int i = 1;
		terminal = dup(1);
		if(should_o !=0 || should_a != 0)
		{
			if(should_o != 0)
				out = open(outfile,O_WRONLY|O_CREAT,0600);
			else if(should_a != 0)
				out = open(outfile,O_WRONLY|O_CREAT|O_APPEND,0600);
			if(out<0)
				printf("Error in Opening File\n");
			dup2(out,1);
			close(out);
		}

		while(arguments[i]!=NULL)
		{
			if(arguments[i][0] != '-')
				printf("%s ",arguments[i]);

			i++;
		}
		printf("\n") ;
		dup2(terminal,1);
	}

	/* ****BONUS**** */

	else if(strcmp(arguments[0],"pinfo")==0)
	{
		terminal = dup(1);
		if(should_o !=0 || should_a != 0)
		{
			if(should_o != 0)
				out = open(outfile,O_WRONLY|O_CREAT,0600);
			else if(should_a != 0)
				out = open(outfile,O_WRONLY|O_CREAT|O_APPEND,0600);
			if(out<0)
				printf("Error in Opening File\n");
			dup2(out,1);
			close(out);
		}

		char id[100];
		if(size == 0)
		{
			int g= getpid();
			sprintf(id,"%d",g);
		}
		else strcpy(id,arguments[1]);
		char **args = malloc(2 * sizeof(char *));
		args[0] = malloc(strlen(arguments[0]+1));
		args[1] = malloc(100);
		strcpy(args[0],arguments[0]);
		strcpy(args[1],"/proc/");
		strcat(args[1],id);
		strcat(args[1],"/status");
		printf("%s   %s\n",args[0],args[1]);
		execvp("cat",args);
		free(args);
		dup2(terminal,1);
		return 1 ;
	}

	/* FOR THE JOBS COMMAND*/	

	else if(strcmp(arguments[0],"jobs")==0)
	{
		terminal = dup(1);
		if(should_o !=0 || should_a != 0)
		{
			if(should_o != 0)
				out = open(outfile,O_WRONLY|O_CREAT,0600);
			else if(should_a != 0)
				out = open(outfile,O_WRONLY|O_CREAT|O_APPEND,0600);
			if(out<0)
				printf("Error in Opening File\n");
			dup2(out,1);
			close(out);
		}

		int i;
		if(no_of_jobs <= 0)
			printf("No processes are currently running.....\n");
		else
		{
			for(i=0;i<no_of_jobs;i++)
			{
				printf("[%d] %s [%d]\n",i+1,jobs[i].name,jobs[i].pid);
			}
		}
		dup2(terminal,1);
	}

	/* FOR THE FG COMMAND */

	else if(strcmp(arguments[0],"fg")==0)
	{
		if(arguments[1]==NULL)
			printf("Job number not specified.......\n");
		else
		{
			int jobnumber = atoi(arguments[1]) - 1;
			if(jobnumber <= no_of_jobs - 1)
			{
				flag =0;
				kill(jobs[jobnumber].pid,SIGCONT);
				waitpid(jobs[jobnumber].pid,NULL,WUNTRACED);
				if(flag != 1)
					delete_job(jobs[jobnumber].pid);
			}
			else
				printf("Job number does not exist......\n");
		}
	}

	/* FOR THE OVERKILL COMMAND */

	else if(strcmp(arguments[0],"overkill")==0)
	{
		while(no_of_jobs > 0)
			kill(jobs[no_of_jobs-1].pid,9);
	}

	/* FOR THE KJOB COMMAND */

	else if(strcmp(arguments[0],"kjob")==0)
	{
		if(arguments[2] == NULL || arguments[1] == NULL)
			printf("The job number or signal number not provided....\n ");
		else
		{
			int jobnumber = atoi(arguments[1])-1;
			int signalnumber = atoi(arguments[2]);
			if(jobnumber <= no_of_jobs -1)
			{
				kill(jobs[jobnumber].pid,signalnumber);
			}
			else
				printf("Job number does not exist....\n");
		}
	}

	/* FOR THE PWD COMMAND */

	else if(strcmp(arguments[0],"pwd")==0)
	{
		terminal = dup(1);
		if(should_o !=0 || should_a != 0)
		{
			if(should_o != 0)
				out = open(outfile,O_WRONLY|O_CREAT,0600);
			else if(should_a != 0)
				out = open(outfile,O_WRONLY|O_CREAT|O_APPEND,0600);
			if(out<0)
				printf("Error in Opening File\n");
			dup2(out,1);
			close(out);
		}

		char dir[100];
		getcwd(dir,100);
		printf("%s\n",dir);
		dup2(terminal,1);
	}

	/* FOR ALL OTHER COMMANDS */

	else {
		pid = fork();

		if (pid == 0)

		{
			if(should_o !=0 || should_a != 0)
			{
				if(should_o != 0)
					out = open(outfile,O_WRONLY|O_CREAT,0600);
				else if(should_a !=0)
					out = open(outfile,O_WRONLY|O_CREAT|O_APPEND,0600);
				if(out<0)
					printf("Error in Opening File\n");
				dup2(out,1);
				close(out);
			}
			if(should_in !=0)
			{

				//arguments[0] = NULL;
				in = open(infile,O_RDONLY,0600);
				if(in<0)
					printf("Error in Opening File\n");
				dup2(in,0);
				close(in);
			}
			if (execvp(arguments[0], arguments) == -1)
			{
				printf("Command Not Found\n");
			}

			exit(EXIT_FAILURE);
		} 
		else if (pid < 0)
		{
			perror("lsh");
		} 
		else
		{
			jobs[no_of_jobs].pid = pid;
			jobs[no_of_jobs].name = (char*)malloc(sizeof(char));
			strcpy(jobs[no_of_jobs].name,arguments[0]);
			no_of_jobs++;

			if(is_background == 0)
			{
				flag = 0 ;
				waitpid(pid,&status,WUNTRACED);
				if(flag != 1)
					delete_job(pid);
			}
			is_background = 0;
		}
	}
	return 1;
}

int main(int argc , char **argv)
{
	int status,i,flag = 1,j;
	char hostname[100],calldir[100];
	int sizeofbuf = 64;
	getcwd(calldir,100);						//CALLDIR CONTAINS THE PULL PATH TO THE PLACE WHERE THE PROGRAM IS CALLED
	register struct passwd *pw;
	pw = getpwuid(geteuid());
	char *username = malloc(64*sizeof(char));
	gethostname(hostname,100);                                      //HOSTNAME HAS THE NAME OF THE LAPTOP

	while(flag)
	{
		sizeofbuf = 64;
		char *line = NULL;
		ssize_t bufsize = 0;
		char location[100];
		char *print_location = malloc( 100 *sizeof(char));
		getcwd(location,100);					//LOCATION CONTAINS THE CURRENT PWD
		
		if (signal(SIGUSR1, signal_handler) == SIG_ERR)
			sleep(0);
		if (signal(SIGKILL, signal_handler) == SIG_ERR)
			sleep(0);
		if (signal(SIGSTOP, signal_handler) == SIG_ERR)
			sleep(0);
		if (signal(SIGINT, signal_handler) == SIG_ERR)
			sleep(0);
		if (signal(SIGCHLD, signal_handler) == SIG_ERR)
			sleep(0);
		if (signal(SIGTSTP, signal_handler) == SIG_ERR)
			sleep(0);


		/* THIS SPECIFIES WHERE THE HOME DIRECTORY IS */
		/********************************************************************************************************************************************************************************/
		if(strlen(location) > strlen(calldir))
		{
			int p, flag =0;
			for(p=0;calldir[p]!='\0';p++)
			{
				if(calldir[p]!=location[p])
				{
					flag = 1 ;
					break;
				}
			}
			if(flag == 0)
			{
				print_location[0] = '~';
				p=1;
				int u = strlen(calldir),c;
				for(c=u;location[c]!='\0';c++)
				{
					print_location[p++]=location[c];
				}
				print_location[p]='\0';

			}
			else
				strcpy(print_location,location);
		}
		else if(strlen(location)== strlen(calldir))
		{
			print_location[0] = '~';
			print_location[1] = '/';
			print_location[2] = '\0';
		}
		else
		{
			strcpy(print_location,location);
		}
		/***********************************************************************************************************************************************************************************/		
		printf("%s@%s : %s-> ",pw->pw_name,hostname,print_location);
		char **commands = malloc(sizeofbuf * sizeof(char*));
		getline(&line , &bufsize , stdin);
		int po,checkspace = 0;
		for(po=0;line[po] !='\0';po++)
		{
			if(!isspace(line[po]))
			{
				checkspace = 1;
				break;
			}
		}
		if(checkspace == 1)
		{
			/*if (signal(SIGUSR1, signal_handler) == SIG_ERR)
				sleep(0);
			if (signal(SIGKILL, signal_handler) == SIG_ERR)
				sleep(0);
			if (signal(SIGSTOP, signal_handler) == SIG_ERR)
				sleep(0);
			if (signal(SIGINT, signal_handler) == SIG_ERR)
				sleep(0);
			if (signal(SIGCHLD, signal_handler) == SIG_ERR)
				sleep(0);
			if (signal(SIGTSTP, signal_handler) == SIG_ERR)
				sleep(0);
*/
			for(i=0;line[i]!='\0';i++)
			{
				if(line[i]=='&')
				{
					is_background = 1;
					line[i]='\0';
					break;
				}
				else{is_background = 0;}
			}
			char *lines = strtok(line,";\n");             //SPLITTING THE LINE BASED ON ";" USING STRTOK
			i=0;
			while(lines != NULL)
			{
				commands[i++] = lines;
				lines = strtok(NULL , ";\n");

			}
			int k;
			j=0;
			while(j<i)
			{
				int u,is_piped=0;
				for(u=0;commands[j][u]!='\0';u++)
				{
					if(commands[j][u]=='|')
					{
						is_piped = 1;
						break;
					}
				}

				if(is_piped == 0)
				{
					char **arguments = malloc(sizeofbuf * sizeof(char *));

					char *token = strtok(commands[j], delimiters);
					k = 0;
					while(token != NULL)
					{
						arguments[k] = token;
						k++;
						token = strtok(NULL , delimiters);

					}
					wflag = 1;
					int fd[2];
					fd[0]=0;
					fd[1]=1;
					flag = execute(arguments,k-1);
					j++;
				}
				else
				{
					char ** pipedcommands = malloc(sizeofbuf * sizeof(char *));
					char *pi = strtok(commands[j],"|\n");
					k=0;
					while(pi != NULL)
					{
						pipedcommands[k] = pi;
						k++;
						pi = strtok(NULL,"|\n");
					}
					int l=0 ;
					int fd[2];
					fd_in = 0;
					while(pipedcommands[l]!=NULL)
					{

						pipe(fd);
						char **arguments = malloc(sizeofbuf * sizeof(char *));

						char *token = strtok(pipedcommands[l], delimiters);
						k = 0;
						while(token != NULL)
						{
							arguments[k] = token;
							k++;
							token = strtok(NULL , delimiters);

						}
						wflag = 1;
						//flag = execute(arguments,k-1,pipedcommands,fd,fd_in,l);
						pid_t pid, wpid;
						int i,status,should_o = 0,out,in,should_in=0;
						char outfile[1000];
						char infile[1000];
						int size = k-1;
						for(i=0;i<=size;i++)
						{
							if(arguments[i]!=NULL)
							{
								if(strcmp(arguments[i],">")==0)
								{
									should_o = 1;
									strcpy(outfile,arguments[i+1]);
									arguments[i] = NULL;
									arguments[i+1] = NULL;
									break;
								}
								if(strcmp(arguments[i],"<")==0)
								{
									should_in = 1;
									strcpy(infile,arguments[i+1]);
									arguments[i] = NULL;
									arguments[i+1] = NULL;
								}
							}
						}
						/* FOR THE "ECHO" COMMAND */

						if(strcmp(arguments[0],"echo")==0)
						{
							int i = 1;
							terminal = dup(1);
							if(should_o !=0)
							{
								out = open(outfile,O_WRONLY|O_CREAT,0600);
								if(out<0)
									printf("Error in Opening File\n");
								dup2(out,1);
								close(out);
							}

							while(arguments[i]!=NULL)
							{
								if(arguments[i][0] != '-')
									printf("%s ",arguments[i]);

								i++;
							}
							printf("\n") ;
							dup2(terminal,1);
						}

						/* ****BONUS**** */

						else if(strcmp(arguments[0],"pinfo")==0)
						{
							char id[100];
							if(size == 0)
							{
								int g= getpid();
								sprintf(id,"%d",g);
							}
							else strcpy(id,arguments[1]);
							char **args = malloc(2 * sizeof(char *));
							args[0] = malloc(strlen(arguments[0]+1));
							args[1] = malloc(100);
							strcpy(args[0],arguments[0]);
							strcpy(args[1],"/proc/");
							strcat(args[1],id);
							strcat(args[1],"/status");
							printf("%s   %s\n",args[0],args[1]);
							execvp("cat",args);
							free(args);
							return 1 ;
						}

						/* FOR THE JOBS COMMAND*/	

						else if(strcmp(arguments[0],"jobs")==0)
						{
							int i;
							if(no_of_jobs <= 0)
								printf("No processes are currently running.....\n");
							else
							{
								for(i=0;i<no_of_jobs;i++)
								{
									printf("[%d] %s [%d]\n",i+1,jobs[i].name,jobs[i].pid);
								}
							}
						}

						/* FOR THE FG COMMAND */

						else if(strcmp(arguments[0],"fg")==0)
						{
							if(arguments[1]==NULL)
								printf("Job number not specified.......\n");
							else
							{
								int jobnumber = atoi(arguments[1]) - 1;
								if(jobnumber <= no_of_jobs - 1)
								{
									flag =0;
									kill(jobs[jobnumber].pid,SIGCONT);
									waitpid(jobs[jobnumber].pid,NULL,WUNTRACED);
									if(flag != 1)
										delete_job(jobs[jobnumber].pid);
								}
								else
									printf("Job number does not exist......\n");
							}
						}

						/* FOR THE OVERKILL COMMAND */

						else if(strcmp(arguments[0],"overkill")==0)
						{
							while(no_of_jobs > 0)
								kill(jobs[no_of_jobs-1].pid,9);
						}

						/* FOR THE KJOB COMMAND */

						else if(strcmp(arguments[0],"kjob")==0)
						{
							if(arguments[2] == NULL || arguments[1] == NULL)
								printf("The job number or signal number not provided....\n ");
							else
							{
								int jobnumber = atoi(arguments[1])-1;
								int signalnumber = atoi(arguments[2]);
								if(jobnumber <= no_of_jobs -1)
								{
									kill(jobs[jobnumber].pid,signalnumber);
								}
								else
									printf("Job number does not exist....\n");
							}
						}

						/* FOR THE PWD COMMAND */

						else if(strcmp(arguments[0],"pwd")==0)
						{
							char dir[100];
							getcwd(dir,100);
							printf("%s\n",dir);
						}

						/* FOR ALL OTHER COMMANDS */

						else {
							pid = fork();

							if (pid == 0)

							{
								if(should_o !=0)
								{
									out = open(outfile,O_WRONLY|O_CREAT,0600);
									if(out<0)
										printf("Error in Opening File\n");
									dup2(out,1);
									close(out);
								}
								else
								{
									if(pipedcommands[l+1]!=NULL)
										dup2(fd[1],1);
								}
								if(should_in !=0)
								{

									//arguments[0] = NULL;
									in = open(infile,O_RDONLY,0600);
									if(in<0)
										printf("Error in Opening File\n");
									dup2(in,0);
									close(in);
								}
								else
								{
									dup2(fd_in,0);
								}
								close(fd[0]);
								if (execvp(arguments[0], arguments) == -1)
								{
									printf("Command Not Found\n");
								}

								exit(EXIT_FAILURE);
								flag  =0;
							} 
							else if (pid < 0)
							{
								perror("lsh");
							} 
							else
							{
								jobs[no_of_jobs].pid = pid;
								jobs[no_of_jobs].name = (char*)malloc(sizeof(char));
								strcpy(jobs[no_of_jobs].name,arguments[0]);
								no_of_jobs++;

								if(is_background == 0)
								{
									flag = 0 ;
									waitpid(pid,&status,WUNTRACED);
									if(flag != 1)
										delete_job(pid);
								}
								is_background = 0;
								close(fd[1]);
								fd_in=fd[0];
								l++;
							}
							flag =1;
						}


					}
					j++;
				}
			}
		
			free(lines);
		}
		free(line);
		free(commands);
	}
	return EXIT_SUCCESS;
}
