using namespace std;
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <string>
#include <sstream> 
#include <signal.h>
#include <dirent.h>
#include <fstream>
#include "ps.h"

sighandler_t signal(int signum, sighandler_t handler);

void Help_func();
int Interpetation();
void delete_spaces(std::string);
void INThandler(int);
int Exit_func();
int MyKill(std::string,int);
//int MyPs();


int main(int argc, char **argv)
{
	signal(SIGINT, INThandler);
	printf("Hello,User!\n\r");
	Help_func();
	cout<<argv[1]<<"\n";
	while (1)
	{
		if (Interpetation())
			{
				printf("Some error..\n\r");
				exit(1);
			}
	}
	return 0;
}

void Help_func()
{
	printf("Commadns at parser:\n\r");
	
	printf(" - help\n\r");
	printf(" - run name_file(start process)\n\r");
	printf(" - ls\n\r");
	printf(" - pwd\n\r");
	printf(" - ps\n\r");
	printf(" - kill -a -b\n\r");
	printf(" - exit\n\r"); 
}

int Interpetation()
{
	string input;
	getline(cin,input);
    while(input[0]==' ')
    {
		input=input.substr(1,input.length()-1);
	}
	while(input[(int)input.length()-1]==' ')
	{
		input=input.substr(0,input.length()-1);
	}
	
	int space_pos = input.find(" ");
	string command = input.substr(0,space_pos);

	if (space_pos==-1)
	{
		if (input=="exit")
		{
			cout<<"You want to exit program..\n\r";
			if (Exit_func())
				exit(0);
			return 0;
		}
		if (input=="help")
		{
			Help_func();
			return 0;
		}
		if (input=="pwd")
		{
			printf("Current dirrectory: %s \n\r",get_current_dir_name());
			return 0;
		}
		if (input=="ls")
		{
			DIR *current_dir = opendir(".");
	        if(!current_dir)
	        {
				printf("Error opening current dirrectory..");
				return 1;
			}
	        
	        struct dirent *f_dir;
	        while ((f_dir=readdir(current_dir))!=NULL)
				printf("%s\n\r", f_dir->d_name); 
	        closedir(current_dir);
	        return 0;
		}
		if (input=="ps")
		{
			char **tmp;
			return MyPs(tmp);
		}
		printf("Parser error...unknown command\n\r");
		return 0;
	}
	else
	{
		std::string arg = input.substr(space_pos+1,input.length()-space_pos); //вычленим из строки аргументы
		
		if (command=="kill")
		{
			return MyKill(arg,space_pos);
		}
		if (command == "run")
		{
			if(system(arg.c_str()))
			{
				printf("Parser error...can't open file\n\r");
				return 0;
			}
			return 0;
		}
		else
		{
			printf("Parser error...unknown command\n\r");
			return 1;
		}
	}
		return 0;
}
void delete_spaces(std::string tmp)
{
	while(tmp[0]==' ')
    {
		tmp=tmp.substr(1,tmp.length()-1);
	}
	while(tmp[(int)tmp.length()-1]==' ')
	{
		tmp=tmp.substr(0,tmp.length()-1);
	}
}

void  INThandler(int sig)
{
     signal(sig,SIG_IGN);
     printf("You entered Ctrl-C?\n\r");
     if (Exit_func())
          exit(0);
     else
          signal(SIGINT, INThandler);
     getchar();
}

int Exit_func()
{
     char c;
	 printf("Do you really want to quit? ['y' to exit] ");
     c = getchar();
     if (c == 'y' || c == 'Y')
          return (1);
     return 0;
}

int MyKill(std::string arg,int space_pos)
{
	delete_spaces(arg);
	space_pos = arg.find(' ');
	std::string first_arg,second_arg;
	first_arg = arg.substr(1,space_pos-1);
	space_pos = arg.rfind('-');
	second_arg = arg.substr(space_pos+1,arg.length()-space_pos-1);
	printf("You want to kill proc %s by signal %s\n\r",first_arg.c_str(),second_arg.c_str());
	if (!kill(atoi(first_arg.c_str()),atoi(second_arg.c_str())))
		return 0;
	else
	{
		printf("Error:Can't kill this process!\n\r");
		return(1);
	}
}


