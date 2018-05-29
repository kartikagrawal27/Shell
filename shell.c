#include "format.h"
#include "log.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
Log* init();
char *  get_command(FILE * input);
int exec_command(char *command, Log * log);
void end(Log * log);
int Log_Process(Log* log, char** command, FILE *input);
static char* g_orig_dir = NULL;
static char *file_to_write_name = NULL; 
static int hpresent = 0;
void handle_int(){return;}
char * Log_find_from_last(Log *log, const char *prefix);
static int inputfilecounter = 0;
Log *ifbothexist = NULL;


int main(int argc, char *argv[]) { 

	signal(SIGINT, handle_int);
    g_orig_dir = getcwd(NULL,0);
    int exec_return = 0;
    FILE * input = NULL;
    Log* log = init(argc, argv, &input);
    pid_t pid = getpid();
    char * command = NULL;
    print_shell_owner("kagrawa3");
    while(exec_return != -1)
    {
        char * pwd = getcwd (NULL, 0);
        print_prompt(pwd, pid);
        command = get_command(input);
       // printf("command received is \"%s\" and my log size is%d\n", command, (int)Log_size(log));
        if(input != stdin){
            //printf("Executing this\n");// remove this
            inputfilecounter++;
            printf("%s\n",command);
        }
        //printf("before calling log_process\n");
        int log_flag = Log_Process(log, &command, input);
        //printf("Log flag is %d while executing %s\n", log_flag, command);
        if(log_flag == 0) 
        {
            exec_return = exec_command(command, log);
            free(command);
        }
        else if(log_flag == 2) 
        {
            exec_return = exec_command(command, log);
        }
        free(pwd);
    }
    end(log);
    return 0;
}
int exec_command(char * command, Log *log)
{
    if(command == NULL)
        return 0;
    else if(strcmp(command,"exit") == 0)
        return -1;
    else if(strncmp(command,"cd ", 3) == 0)
    {
        if(chdir(command+3) == -1)
            print_no_directory(command+3);
        if(ifbothexist!=NULL)
        {
            Log_add_command(log, command);
        }
    }
    else
    {
        //printf("Come here\n");
        int checker= 0;
        int background = 0;
        int com_len = strlen(command);
        if(command[com_len-1] == '&')
        {
            background = 1;
            command[com_len -1] = '\0';
        }
        pid_t pid = fork();
        if(pid == 0)
        {
            print_command_executed(getpid());
            char ** args = malloc(sizeof(char*) * com_len);
            args[0] = strtok(command," ");
            for(int i = 1; i < com_len; i++)
            {
                args[i] = strtok(NULL," ");
                if(args[i] == NULL)
                    break;
            }
            checker = execvp(args[0],args);
            print_exec_failed(command);
            exit(1);
        }
        else
        {
            int status = 0;
            if(background != 1)
                waitpid(pid,&status,0);
            if(ifbothexist!=NULL && checker !=-1)
            {
                Log_add_command(log, command);
            }
        } 
    }
    return 0;
}
char * get_command(FILE * input)
{
    int numberOfCharsRead = 0;
    char* lineptr = calloc(sizeof(char),20);
    size_t sizeOfLineptr = 20 * sizeof(char);
    {
        numberOfCharsRead = getline(&lineptr,&sizeOfLineptr,input);
        if(numberOfCharsRead > 0)
        {
            size_t newLine = strlen(lineptr) - 1;
            if (lineptr[newLine] == '\n')
                lineptr[newLine] = '\0';
            return lineptr;
        }
        else
        {
            free(lineptr);
            lineptr = malloc(sizeof(char)*10);
            strcpy(lineptr,"exit");
            return lineptr;
        }
    }
}
int Log_Process(Log* log, char** command, FILE *input)
{
    //printf("Log process started and my command is %s\n", *command);
    if(strcmp(*command,"!history") == 0)
    {
        //printf("Enter here\n");
        //free(*command);
        //printf("My log size is %d\n", (int)Log_size(log));
        int sizeofimportance;
        if(input!=stdin)
            sizeofimportance = inputfilecounter-1;
        else
            sizeofimportance = (int)Log_size(log);
        for(int i=0;i<sizeofimportance;i++)
        {
            char *temp = (char*)Log_get_command(log, i);
            printf("%d\t%s\n",i,temp);
            //free(temp);
        }
        //printf("Exiting now\n");
        //if(input==stdin)
        //    Log_add_command(log,*command);
        return 1;
    }
    int ret = 0;
    if((*command)[0] == '!')
    {
        //printf("entered here my man\n");
        ret = 2;
        if(strlen(*command)<=1){
            print_invalid_index();
            return 1;
        }   
        //printf("Reached here surprise\n");
        //printf("after ! is:%s\n", (*command)+1);
        char * tmp = (char*)Log_find_from_last(log,(*command)+1);
        //printf("Command retrievd is %s\n", tmp);
        
        free(*command);
        (*command) = tmp;

        if(tmp != NULL)
        {
            printf("%s\n",*command);

            int com_len = strlen(tmp);
            pid_t pid = fork();
            if(pid == 0)
            {
                print_command_executed(getpid());
                /*
                int sizeofimportance;
                if(input!=stdin)
                    sizeofimportance = inputfilecounter-1;
                else
                    sizeofimportance = (int)Log_size(log);
                for(int i=0;i<sizeofimportance;i++)
                {
                    char *temp = (char*)Log_get_command(log, i);
                    printf("%d\t%s\n",i,temp);
                    //free(temp);
                }
                */
                char ** args = malloc(sizeof(char*) * com_len);
                args[0] = strtok(tmp," ");
                for(int i = 1; i < com_len; i++)
                {
                    args[i] = strtok(NULL," ");
                    if(args[i] == NULL)
                        break;
                }
                execvp(args[0],args);
                print_exec_failed(tmp);
                exit(1);
            }
            else
            {
                int status = 0;
                waitpid(pid,&status,0);
                if(input==stdin && strlen(tmp)>0)
                    Log_add_command(log,tmp);
                return 1;
            }
        }
        else
        {
            print_no_history_match();
            return 1;
        }
    }
    if(*command[0]=='#')
    {
        if(strlen(*command)<=1)
        {
            print_invalid_index();
            end(log);
        }
        else
        {       
            char *temp;
            temp = strdup(*(command)+1);
            //printf("the value parsed is%s\n", temp);
            int val = atoi(temp);
            free(temp);
            if(val>(int)Log_size(log)-1 || val<0)
            {
                print_invalid_index();
                return 1;
            }    
            else
            {
                //printf("My value of val is: %d", val);
                char *myhashtemp =(char *)Log_get_command(log, val);
                printf("%s\n", myhashtemp);

                int com_len = strlen(myhashtemp);
                pid_t pid = fork();
                if(pid == 0)
                {
                    print_command_executed(getpid());

                    /*
                    int sizeofimportance;
                    if(input!=stdin)
                        sizeofimportance = inputfilecounter-1;
                    else
                        sizeofimportance = (int)Log_size(log);
                    for(int i=0;i<sizeofimportance;i++)
                    {
                        char *temp = (char*)Log_get_command(log, i);
                        printf("%d\t%s\n",i,temp);
                        //free(temp);
                    }
                    */
                    char ** args = malloc(sizeof(char*) * com_len);
                    args[0] = strtok(myhashtemp," ");
                    for(int i = 1; i < com_len; i++)
                    {
                        args[i] = strtok(NULL," ");
                        if(args[i] == NULL)
                            break;
                    }
                    execvp(args[0],args);
                    print_exec_failed(myhashtemp);
                    exit(1);
                }
                else
                {
                    int status = 0;
                    waitpid(pid,&status,0);
                    if(input==stdin && strlen(myhashtemp)>0)
                        Log_add_command(log,myhashtemp);
                    return 1;
                }


            }
        }
    }
    if(input==stdin && strlen(*command)>0 && (strcmp(*(command),"exit") != 0))
        Log_add_command(log,*command);
    return ret;
}
Log* init(int argc, char *argv[], FILE ** input)
{
    int file_read = 0;
    char* file_to_read = NULL;
    int c;
    while ((c = getopt (argc, argv, "h:f:")) != -1)
        switch (c)
        {
            case 'h':
                if(hpresent==1)
                {
                    print_usage();
                    exit(1);
                }
                else
                {
                    //printf("acknowledged -h\n");
                    hpresent = 1;
                }
               // printf("optarg's value is %s\n", optarg);
                file_to_write_name = optarg;
               // printf("file to write name has been updated to %s\n", file_to_write_name);
                break;
            case 'f':
                if(file_read==1)
                {
                    print_usage();
                    exit(1);
                }
                else{
                 //   printf("acknowledged -f\n");
                    file_read = 1;
                }
               // printf("optarg's value is %s\n", optarg);
                file_to_read = optarg;
              //  printf("file to read name has been updated to %s\n", file_to_read);
                //printf("Reached here");
                break;
            case '?':
                if (optopt == 'f')
                {
                    //print_usage();
                    exit(1);
                }
                else if (optopt=='h'){
                    //print_usage();
                    exit(1);
                }
                else
                {
                    //print_usage();
                    exit(1);
                }
            default:
                abort();
        }

    if(argc>1 && ((file_read==0) && (hpresent==0)))
    {
        print_usage();
        free(g_orig_dir);
        exit(1);
    }

    if(file_read == 1 && hpresent ==1)
    {
        *input = fopen(file_to_read,"r");
        if(*input==NULL)
        {
            print_script_file_error();
            free(g_orig_dir);
            exit(1);
        }
        else
        {
            //printf("Enter here\n");
            Log *ret = Log_create_from_file(file_to_write_name);
            ifbothexist = Log_create_from_file(file_to_read);
            return ret;
        }
    }
    else if(file_read==0 && hpresent==1)
     {
        *input = fopen(file_to_write_name, "r");
        if(*input==NULL)
        {
            print_script_file_error();
            free(g_orig_dir);
            exit(1);
        }
        *input = stdin;
        Log *ret = Log_create_from_file(file_to_write_name);
        return ret;
     }  

    else if(file_read==1 && hpresent==0)
    {
        *input = fopen(file_to_read, "r");
        if(*input==NULL)
        {
            print_script_file_error();
            free(g_orig_dir);
            exit(1);
        }
        else
        {
            Log *ret = Log_create_from_file(file_to_read);
            return ret;
        }
    }
    else
    {
        *input = stdin;
        Log* ret = Log_create();
        return ret;
    }
}

char * Log_find_from_last(Log * log, const char *prefix)
{
    if(prefix==NULL)
        return NULL;
    char *temp = NULL;
    for(int i=(int)Log_size(log)-1;i>=0;i--)
    {
        temp = (char *)Log_get_command(log,i);
        if(temp!=NULL)
        {
            if(strstr(temp, prefix)==temp)
                return temp;
        }
    }
    return NULL;
        
}
void end(Log* log)
{
    //char * tmp = malloc(sizeof(char) * (strlen(g_orig_dir)+20));
    //strcpy(tmp,g_orig_dir);
   //printf("The file I will write to is called:%s\n", file_to_write_name);

    if(hpresent==1)
    {
    	//printf("Enter in the end\n");
        // for(int i=0;i<(int)Log_size(log);i++)
        // {
        //     char *temp = (char*)Log_get_command(log, i);
        //     printf("%d\t%s\n",i,temp);
        //     //free(temp);
        // }
        Log_save(log, file_to_write_name);
        //printf("Got here too\n");
    }
    //free(tmp);
    free(g_orig_dir);
    Log_destroy(log);
}
